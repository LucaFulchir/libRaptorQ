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

#ifndef RAPTORQ_ENCODER_HPP
#define RAPTORQ_ENCODER_HPP

#include "common.hpp"
#include "Interleaver.hpp"
#include "multiplication.hpp"
#include "Parameters.hpp"
#include "Precode_Matrix.hpp"
#include "Rand.hpp"
#include "Shared_Computation/Decaying_LF.hpp"
#include <Eigen/Dense>
#include <Eigen/SparseLU>
#include <memory>
#include <iostream>

namespace RaptorQ {
namespace Impl {

extern template class Precode_Matrix<Save_Computation::OFF>;
extern template class Precode_Matrix<Save_Computation::ON>;


template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Encoder
{
public:
	Encoder (const Interleaver<Rnd_It> &symbols, const uint8_t SBN)
		: _SBN(SBN), type (test_computation()),
		  precode_on  (init_precode_on  (symbols.source_symbols(SBN))),
		  precode_off (init_precode_off (symbols.source_symbols(SBN))),
		  _symbols(symbols)
	{
		IS_RANDOM(Rnd_It, "RaptorQ::Impl::Encoder");
		IS_FORWARD(Fwd_It, "RaptorQ::Impl::Encoder");
		if (type == Save_Computation::ON) {
			precode_on->gen (0);
		} else {
			precode_off->gen (0);
		}
	}
	uint64_t Enc (const uint32_t ESI, Fwd_It &output, const Fwd_It end) const;

	bool generate_symbols();

	uint16_t padded() const;
private:
	const uint8_t _SBN;
	const Save_Computation type;
	const std::unique_ptr<Precode_Matrix<Save_Computation::ON>> precode_on;
	const std::unique_ptr<Precode_Matrix<Save_Computation::OFF>> precode_off;
	const Interleaver<Rnd_It> _symbols;

	DenseMtx encoded_symbols;

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

//
//
// Implementation
//
//

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::padded () const
{
	if (type == Save_Computation::ON)
		return precode_on->_params.K_padded;
	return precode_off->_params.K_padded;
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::generate_symbols()
{
	using T = typename std::iterator_traits<Rnd_It>::value_type;
	// do not bother checing for multithread. that is done in RaptorQ.hpp
	if (encoded_symbols.cols() != 0)
		return true;

	uint16_t S_H;
	uint16_t K_S_H;
	if (type == Save_Computation::ON) {
		S_H = precode_on->_params.S + precode_on->_params.H;
		K_S_H = precode_on->_params.K_padded + S_H;
	} else {
		S_H = precode_off->_params.S + precode_off->_params.H;
		K_S_H = precode_off->_params.K_padded + S_H;
	}

	DenseMtx D = DenseMtx (K_S_H, sizeof(T) * _symbols.symbol_size());
	auto C = _symbols[_SBN];

	// fill matrix D: full zero for the first S + H symbols
	uint16_t row;
	for (row = 0; row < S_H; ++row ) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}
	// now the C[0...K] symbols follow
	for (; row < S_H + _symbols.source_symbols (_SBN); ++row) {
		auto symbol = C[row - S_H];
		uint16_t col = 0;
		for (uint16_t i = 0; i < _symbols.symbol_size(); ++i) {
			T val = symbol[i];
			uint8_t *octet = reinterpret_cast<uint8_t *> (&val);
			for (uint8_t byte = 0; byte < sizeof(T); ++byte)
				D (row, col++) = *(octet++);
		}
	}

	// finally fill with eventual padding symbols (K...K_padded)
	for (; row < D.rows(); ++row ) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}

	uint16_t size;
	std::deque<std::unique_ptr<Operation>> ops;
	if (type == Save_Computation::ON) {
		const Cache_Key key (precode_on->_params.K_padded, 0,
														std::vector<bool>());
		std::vector<uint8_t> compressed =DLF<std::vector<uint8_t>, Cache_Key>::
															get()->get (key);
		auto uncompressed = compress_to_raw (compressed);
		DenseMtx precomputed = raw_to_Mtx (uncompressed, key._mt_size);
		if (precomputed.rows() != 0) {
			// we have a precomputed matrix! let's use that!
			encoded_symbols = precomputed * D;
			// result is granted. we only save matrices that work
			return true;
		}
		encoded_symbols = precode_on->intermediate (D, ops, size);
		if (encoded_symbols.cols() == 0)
			return false;
		// RaptorQ succeded.
		// build the precomputed matrix.
		DenseMtx res;
		if (encoded_symbols.cols() != 0) {
			res.setIdentity (size, size);
			for (auto &op : ops)
				op->build_mtx (res);
			const auto raw_mtx = Mtx_to_raw (res);
			auto compressed_mtx = raw_compress (raw_mtx);
			DLF<std::vector<uint8_t>, Cache_Key>::get()->add (compressed_mtx,
																		key);
		}

		// FIXME: useless. I just wanted to understand if it could
		// be compressed.
		uint32_t zeros = 0;
		for (row = 0; row < res.rows(); ++row) {
			for (uint16_t col = 0; col < res.cols(); ++col) {
				if (static_cast<uint8_t> (res (row, col)) == 0)
					++zeros;
			}
		}
		std::cout << "mat: " << size << " = " << size * size << "0: " <<
																zeros << "\n";
	} else {
		encoded_symbols = precode_off->intermediate (D, ops, size);
	}
	return encoded_symbols.cols() != 0;
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Encoder<Rnd_It, Fwd_It>::Enc (const uint32_t ESI, Fwd_It &output,
														const Fwd_It end) const
{
	// ESI means that the first _symbols.source_symbols() are the
	// original symbols, and the next ones are repair symbols.

	// The alignment of "Fwd_It" might *NOT* be the alignment of "Rnd_It"

	uint64_t written = 0;
	auto non_repair = _symbols.source_symbols (_SBN);

	if (ESI < non_repair) {
		// just return the source symbol.
		auto block = _symbols[_SBN];
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
		auto ISI = ESI + (K - _symbols.source_symbols (_SBN));
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
}	// namespace RaptorQ

#endif
