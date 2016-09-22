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

#include "RaptorQ/v1/common.hpp"
#include "RaptorQ/v1/caches.hpp"
#include "RaptorQ/v1/Interleaver.hpp"
#include "RaptorQ/v1/multiplication.hpp"
#include "RaptorQ/v1/Parameters.hpp"
#include "RaptorQ/v1/Precode_Matrix.hpp"
#include "RaptorQ/v1/Rand.hpp"
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"
#include "RaptorQ/v1/Thread_Pool.hpp"
#include <Eigen/Dense>
#include <Eigen/SparseLU>
#include <memory>

namespace RaptorQ__v1 {
namespace Impl {

extern template class Precode_Matrix<Save_Computation::OFF>;
extern template class Precode_Matrix<Save_Computation::ON>;

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_LOCAL Raw_Encoder
{
public:
	Raw_Encoder (const uint16_t symbols)
		: _SBN(0), type (Save_Computation::ON),
		  precode_on  (init_precode_on (symbols)),
		  precode_off (nullptr),
		  _symbols (nullptr)
	{
		IS_RANDOM(Rnd_It, "RaptorQ__v1::Impl::Encoder");
		IS_FORWARD(Fwd_It, "RaptorQ__v1::Impl::Encoder");
		precode_on->gen(0);
		keep_working = true;
	}
	Raw_Encoder (RFC6330__v1::Impl::Interleaver<Rnd_It> *symbols,
															const uint8_t SBN)
		: _SBN(SBN), type (test_computation()),
		  precode_on  (init_precode_on  (symbols->source_symbols(SBN))),
		  precode_off (init_precode_off (symbols->source_symbols(SBN))),
		  _symbols (symbols)
	{
		IS_RANDOM(Rnd_It, "RaptorQ__v1::Impl::Encoder");
		IS_FORWARD(Fwd_It, "RaptorQ__v1::Impl::Encoder");
		assert (symbols != nullptr);
		if (type == Save_Computation::ON) {
			precode_on->gen (0);
		} else {
			precode_off->gen (0);
		}
		keep_working = true;
	}
	~Raw_Encoder();

	uint64_t Enc (const uint32_t ESI, Fwd_It &output, const Fwd_It end) const;


	DenseMtx get_precomputed (RaptorQ__v1::Work_State *thread_keep_working);
	bool generate_symbols (const DenseMtx &precomputed,
							RFC6330__v1::Impl::Interleaver<Rnd_It> *symbols);
	bool generate_symbols (RaptorQ__v1::Work_State *thread_keep_working);
	void stop();
	void clear_data();
	bool ready() const;

private:
	const uint8_t _SBN;
	const Save_Computation type;
	bool  keep_working;
	const std::unique_ptr<Precode_Matrix<Save_Computation::ON>> precode_on;
	const std::unique_ptr<Precode_Matrix<Save_Computation::OFF>> precode_off;
	RFC6330__v1::Impl::Interleaver<Rnd_It> *_symbols;

	DenseMtx encoded_symbols;

	DenseMtx get_raw_symbols (const uint16_t K_S_H, const uint16_t S_H) const;
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
		if (type == Save_Computation::ON)
			return new Precode_Matrix<Save_Computation::ON> (
														Parameters(symbols));
		return nullptr;
	}
	// to help making things const
	Precode_Matrix<Save_Computation::OFF> *init_precode_off (
												const uint16_t symbols) const
	{
		if (type == Save_Computation::OFF)
			return new Precode_Matrix<Save_Computation::OFF> (
														Parameters(symbols));
		return nullptr;
	}
};

//
//
// Implementation
//
//

template <typename Rnd_It, typename Fwd_It>
Raw_Encoder<Rnd_It, Fwd_It>::~Raw_Encoder()
{
	stop();
}

template <typename Rnd_It, typename Fwd_It>
void Raw_Encoder<Rnd_It, Fwd_It>::stop()
{
	keep_working = false;
}

template <typename Rnd_It, typename Fwd_It>
void Raw_Encoder<Rnd_It, Fwd_It>::clear_data()
{
	encoded_symbols = DenseMtx();
	_symbols = nullptr;
}

template <typename Rnd_It, typename Fwd_It>
bool Raw_Encoder<Rnd_It, Fwd_It>::ready() const
{
	return encoded_symbols.cols() != 0;
}

template <typename Rnd_It, typename Fwd_It>
DenseMtx Raw_Encoder<Rnd_It, Fwd_It>::get_precomputed (
								RaptorQ__v1::Work_State *thread_keep_working)
{
	if (_symbols != nullptr)
		return DenseMtx();
	keep_working = true;

	if (type == Save_Computation::ON) {
		const uint16_t size = precode_on->_params.L;
		const Cache_Key key (size, 0, std::vector<bool>());
		auto compressed = DLF<std::vector<uint8_t>, Cache_Key>::
															get()->get (key);
		if (compressed.second.size() != 0) {
			auto uncompressed = decompress (compressed.first,compressed.second);
			DenseMtx precomputed = raw_to_Mtx (uncompressed, key._mt_size);
			if (precomputed.rows() != 0) {
				return precomputed;
			}
			return DenseMtx();
		}
		// else not found, generate one.
	}

	uint16_t S_H;
	uint16_t K_S_H;
	S_H = precode_on->_params.S + precode_on->_params.H;
	K_S_H = precode_on->_params.K_padded + S_H;

	// we only want the precomputex matrix.
	// we can generate that without input data.
	// each symbol now has size '1' and val '0'
	DenseMtx D;
	D.setZero (K_S_H, 1);

	Precode_Result precode_res;
	std::deque<std::unique_ptr<Operation>> ops;
	DenseMtx encoded_no_symbols;
	std::tie (precode_res, encoded_no_symbols) = precode_on->intermediate (D,
														ops, keep_working,
														thread_keep_working);
	if (precode_res != Precode_Result::DONE || encoded_no_symbols.cols() == 0)
		return DenseMtx();

	// RaptorQ succeded.
	// build the precomputed matrix.
	DenseMtx res;
	// don't save really small matrices.

	const uint16_t size = precode_on->_params.L;
	const Cache_Key key (size, 0, std::vector<bool>());
	res.setIdentity (size, size);
	for (auto &op : ops)
		op->build_mtx (res);
	if (type == Save_Computation::ON) {
		auto raw_mtx = Mtx_to_raw (res);
		auto compressed = compress (raw_mtx);
		DLF<std::vector<uint8_t>, Cache_Key>::get()->add (compressed.first,
														compressed.second, key);
	}
	return res;
}

template <typename Rnd_It, typename Fwd_It>
bool Raw_Encoder<Rnd_It, Fwd_It>::generate_symbols (const DenseMtx &precomputed,
						RFC6330__v1::Impl::Interleaver<Rnd_It> *symbols)
{
	if (precomputed.rows() == 0 || symbols == nullptr)
		return false;
	const uint16_t S_H = precode_on->_params.S + precode_on->_params.H;
	const uint16_t K_S_H = precode_on->_params.K_padded + S_H;
	_symbols = symbols;
	const DenseMtx D = get_raw_symbols (K_S_H, S_H);
	encoded_symbols = precomputed * D;
	return true;
}

template <typename Rnd_It, typename Fwd_It>
DenseMtx Raw_Encoder<Rnd_It, Fwd_It>::get_raw_symbols(const uint16_t K_S_H,
													const uint16_t S_H) const
{
	using T = typename std::iterator_traits<Rnd_It>::value_type;
	assert (_symbols != nullptr);

	DenseMtx D = DenseMtx (K_S_H, sizeof(T) * _symbols->symbol_size());
	auto C = (*_symbols)[_SBN];

	// fill matrix D: full zero for the first S + H symbols
	D.block (0, 0, S_H, D.cols()).setZero();
	uint16_t row = S_H;
	// now the C[0...K] symbols follow
	for (; row < S_H + _symbols->source_symbols (_SBN); ++row) {
		auto symbol = C[row - S_H];
		uint16_t col = 0;
		for (uint16_t i = 0; i < _symbols->symbol_size(); ++i) {
			T val = symbol[i];
			uint8_t *octet = reinterpret_cast<uint8_t *> (&val);
			for (uint8_t byte = 0; byte < sizeof(T); ++byte)
				D (row, col++) = *(octet++);
		}
	}

	// finally fill with eventual padding symbols (K...K_padded)
	D.block (row, 0, D.rows() - row, D.cols()).setZero();
	return D;
}


template <typename Rnd_It, typename Fwd_It>
bool Raw_Encoder<Rnd_It, Fwd_It>::generate_symbols (
								RaptorQ__v1::Work_State *thread_keep_working)
{
	if (_symbols == nullptr)
		return false;
	// do not bother checking for multithread. that is done in RaptorQ.hpp
	if (encoded_symbols.cols() != 0)
		return true;

	keep_working = true;

	uint16_t S_H;
	uint16_t K_S_H;
	if (type == Save_Computation::ON) {
		S_H = precode_on->_params.S + precode_on->_params.H;
		K_S_H = precode_on->_params.K_padded + S_H;
	} else {
		S_H = precode_off->_params.S + precode_off->_params.H;
		K_S_H = precode_off->_params.K_padded + S_H;
	}

	DenseMtx D = get_raw_symbols (K_S_H, S_H);

	Precode_Result precode_res;
	std::deque<std::unique_ptr<Operation>> ops;
	if (type == Save_Computation::ON) {
		const uint16_t size = precode_on->_params.L;
		const Cache_Key key (size, 0, std::vector<bool>());
		auto compressed = DLF<std::vector<uint8_t>, Cache_Key>::
															get()->get (key);
		if (compressed.second.size() != 0) {
			auto decompressed = decompress (compressed.first,compressed.second);
			DenseMtx precomputed = raw_to_Mtx (decompressed, key._mt_size);
			if (precomputed.rows() != 0) {
				// we have a precomputed matrix! let's use that!
				encoded_symbols = precomputed * D;
				// result is granted. we only save matrices that work
				return true;
			}
			return false;
		}
		std::tie (precode_res, encoded_symbols) = precode_on->intermediate (D,
														ops, keep_working,
														thread_keep_working);
		if (precode_res != Precode_Result::DONE || encoded_symbols.cols() == 0)
			return false;

		// RaptorQ succeded.
		// build the precomputed matrix.
		DenseMtx res;
		// don't save  really small matrices.
		if (encoded_symbols.cols() != 0 && size > 100) {
			res.setIdentity (size, size);
			for (auto &op : ops)
				op->build_mtx (res);
			auto raw_mtx = Mtx_to_raw (res);
			compressed = compress (raw_mtx);
			DLF<std::vector<uint8_t>, Cache_Key>::get()->add (compressed.first,
														compressed.second, key);
		}
	} else {
		std::tie (precode_res, encoded_symbols) = precode_off->intermediate (D,
														ops, keep_working,
														thread_keep_working);
	}
	return (Precode_Result::DONE == precode_res) && 0 != encoded_symbols.cols();
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Raw_Encoder<Rnd_It, Fwd_It>::Enc (const uint32_t ESI, Fwd_It &output,
														const Fwd_It end) const
{
	// returns iterators written
	// ESI means that the first _symbols.source_symbols() are the
	// original symbols, and the next ones are repair symbols.

	// The alignment of "Fwd_It" might *NOT* be the alignment of "Rnd_It"

	uint64_t written = 0;
	if (_symbols == nullptr || !ready())
		return written;
	auto non_repair = _symbols->source_symbols (_SBN);

	if (ESI < non_repair) {
		// just return the source symbol.
		auto block = (*_symbols)[_SBN];
		auto requested_symbol = block[static_cast<uint16_t> (ESI)];

		typedef typename std::iterator_traits<Fwd_It>::value_type out_al;
		size_t byte = 0;
		out_al tmp_out = 0;
		for (auto al : requested_symbol) {
			uint8_t *p;
			for (p = reinterpret_cast<uint8_t *> (&al);
							p != reinterpret_cast<uint8_t *>(&al) + sizeof(al);
																		++p) {
				tmp_out += static_cast<out_al> (*p) << (byte * 8);
				++byte;
				if (byte % sizeof(out_al) == 0) {
					*(output++) = tmp_out;
					++written;
					byte = 0;
					tmp_out = 0;
					if (output == end)
						return written;
				}
			}
		}
		if (byte % sizeof(out_al) != 0) {
			*(output++) = tmp_out;
			++written;
		}
	} else {
		// repair symbol requested.
		if (encoded_symbols.cols() == 0)
			return false;
		uint16_t K;
		if (type == Save_Computation::ON) {
			K = precode_on->_params.K_padded;
		} else {
			K = precode_off->_params.K_padded;
		}
		auto ISI = ESI + (K - _symbols->source_symbols (_SBN));
		DenseMtx tmp;
		if (type == Save_Computation::ON) {
			tmp = precode_on->encode (encoded_symbols, ISI);
		} else {
			tmp = precode_off->encode (encoded_symbols, ISI);
		}

		// put "tmp" in output, but the alignment is different

		using T = typename std::iterator_traits<Fwd_It>::value_type;
		T al = static_cast<T> (0);
		uint8_t *p = reinterpret_cast<uint8_t *>  (&al);
		for (uint32_t i = 0; i < tmp.cols(); ++i) {
			*p = static_cast<uint8_t> (tmp (0, i));
			++p;
			if (p == reinterpret_cast<uint8_t *>  (&al) + sizeof(T)) {
				*output = al;
				++output;
				al = static_cast<T> (0);
				p = reinterpret_cast<uint8_t *>  (&al);
				++written;
				if (output == end)
					return written;
			}
		}
		if (p != reinterpret_cast<uint8_t *>  (&al) + sizeof(T)) {
			// symbol size is not aligned with Fwd_It type
			while (p != reinterpret_cast<uint8_t *>  (&al) + sizeof(T))
				*(p++) = 0;
			*output = al;
			++output;
			++written;
		}
	}
	return written;
}

}	// namespace Impl
}	// namespace RFC6330__v1
