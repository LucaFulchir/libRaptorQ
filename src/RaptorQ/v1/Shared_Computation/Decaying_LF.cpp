/*
 * Copyright (c) 2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"
#include <cmath>


namespace RaptorQ__v1 {
namespace Impl {


// easy compress from/to eigen matrix
std::vector<uint8_t> Mtx_to_raw (const DenseMtx &mtx)
{
	std::vector<uint8_t> ret;
	ret.reserve (static_cast<size_t> (mtx.rows() * mtx.cols()));
	for (uint32_t row = 0; row < mtx.rows(); ++row) {
		for (uint32_t col = 0; col < mtx.cols(); ++col)
			ret.emplace_back (static_cast<uint8_t> (mtx (row, col)));
	}
	return ret;
}

DenseMtx raw_to_Mtx (const std::vector<uint8_t> &raw, const uint16_t cols)
{
	uint16_t rows = static_cast<uint16_t> (raw.size() / cols);
	DenseMtx ret (rows, cols);
	auto raw_it = raw.begin();
	for (uint32_t row = 0; row < rows; ++row) {
		for (uint32_t col = 0; col < cols; ++col, ++raw_it)
			ret (row, col) = *raw_it;
	}
	return ret;
}

bool Cache_Key::operator< (const Cache_Key &rhs) const
{
	if (_mt_size < rhs._mt_size)
		return true;
	if (_mt_size == rhs._mt_size) {
		if (_lost < rhs._lost)
			return true;
		if (_lost == rhs._lost) {
			if (bitmask.size() < rhs.bitmask.size())
				return true;
			if (bitmask.size() == rhs.bitmask.size()) {
				int32_t idx = static_cast<int32_t> (bitmask.size() - 1);
				for (; idx >= 0; --idx) {
					uint32_t i = static_cast<uint32_t> (idx);
					if (rhs.bitmask[i] == false &&
							bitmask[i] == true) {
						return false;
					}
					if (rhs.bitmask[i] == true &&
							bitmask[i] == false) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool Cache_Key::operator== (const Cache_Key &rhs) const
{
	return _mt_size == rhs._mt_size && _lost == rhs._lost &&
				bitmask.size() == rhs.bitmask.size() && bitmask == rhs.bitmask;
}


} // namespace Impl
} // namespace RaptorQ__v1
