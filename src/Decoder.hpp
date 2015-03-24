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

#ifndef RAPTORQ_DECODER_HPP
#define RAPTORQ_DECODER_HPP

#include "common.hpp"
#include "Parameters.hpp"
#include "Precode_Matrix.hpp"
#include "Graph.hpp"
#include <cassert>
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>
#include <vector>
#include <Eigen/Dense>

#include <iostream>

namespace RaptorQ {
namespace Impl {

template <typename T>
using Symbol = std::vector<T>;

template <typename T>
class RAPTORQ_LOCAL Decoder
{
public:
	Decoder (const uint16_t symbols, const uint16_t symbol_size)
		:_symbols (symbols), _symbol_size (symbol_size / sizeof(T)),
				precode (Parameters(symbols)), mask (_symbols)
	{
		static_assert(std::is_unsigned<T>::value,
					"RaptorQ: Decoder can only be used with unsigned types");

		// symbol size is in octets, but we save it in "T" sizes.
		// so be aware that "symbol_size" != "_symbol_size" for now
		source_symbols = DenseMtx (_symbols, _symbol_size * sizeof(T));
	}

	bool add_symbol (const uint32_t esi, const std::vector<T> &symbol);
	bool decode ();
	DenseMtx* get_symbols();
	std::vector<T> get (const uint16_t symbol) const;

private:
	std::mutex lock;
	const uint16_t _symbols, _symbol_size;
	Precode_Matrix precode;
	Bitmask mask;
	DenseMtx source_symbols;
	std::vector<std::pair<uint32_t, std::vector<T>>> received_repair;
};


///////////////////////////////////
//
// IMPLEMENTATION OF ABOVE TEMPLATE
//
///////////////////////////////////


template <typename T>
bool Decoder<T>::add_symbol (const uint32_t esi, const std::vector<T> &symbol)
{
	// true if added succesfully

	if (symbol.size() != _symbol_size || esi >= std::pow (2, 20))
		return false;


	std::lock_guard<std::mutex> guard (lock);
	UNUSED(guard);

	if (mask.get_holes() == 0)
		return false;	// not even needed;
	if (esi < _symbols) {
		if (mask.exists(esi))
			return false;	// already present.
		uint16_t col = 0;
		for (T al : symbol) {
			for (uint8_t *p = reinterpret_cast<uint8_t *> (&al);
					p != reinterpret_cast<uint8_t *> (&al) + sizeof(T); ++p) {
				source_symbols (esi, col++) = *p;
			}
		}
		mask.add (esi);
	} else {
		// FIXME: do *NOT* ad it twice, even if asked.
		received_repair.emplace_back (esi, symbol);
	}

	return true;
}

template <typename T>
bool Decoder<T>::decode ()
{
	// rfc 6330: can decode when received >= K_padded
	// actually: (K_padded - K) are padding and thus constant and NOT
	// transmitted. so really N >= K.
	if (mask.get_holes() == 0)
		return true;

	if (received_repair.size() < mask.get_holes())
		return false;

	precode.gen (received_repair.size() - mask.get_holes());


	DenseMtx D = DenseMtx (precode._params.S + precode._params.H +
								precode._params.K_padded +
								(received_repair.size() - mask.get_holes()),
														source_symbols.cols());

	// initialize D
	for (uint16_t row = 0; row < precode._params.S + precode._params.H; ++row) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}
	// put non-repair symbols (source symbols) in place
	lock.lock();
	if (mask.get_holes() == 0)	// other thread completed its work before us?
		return true;
	D.block (precode._params.S + precode._params.H, 0,
							source_symbols.rows(), D.cols()) = source_symbols;

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
		uint8_t col = 0;
		const int16_t row = precode._params.S + precode._params.H + hole;
		for (T al : symbol->second) {
			for (uint8_t *p = reinterpret_cast<uint8_t *> (&al);
							p != reinterpret_cast<uint8_t *> (&al) + sizeof(T);
																++p, ++col) {
				D (row, col) = *p;
			}
		}
		++symbol;
	}
	// fill the padding symbols (always zero)
	for (uint16_t row = precode._params.S + precode._params.H + _symbols;
								row < precode._params.S + precode._params.H +
											precode._params.K_padded; ++row) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}
	// fill the remaining (redundant) repair symbols
	for (uint16_t row =
			precode._params.S + precode._params.H + precode._params.K_padded;
							symbol != received_repair.end(); ++symbol, ++row) {
		uint8_t col = 0;
		for (T al : symbol->second) {
			for (uint8_t *p = reinterpret_cast<uint8_t *> (&al);
							p != reinterpret_cast<uint8_t *> (&al) + sizeof(T);
																++p, ++col) {
				D (row, col) = *p;
			}
		}
	}
	lock.unlock();

	// do not lock this part, as it's the expensive part
	precode.intermediate (D, mask_safe, repair_esi);

	if (D.rows() == 0)
		return mask.get_holes() == 0; // other threads did something?

	std::lock_guard<std::mutex> guard (lock);
	UNUSED(guard);

	if (mask.get_holes() == 0)
		return true;			// other thread did something))

	// put missing symbols into "source_symbols".
	// remember: we might have received other symbols while decoding.
	uint16_t D_row = 0;
	for (uint16_t row = 0; row < mask._max; ++row) {
		if (mask_safe.exists (row))
			continue;
		++D_row;
		if (mask.exists (row))
			continue;
		source_symbols.row (row) = D.row (D_row - 1);
		mask.add (row);
	}

	if (received_repair.capacity() != 0) {
		// free some memory, we don't need recover symbols anymore
		received_repair = std::vector<std::pair<uint32_t, std::vector<T>>>();
	}

	return true;
}

template <typename T>
std::vector<T> Decoder<T>::get (const uint16_t symbol) const
{
	std::vector<T> ret;
	if (mask.get_holes() != 0 || symbol >= source_symbols.rows())
		return ret;

	for (uint16_t col = 0; col < source_symbols.cols(); ++col) {
		T al;
		for (uint8_t *p = reinterpret_cast<uint8_t *> (&al);
					p != reinterpret_cast<uint8_t *> (&al) + sizeof(T); ++p) {
			*p = static_cast<uint8_t> (source_symbols (symbol, col));
			++col;
		}
		--col;
		ret.push_back (al);
	}
	return ret;
}

template <typename T>
DenseMtx* Decoder<T>::get_symbols()
{
	return &source_symbols;
}

}	//namespace Impl
}	// namespace RaptorQ

#endif
