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
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>
#include <vector>
#include <Eigen/Dense>

namespace RaptorQ {
namespace Impl {

extern template class Precode_Matrix<Save_Computation::OFF>;
extern template class Precode_Matrix<Save_Computation::ON>;

template <typename In_It>
class RAPTORQ_API Decoder
{
	using Vect = Eigen::Matrix<Octet, 1, Eigen::Dynamic, Eigen::RowMajor>;
	using T_in = typename std::iterator_traits<In_It>::value_type;
public:
	Decoder (const uint16_t symbols, const uint16_t symbol_size)
		:_symbols (symbols), type (test_computation()),
			precode_on  (init_precode_on  (symbols)),
			precode_off (init_precode_off (symbols)),
			mask (_symbols)
	{
		IS_INPUT(In_It, "RaptorQ::Impl::Decoder");
		// symbol size is in octets, but we save it in "T" sizes.
		// so be aware that "symbol_size" != "_symbol_size" for now
		source_symbols = DenseMtx (_symbols, symbol_size);
	}

	bool add_symbol (In_It &start, const In_It end, const uint32_t esi);
	bool decode();
	DenseMtx* get_symbols();
	//std::vector<T> get (const uint16_t symbol) const;

private:
	std::mutex lock;
	const uint16_t _symbols;
	const Save_Computation type;
	const std::unique_ptr<Precode_Matrix<Save_Computation::ON>> precode_on;
	const std::unique_ptr<Precode_Matrix<Save_Computation::OFF>> precode_off;
	Bitmask mask;
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
bool Decoder<In_It>::add_symbol (In_It &start, const In_It end,
															const uint32_t esi)
{
	// true if added succesfully

	// if we were lucky to get a random access iterator, quickly check that
	// the we have enough data for the symbol.
	if (std::is_same<typename std::iterator_traits<In_It>::iterator_category,
									std::random_access_iterator_tag>::value) {
		if (static_cast<size_t>(end - start) * sizeof(T_in) <
								static_cast<size_t> (source_symbols.cols()))
			return false;
	}

	if (esi >= std::pow (2, 20))
		return false;

	std::lock_guard<std::mutex> guard (lock);
	UNUSED(guard);

	if (mask.get_holes() == 0)
		return false;	// not even needed.
	if (mask.exists (esi))
		return false;	// already present.

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
			return false;
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
			return false;
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

	return true;
}

template <typename In_It>
bool Decoder<In_It>::decode()
{
	// rfc 6330: can decode when received >= K_padded
	// actually: (K_padded - K) are padding and thus constant and NOT
	// transmitted. so really N >= K.
	if (mask.get_holes() == 0)
		return true;

	if (received_repair.size() < mask.get_holes())
		return false;

	const uint16_t overhead = static_cast<uint16_t> (received_repair.size() -
															mask.get_holes());
	if (type == Save_Computation::ON) {
		precode_on->gen (static_cast<uint32_t> (overhead));
	} else {
		precode_off->gen (static_cast<uint32_t> (overhead));
	}

	uint16_t S_H;
	uint16_t K_S_H;
	uint16_t L_rows;
	bool DO_NOT_SAVE = false;
	std::vector<bool> bitmask_repair;
	if (type == Save_Computation::ON) {
		L_rows = precode_on->_params.L;
		S_H = precode_on->_params.S + precode_on->_params.H;
		K_S_H = precode_on->_params.K_padded + S_H;
		// repair.rend() is the highest repair symbol
		const auto it = received_repair.rend();
		if (it->first >= std::pow(2,16)) {
			DO_NOT_SAVE = true;
		} else {
			bitmask_repair.reserve (it->first - _symbols);
			uint32_t idx = _symbols;
			for (auto rep = received_repair.begin();
								rep != received_repair.end(); ++rep, ++idx) {
				for (;idx < rep->first; ++idx)
					bitmask_repair.push_back (false);
				bitmask_repair.push_back (true);
			}
		}
	} else {
		L_rows = precode_off->_params.L;
		S_H = precode_off->_params.S + precode_off->_params.H;
		K_S_H = precode_off->_params.K_padded + S_H;
	}
	const Cache_Key key (L_rows, mask.get_holes(), bitmask_repair);

	DenseMtx D = DenseMtx (K_S_H + overhead, source_symbols.cols());

	// initialize D
	for (uint16_t row = 0; row < S_H; ++row) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}
	// put non-repair symbols (source symbols) in place
	lock.lock();
	if (mask.get_holes() == 0) { // other thread completed its work before us?
		lock.unlock();
		return true;
	}
	D.block (S_H, 0, source_symbols.rows(), D.cols()) = source_symbols;

	// mask must be copied to avoid threading problems, same with tracking
	// the repair esi.
	const Bitmask mask_safe = mask;
	std::vector<uint32_t> repair_esi;
	repair_esi.reserve (received_repair.size());
	for (auto rep : received_repair)
		repair_esi.push_back (rep.first);

	// fill holes with the first repair symbols available
	auto symbol = received_repair.begin();
	for (uint16_t hole = 0; hole < _symbols && symbol != received_repair.end();
																	++hole) {
		if (mask_safe.exists (hole))
			continue;
		const uint16_t row = S_H + hole;
		D.row (row) = symbol->second;
		++symbol;
	}
	// fill the padding symbols (always zero)
	for (uint16_t row = S_H + _symbols; row < K_S_H; ++row) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}
	// fill the remaining (redundant) repair symbols
	for (uint16_t row = K_S_H;
							symbol != received_repair.end(); ++symbol, ++row) {
		D.row (row) = symbol->second;
	}

	// do not lock this part, as it's the expensive part
	lock.unlock();
	std::deque<std::unique_ptr<Operation>> ops;

	DenseMtx missing;
	if (type == Save_Computation::ON) {
		std::vector<uint8_t> compressed = DLF<std::vector<uint8_t>, Cache_Key>::
															get()->get (key);
		LZ4<LZ4_t::DECODER> lz4;
		auto uncompressed = lz4.decode (compressed);
		DenseMtx precomputed = raw_to_Mtx (uncompressed, key._mt_size);
		if (precomputed.rows() != 0) {
			missing = precomputed * D;
			DO_NOT_SAVE = true;
		} else {
			missing = precode_on->intermediate (D, mask_safe, repair_esi, ops);
		}
	} else {
		missing = precode_off->intermediate (D, mask_safe, repair_esi, ops);
	}
	D = DenseMtx();	// free some memory;
	if (type == Save_Computation::ON && !DO_NOT_SAVE) {
		DenseMtx res;
		// don't save really small matrices
		if (missing.rows() != 0 && L_rows > 100) {
			res.setIdentity (L_rows + overhead, L_rows);
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

	if (missing.rows() == 0)
		return mask.get_holes() == 0; // did other threads do something?
	if (mask.get_holes() == 0)
		return true;			// other thread did something

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

	// free some memory, we don't need recover symbols anymore
	received_repair = std::vector<std::pair<uint32_t, Vect>>();
	return true;
}

template <typename In_It>
DenseMtx* Decoder<In_It>::get_symbols()
{
	return &source_symbols;
}

}	//namespace Impl
}	// namespace RaptorQ
