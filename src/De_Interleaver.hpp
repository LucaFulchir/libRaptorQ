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

#ifndef RAPTORQ_DE_INTERLEAVING_HPP
#define RAPTORQ_DE_INTERLEAVING_HPP

#include "common.hpp"
#include "Decoder.hpp"
#include <iostream>

namespace RaptorQ {
namespace Impl {

template <typename Out_It>
class RAPTORQ_LOCAL De_Interleaver
{
public:
	De_Interleaver (const DenseMtx *symbols, const uint16_t sub_blocks)
		:_symbols (symbols), _sub_blocks (sub_blocks)
	{
		IS_OUTPUT(Out_It, "RaptorQ::Impl::De_Interleaver");
	}
	uint64_t operator() (Out_It &start, const Out_It end);
private:
	const DenseMtx *_symbols;
	const uint16_t _sub_blocks;
};

template <typename Out_It>
uint64_t De_Interleaver<Out_It>::operator() (Out_It &start, const Out_It end)
{
	uint64_t written = 0;
	uint32_t byte = 0;
	uint16_t esi = 0;
	uint16_t sub_blk = 0;
	const uint16_t max_esi = static_cast<uint16_t> (_symbols->rows());
	const uint16_t sub_sym_size = static_cast<uint16_t> (_symbols->cols() /
																_sub_blocks);
	uint8_t offset_al = 0;
	using T = typename std::iterator_traits<Out_It>::value_type;
	T al = static_cast<T> (0);
	while (start != end && sub_blk < _sub_blocks) {
		al += static_cast<T> (static_cast<uint8_t> ((*_symbols)(esi, byte)))
															<< offset_al * 8;
		++offset_al;
		if (offset_al >= sizeof(T)) {
			*start = al;
			++start;
			++written;
			al = static_cast<T> (0);
			offset_al = 0;
		}
		++byte;
		if ((byte % sub_sym_size) == 0) {
			++esi;
			if (esi >= max_esi) {
				esi = 0;
				++sub_blk;
			}
			byte = sub_sym_size * sub_blk;
		}
	}
	if (start != end && offset_al != 0) {
		// we have more stuff in "al", but not enough to fill
		// the iterator.
		// Shift the remaining bytes and save it into the iterator
		al <<= (sizeof(T) - offset_al) * 8;
		*start = al;
		++start;
		++written;
	}
	return written;
}

}	// namespace Impl
}	// namespace RaptorQ

#endif
