/*
 * Copyright (c) 2015, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#include "Parameters.hpp"
#include "Precode_Matrix.hpp"
#include "Interleaver.hpp"
#include "Rand.hpp"
#include "multiplication.hpp"
#include <Eigen/Dense>
#include <Eigen/SparseLU>

namespace RaptorQ {
namespace Impl {

template <typename T>
class RAPTORQ_API Encoder
{
public:
	Encoder (const Interleaver<T> &symbols, const uint8_t SBN)
		:_symbols(symbols), precode (Parameters (symbols.source_symbols(SBN))),
		  _SBN(SBN)
	{
		precode.gen(0);
	}
	bool Enc (const uint32_t ESI, std::vector<T> &output,
												const size_t offset = 0) const;

	bool generate_symbols ();
	uint16_t padded() const;
private:
	Precode_Matrix precode;
	const Interleaver<T> _symbols;
	const uint8_t _SBN;

	DenseMtx encoded_symbols;
};

//
//
// Implementation
//
//

template <typename T>
uint16_t Encoder<T>::padded () const
{
	return precode._params.K_padded;
}

template <typename T>
bool Encoder<T>::generate_symbols ()
{
	// do not obther checing for multithread. that is done in RaptorQ.hpp
	if (encoded_symbols.cols() != 0)
		return true;
	DenseMtx D = DenseMtx (precode._params.K_padded + precode._params.S +
															precode._params.H,
											sizeof(T) * _symbols.symbol_size());
	auto C = _symbols[_SBN];

	// fill matrix D: full zero for the first S + H symbols
	uint16_t row;
	for (row = 0; row < precode._params.S + precode._params.H; ++row ) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}
	// now the C[0...K] symbols follow
	for (; row < precode._params.S + precode._params.H +
										_symbols.source_symbols (_SBN); ++row) {
		auto symbol = C[row - (precode._params.S + precode._params.H)];
		uint16_t col = 0;
		for (uint16_t i = 0; i < _symbols.symbol_size(); ++i) {
			T val = symbol[i];
			uint8_t *octet = reinterpret_cast<uint8_t *> (&val);
			for (uint8_t i = 0; i < sizeof(T); ++i)
				D (row, col++) = *(octet++);
		}
	}

	// finally fill with eventual padding symbols (K...K_padded)
	for (; row < D.rows(); ++row ) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}

	encoded_symbols = precode.intermediate (D);
	return encoded_symbols.cols() != 0;
}

template <typename T>
bool Encoder<T>::Enc (const uint32_t ESI, std::vector<T> &output,
													const size_t offset) const
{
	// ESI means that the first _symbols.source_symbols() are the
	// original symbols, and the next ones are repair symbols.

	if (output.capacity() < offset + _symbols.symbol_size())
		output.reserve (offset + _symbols.symbol_size ());

	auto non_repair = _symbols.source_symbols (_SBN);

	auto position = output.begin() + offset;

	if (ESI < non_repair) {
		// just return the source symbol.
		auto block = _symbols[_SBN];
		auto requested_symbol = block[ESI];

		for (auto al = requested_symbol.begin(); al != requested_symbol.end();
																		++al) {
			output.insert (position++, *al);
		}
	} else {
		// repair symbol requested.
		if (encoded_symbols.cols() == 0)
			return false;
		auto ISI = ESI + (precode._params.K_padded -
												_symbols.source_symbols (_SBN));
		DenseMtx tmp = precode.encode (encoded_symbols, ISI);

		// put "tmp" in "ret", but "tmp" is aligned to "uint8_t",
		// while "ret" is aligned to "T"
		for (size_t i = 0; i < tmp.cols(); ++i) {
			T al;
			for (uint8_t *p = reinterpret_cast<uint8_t *> (&al);
							p != reinterpret_cast<uint8_t *> (&al) + sizeof(T);
																	++p, ++i) {
				*p = static_cast<uint8_t> (tmp (0, i));
			}
			--i;
			output.insert (position++, al);
		}
	}
	return true;
}

}	// namespace Impl
}	// namespace RaptorQ

#endif
