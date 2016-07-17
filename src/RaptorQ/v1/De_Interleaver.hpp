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
#include "RaptorQ/v1/Decoder.hpp"
#include "RaptorQ/v1/Interleaver.hpp"

namespace RFC6330__v1 {
namespace Impl {

template <typename Fwd_It>
class RAPTORQ_LOCAL De_Interleaver
{
public:
	De_Interleaver (const RaptorQ__v1::Impl::DenseMtx *symbols,
													const Partition sub_blocks,
													const uint8_t alignment)
		:_symbols (symbols), _sub_blocks (sub_blocks), _al (alignment)
	{
		IS_FORWARD(Fwd_It, "RaptorQ__v1::Impl::De_Interleaver");
	}
	uint64_t operator() (Fwd_It &start, const Fwd_It end,
								const uint64_t max_bytes, const uint8_t skip);
private:
	const RaptorQ__v1::Impl::DenseMtx *_symbols;
	const Partition _sub_blocks;
	const uint8_t _al;
};

template <typename Fwd_It>
uint64_t De_Interleaver<Fwd_It>::operator() (Fwd_It &start, const Fwd_It end,
													const uint64_t max_bytes,
													const uint8_t skip)
{
	// return number of BYTES written
	uint64_t written = 0;
	uint32_t byte = 0;
	uint32_t subsym_byte = 0;
	uint16_t esi = 0;
	uint16_t sub_blk = 0;
	const uint16_t max_esi = static_cast<uint16_t> (_symbols->rows());
	uint16_t sub_sym_size = _al *(_sub_blocks.num(0) > 0 ? _sub_blocks.size(0) :
													  _sub_blocks.size(1));
	// if the Fwd_It::value_type is not aligned with the block size,
	// we need to skip a certain amount of data in the first output element
	// so that we do not overwrite the old data or add unnecessary zeros
	// to (start-1) during the decoding of the previous block.
	uint8_t offset_al = skip;
	using T = typename std::iterator_traits<Fwd_It>::value_type;
	T element = static_cast<T> (0);
	if (skip != 0) {
		uint8_t *p = reinterpret_cast<uint8_t *> (&*start);
		for (size_t keep = 0; keep < skip; ++keep) {
			element += static_cast<T> (*(p++)) << keep * 8;
		}
	}
	while (start != end && sub_blk < (_sub_blocks.num(0) + _sub_blocks.num(1))){
		if ((written + offset_al) >= max_bytes)
			break;
		element += static_cast<T> (static_cast<uint8_t>((*_symbols)(esi, byte)))
															<< offset_al * 8;
		++offset_al;
		if (offset_al == sizeof(T)) {
			*start = element;
			++start;
			written += offset_al;
			element = static_cast<T> (0);
			offset_al = 0;
		}
		++byte;
		++subsym_byte;
		if (subsym_byte == sub_sym_size) {
			subsym_byte = 0;
			++esi;
			if (esi >= max_esi) {
				esi = 0;
				++sub_blk;
			}
			if (sub_blk < _sub_blocks.num (0)) {
				sub_sym_size = _sub_blocks.size (0) * _al;
				byte = sub_sym_size * sub_blk;
			} else {
				sub_sym_size = _sub_blocks.size (1) * _al;
				byte = _sub_blocks.tot (0) * _al +
								sub_sym_size * (sub_blk - _sub_blocks.num (0));
			}
		}
	}
	if (start != end && offset_al != 0) {
		// we have more stuff in "element", but not enough to fill
		// the iterator.
		*start = element;
		++start;
		written += offset_al;
	}
	if (written > 0)
		written -= skip;
	return written;
}

}	// namespace Impl
}	// namespace RFC6330__v1
