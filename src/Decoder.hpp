/*
 * Copyright (c) 2015-2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
 *
 * This file is part of "libRaptorQ".
 *
 * libRaptorQ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * libRaptorQ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and a copy of the GNU Lesser General Public License
 * along with libRaptorQ.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.hpp"
#include "Graph.hpp"
#include "Parameters.hpp"
#include "Precode_Matrix.hpp"
#include "Shared_Computation/Decaying_LF.hpp"
#include "Shared_Computation/LZ4_Wrapper.hpp"
#include "Thread_Pool.hpp"
#include <memory>
#include <mutex>
#include <random>
#include <tuple>
#include <utility>
#include <vector>
#include <Eigen/Dense>


namespace RaptorQ__v1 {
namespace Impl {

extern template class Precode_Matrix<Save_Computation::OFF>;
extern template class Precode_Matrix<Save_Computation::ON>;

template <typename In_It>
class RAPTORQ_API Decoder
{
	// decode () can be launched multiple times,
	// But each time the list of source and repair symbols might
	// change. we also track the number of times we failed at decoding
	// so that we can drop some repair symbol.
	// Dropping sone repair symbol and using an other instead might make
	// things decodable, even without additional repair symbols.
	using Vect = Eigen::Matrix<Octet, 1, Eigen::Dynamic, Eigen::RowMajor>;
	using T_in = typename std::iterator_traits<In_It>::value_type;
public:
	Decoder (const uint16_t symbols, const uint16_t symbol_size)
		:_symbols (symbols), combination (0), combination_drop_sym (0),
			keep_working (true), type (test_computation()), mask (_symbols)
	{
		IS_INPUT(In_It, "RaptorQ__v1::Impl::Decoder");
		// symbol size is in octets, but we save it in "T" sizes.
		// so be aware that "symbol_size" != "_symbol_size" for now
		source_symbols = DenseMtx (_symbols, symbol_size);
		concurrent = 0;
	}
	~Decoder();

	enum class RAPTORQ_API Decoder_Result : uint8_t {
		DECODED = 0,
		STOPPED = 1,
		CAN_RETRY = 2,
		NEED_DATA = 3
	};

	Error add_symbol (In_It &start, const In_It end, const uint32_t esi);
	Decoder_Result decode (Work_State *thread_keep_working);
	DenseMtx* get_symbols();

	bool can_decode() const;
	void stop();
	bool ready() const;
	// should we add work? we have a maximum amount of
	bool add_concurrent (const uint16_t max_concurrent);
	void drop_concurrent();

private:
	// FIXME: multiple decoding should start only after the first
	// decoding has failed!
	std::mutex lock;
	const uint16_t _symbols;
	uint16_t combination, combination_drop_sym; // failure trackers
	uint16_t concurrent;	// currently running decoders retry
	bool keep_working;
	const Save_Computation type;
	Bitmask mask;
	std::vector<bool> selector;
	DenseMtx source_symbols;
	std::vector<std::pair<uint32_t, Vect>> received_repair;


	// to help making things const
	static Save_Computation test_computation()
	{
		if (DLF<std::vector<uint8_t>, Cache_Key>::get()->get_size() != 0)
			return Save_Computation::ON;
		return Save_Computation::OFF;
	}
	// to help making things const
	Precode_Matrix<Save_Computation::ON> *init_precode_on (
												const uint16_t symbols) const
	{
		if (type == Save_Computation::ON) {
			return new Precode_Matrix<Save_Computation::ON> (
														Parameters(symbols));
		}
		return nullptr;
	}
	// to help making things const
	Precode_Matrix<Save_Computation::OFF> *init_precode_off(
												const uint16_t symbols) const
	{
		if (type == Save_Computation::OFF) {
			return new Precode_Matrix<Save_Computation::OFF> (
														Parameters(symbols));
		}
		return nullptr;
	}
};


///////////////////////////////////
//
// IMPLEMENTATION OF ABOVE TEMPLATE
//
///////////////////////////////////

template <typename In_It>
Decoder<In_It>::~Decoder()
{
	stop();
}

template <typename In_It>
void Decoder<In_It>::stop()
{
	keep_working = false;
}

template <typename In_It>
bool Decoder<In_It>::ready() const
{
	return mask.get_holes() == 0;
}

template <typename In_It>
bool Decoder<In_It>::can_decode() const
{
	const int32_t total_overhead =
								static_cast<int32_t> (received_repair.size()) -
								static_cast<int32_t> (mask.get_holes());
	if (total_overhead < 0 || combination_drop_sym > total_overhead)
		return false;
	return true;
}

template <typename In_It>
bool Decoder<In_It>::add_concurrent (const uint16_t max_concurrent)
{
	std::unique_lock<std::mutex> guard (lock);
	if (max_concurrent > concurrent) {
		++concurrent;
		return true;
	}
	return false;
}

template <typename In_It>
void Decoder<In_It>::drop_concurrent()
{
	std::unique_lock<std::mutex> guard (lock);
	// "if" should not be necessary. But I forgot to add --make-bug-free flag.
	if (concurrent > 0)
		--concurrent;
}

template <typename In_It>
Error Decoder<In_It>::add_symbol (In_It &start, const In_It end,
															const uint32_t esi)
{
	// true if added succesfully

	// if we were lucky to get a random access iterator, quickly check that
	// the we have enough data for the symbol.
	if (std::is_same<typename std::iterator_traits<In_It>::iterator_category,
									std::random_access_iterator_tag>::value) {
		if (static_cast<size_t>(end - start) * sizeof(T_in) <
								static_cast<size_t> (source_symbols.cols()))
			return Error::WRONG_INPUT;
	}

	if (esi >= std::pow (2, 20))
		return Error::WRONG_INPUT;

	std::lock_guard<std::mutex> guard (lock);
	UNUSED(guard);

	if (mask.get_holes() == 0 || mask.exists (esi))
		return Error::NOT_NEEDED;	// not even needed.

	uint16_t col = 0;
	if (esi < _symbols) {
		for (; start != end && col != source_symbols.cols(); ++start) {
			T_in al = *start;
			for (uint8_t *p = reinterpret_cast<uint8_t *> (&al);
						p != reinterpret_cast<uint8_t *> (&al) + sizeof(T_in)
										&& col != source_symbols.cols(); ++p) {
				source_symbols (esi, col++) = *p;
			}
		}
		// input iterator might reach end before we get enough data
		// for the symbol.
		if (col != source_symbols.cols())
			return Error::WRONG_INPUT;
	} else {
		Vect v = Vect (source_symbols.cols());
		for (; start != end && col != source_symbols.cols(); ++start) {
			T_in al = *start;
			for (uint8_t *p = reinterpret_cast<uint8_t *> (&al);
						p != reinterpret_cast<uint8_t *> (&al) + sizeof(T_in)
										&& col != source_symbols.cols(); ++p) {
				v (col++) = *p;
			}
		}
		// input iterator might reach end before we get enough data
		// for the symbol.
		if (col != v.cols())
			return Error::WRONG_INPUT;
		received_repair.emplace_back (esi, std::move(v));
		// reorder the received_repair:
		// ordering the repair packets lets us have more deterministic
		// matrices, that we can use for precomputation.
		// this *should* not be a big performance hit as the repair symbols
		// should already be *almost* in order.
		// TODO: b-tree?
		int32_t idx = static_cast<int32_t> (received_repair.size()) - 1;
		while (idx >= 1) {
			const uint32_t idx_u = static_cast<uint32_t> (idx);
			const uint32_t prev = idx_u - 1;
			if (received_repair[idx_u].first < received_repair[prev].first) {
				std::swap (received_repair[idx_u], received_repair[prev]);
				--idx;
			} else {
				break;
			}
		}
	}
	mask.add (esi);

	selector = std::vector<bool> (received_repair.size(), true);
	combination_drop_sym = 0;
	combination = 0;	// new data. try again all combinations from the start.
	return Error::NONE;
}

template <typename In_It>
typename Decoder<In_It>::Decoder_Result Decoder<In_It>::decode (
												Work_State *thread_keep_working)
{
	// this method can be launched concurrently multiple times.
	// track the failures in "combination"


	// rfc 6330: can decode when received >= K_padded
	// actually: (K_padded - K) are padding and thus constant and NOT
	// transmitted. so really N >= K.
	if (mask.get_holes() == 0)
		return Decoder_Result::DECODED;

	if (received_repair.size() < mask.get_holes())
		return Decoder_Result::NEED_DATA;

	const std::unique_ptr<Precode_Matrix<Save_Computation::ON>> precode_on (
												init_precode_on (_symbols));
	const std::unique_ptr<Precode_Matrix<Save_Computation::OFF>> precode_off (
												init_precode_off (_symbols));
	std::unique_lock<std::mutex> shared (lock);
	// if this is not the first time we try to decode the block,
	// we might change which symbols we use.
	// so now we will cycle between
	uint32_t dropped_repair = 0;
	std::vector<uint32_t> drop;
	const uint32_t total_overhead = static_cast<uint32_t> (
									received_repair.size() - mask.get_holes());
	// already tried enough times. don't bother until
	if (combination_drop_sym > total_overhead)
		return Decoder_Result::NEED_DATA;
	// new daa arrives.
	if (combination > 20) {
		// we are already sure that overhead > 0
		// is someone trying very carefully to feed us an undecodable
		// set of symbols?
		// randomize dropping.
		std::random_device rd;
		std::mt19937 rnd (rd());// no crypto safety needed -- mersenne is enough

		std::uniform_int_distribution<size_t> dis_dropped  (1, total_overhead);
		dropped_repair = static_cast<uint32_t> (dis_dropped (rnd));
		std::uniform_int_distribution<size_t> dis_repair  (0,
													received_repair.size() - 1);
		drop.reserve (dropped_repair);
		for (uint32_t i = 0; i < dropped_repair; ++i)
			drop.push_back (received_repair[dis_repair (rnd)].first);
		std::sort (drop.begin(), drop.end());
	} else {
		dropped_repair = combination_drop_sym;
		drop.reserve (dropped_repair);
		++combination;
		auto rec_it = received_repair.begin();
		for (auto sel_it = selector.begin(); sel_it != selector.end();
															++sel_it,++rec_it) {
			if (*sel_it)
				continue;
			drop.push_back (rec_it->first);
		}
		// next selector for next retry.
		if (combination_drop_sym == 0) {
			++combination_drop_sym;
			std::fill (selector.begin(), selector.end(), true);
			selector[0] = false;
		} else {
			auto next = std::prev_permutation (selector.begin(),selector.end());
			if (!next) {
				if (++combination_drop_sym <= total_overhead) {
					std::fill (selector.begin(), selector.end(), true);
					std::fill (	selector.begin(),
								selector.begin() + dropped_repair, false);
				}
			}
		}
	}

	const uint16_t overhead = static_cast<uint16_t> (total_overhead -
																dropped_repair);
	if (type == Save_Computation::ON) {
		precode_on->gen (static_cast<uint32_t> (overhead));
	} else {
		precode_off->gen (static_cast<uint32_t> (overhead));
	}

	uint16_t S_H;
	uint16_t L_rows;
	bool DO_NOT_SAVE = false;
	std::vector<bool> bitmask_repair;
	if (type == Save_Computation::ON) {
		L_rows = precode_on->_params.L;
		S_H = precode_on->_params.S + precode_on->_params.H;
		// repair.rend() is the highest repair symbol
		const auto it = received_repair.rend();
		if (it->first >= std::pow (2, 16)) {
			DO_NOT_SAVE = true;
		} else {
			bitmask_repair.reserve ((it->first - _symbols) - dropped_repair);
			uint32_t idx = _symbols;
			auto drop_it = drop.begin();
			for (auto rep = received_repair.begin();
								rep != received_repair.end(); ++rep, ++idx) {
				for (;idx < rep->first; ++idx)
					bitmask_repair.push_back (false);
				// remember to drop the symbols on retries
				if (drop_it != drop.end() && *drop_it == idx) {
					++drop_it;
					bitmask_repair.push_back (false);
				} else {
					bitmask_repair.push_back (true);
				}
			}
		}
	} else {
		L_rows = precode_off->_params.L;
		S_H = precode_off->_params.S + precode_off->_params.H;
	}
	const Cache_Key key (L_rows, mask.get_holes(), bitmask_repair);

	DenseMtx D = DenseMtx (L_rows + overhead, source_symbols.cols());

	// initialize D
	for (uint16_t row = 0; row < S_H; ++row) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}
	// put non-repair symbols (source symbols) in place
	if (mask.get_holes() == 0) {
		// other thread completed its work before us?
		return Decoder_Result::DECODED;
	}
	D.block (S_H, 0, source_symbols.rows(), D.cols()) = source_symbols;

	// mask must be copied to avoid threading problems, same with tracking
	// the repair esi.
	const Bitmask mask_safe = mask;
	for (const auto drop_me : drop)
		mask.drop (drop_me);
	auto drop_it = drop.begin();
	std::vector<uint32_t> repair_esi;
	repair_esi.reserve (received_repair.size() - dropped_repair);
	for (auto rep : received_repair) {
		if (drop_it != drop.end() && *drop_it == rep.first) {
			++drop_it;
			continue;
		}
		repair_esi.push_back (rep.first);
	}

	drop_it = drop.begin();

	// fill holes with the first repair symbols available
	auto symbol = received_repair.begin();
	uint16_t hole = 0;
	while (hole < _symbols && symbol != received_repair.end()) {
		if (mask_safe.exists (static_cast<size_t> (hole))) {
			++hole;
			continue;
		}
		if (drop_it != drop.end() && symbol->first == *drop_it) {
			++drop_it;
			++symbol;
			continue;
		}
		const uint16_t row = S_H + hole;
		D.row (row) = symbol->second;
		++symbol;
		++hole;
	}
	// fill the padding symbols (always zero)
	for (uint16_t row = S_H + _symbols; row < L_rows; ++row)
		D.row (row).setZero();
	// fill the remaining (redundant) repair symbols
	// remember to drop the repair symbols as needed
	drop_it = drop.begin();
	for (uint16_t row = L_rows; symbol != received_repair.end(); ++symbol) {
		if (drop_it != drop.end() && *drop_it == symbol->first) {
			++drop_it;
			continue;
		}
		D.row (row) = symbol->second;
		++row;
	}

	// do not lock this part, as it's the expensive part
	shared.unlock();
	std::deque<std::unique_ptr<Operation>> ops;

	Precode_Result precode_res = Precode_Result::DONE;
	DenseMtx missing;
	if (type == Save_Computation::ON) {
		std::vector<uint8_t> compressed = DLF<std::vector<uint8_t>, Cache_Key>::
															get()->get (key);
		LZ4<LZ4_t::DECODER> lz4;
		auto uncompressed = lz4.decode (compressed);
		DenseMtx precomputed = raw_to_Mtx (uncompressed,
													key._mt_size + overhead);
		if (precomputed.rows() != 0) {
			missing = precomputed * D;
			DO_NOT_SAVE = true;
		} else {
			std::tie (precode_res, missing) = precode_on->intermediate (D,
											mask_safe, repair_esi, ops,
											keep_working, thread_keep_working);
		}
	} else {
		std::tie (precode_res, missing) = precode_off->intermediate (D,
											mask_safe, repair_esi, ops,
											keep_working, thread_keep_working);
	}
	if (precode_res == Precode_Result::STOPPED)
		return Decoder_Result::STOPPED;

	D = DenseMtx();	// free some memory;
	if (type == Save_Computation::ON && !DO_NOT_SAVE &&
										precode_res == Precode_Result::DONE) {
		DenseMtx res;
		// don't save really small matrices
		// TODO: check again if other thread already saved this.
		if (missing.rows() != 0 && L_rows > 100) {
			res.setIdentity (L_rows + overhead, L_rows + overhead);
			for (auto &op : ops)
				op->build_mtx (res);
			// TODO: lots of wasted ram? how to compress things directly?
			const auto raw_mtx = Mtx_to_raw (res);
			LZ4<LZ4_t::ENCODER> lz4;
			auto compressed = lz4.encode (raw_mtx);
			DLF<std::vector<uint8_t>, Cache_Key>::get()->add (compressed, key);
		}
	}

	std::lock_guard<std::mutex> dec_lock (lock);
	UNUSED(dec_lock);

	if (precode_res == Precode_Result::FAILED) {
		if (combination_drop_sym > total_overhead)
			return Decoder_Result::NEED_DATA;
		return Decoder_Result::CAN_RETRY;
	}
	if (mask.get_holes() == 0)
		return Decoder_Result::DECODED;

	// put missing symbols into "source_symbols".
	// remember: we might have received other symbols while decoding.
	uint16_t miss_row = 0;
	for (uint16_t row = 0; row < mask_safe._max_nonrepair &&
											miss_row < missing.rows(); ++row) {
		if (mask_safe.exists (row))
			continue;
		++miss_row;
		if (mask.exists (row))
			continue;
		source_symbols.row (row) = missing.row (miss_row - 1);
		mask.add (row);
	}

	keep_working = false;	 // tell eventual threads to top crunching,
	// free some memory, we don't need recover symbols anymore
	received_repair = std::vector<std::pair<uint32_t, Vect>>();
	mask.free();
	return Decoder_Result::DECODED;
}

template <typename In_It>
DenseMtx* Decoder<In_It>::get_symbols()
{
	return &source_symbols;
}

}	//namespace Impl
}	// namespace RaptorQ
