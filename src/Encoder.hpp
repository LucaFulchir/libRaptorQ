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

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Encoder
{
public:
	Encoder (const Interleaver<Rnd_It> &symbols, const uint8_t SBN)
		:precode (Parameters (symbols.source_symbols(SBN))), _symbols(symbols),
		  _SBN(SBN)
	{
		IS_RANDOM(Rnd_It, "RaptorQ::Impl::Encoder");
		IS_FORWARD(Fwd_It, "RaptorQ::Impl::Encoder");
		precode.gen(0);
	}
	uint64_t Enc (const uint32_t ESI, Fwd_It &output, const Fwd_It end) const;

	bool generate_symbols ();
	uint16_t padded() const;
private:
	Precode_Matrix precode;
	const Interleaver<Rnd_It> _symbols;
	const uint8_t _SBN;

	DenseMtx encoded_symbols;
};

//
//
// Implementation
//
//

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::padded () const
{
	return precode._params.K_padded;
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::generate_symbols ()
{
	using T = typename std::iterator_traits<Rnd_It>::value_type;
	// do not bother checing for multithread. that is done in RaptorQ.hpp
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
			for (uint8_t byte = 0; byte < sizeof(T); ++byte)
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
		auto ISI = ESI + (precode._params.K_padded -
												_symbols.source_symbols (_SBN));
		DenseMtx tmp = precode.encode (encoded_symbols, ISI);

		// put "tmp" in output, but the alignment is different

		using T = typename std::iterator_traits<Fwd_It>::value_type;
		T al = static_cast<T> (0);
		uint8_t *p = reinterpret_cast<uint8_t *>  (&al);
		for (ssize_t i = 0; i < tmp.cols(); ++i) {
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
