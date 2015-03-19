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

namespace RaptorQ {
namespace Impl {

template <typename T, typename InputIterator>
class De_Interleaver
{
public:
	De_Interleaver (const DenseMtx *symbols, const uint16_t sub_blocks)
		:interleaved (symbols), _sub_blocks (sub_blocks)
	{}
	uint32_t operator() (InputIterator &start, const InputIterator end);
private:
	const DenseMtx *interleaved;
	const uint16_t _sub_blocks;
};

template <typename T, typename InputIterator>
uint32_t De_Interleaver<T, InputIterator>::operator() (InputIterator &start,
													const InputIterator end)
{
	uint32_t written = 0;
	uint32_t byte = 0;
	uint16_t esi = 0;
	uint16_t sub_symbol = 0;
	const uint16_t max_esi = interleaved->rows();
	const uint16_t sub_sym_size = interleaved->cols() / _sub_blocks;
	uint8_t offset_al = 0;
	T al = 0;
	while (start != end && sub_symbol < max_esi * _sub_blocks) {
		al += static_cast<T> ((*interleaved) (esi, byte)) << offset_al * 8;
		++offset_al;
		if (offset_al >= sizeof(T)) {
			*start = al;
			++start;
			++written;
			al = 0;
			offset_al = 0;
		}
		++byte;
		if ((byte % sub_sym_size) == 0) {
			++esi;
			if (esi >= max_esi) {
				esi = 0;
				++sub_symbol;
			}
			byte = sub_sym_size * sub_symbol;
		}
	}
	return written;
}

}	// namespace Impl
}	//namespace RaptorQ

#endif

