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

#include "RaptorQ/v1/Precode_Matrix.hpp"


// force promotion to float in division
namespace {
float RAPTORQ_LOCAL div_floor (const float a, const float b);
float RAPTORQ_LOCAL div_ceil (const float a, const float b);

float div_floor (const float a, const float b)
{
	return std::floor (a / b);
}
float div_ceil (const float a, const float b)
{
	return std::ceil (a / b);
}
}

///////////////////
//
// Bitmask
//
///////////////////

namespace RaptorQ__v1 {
namespace Impl {

Bitmask::Bitmask (const uint16_t symbols)
	: _max_nonrepair (symbols)
{
	holes = _max_nonrepair;
	size_t max_element = static_cast<size_t> (div_ceil (_max_nonrepair,
															sizeof(size_t)));
	mask.reserve (max_element + 1);
	for (size_t i = 0; i <= max_element; ++i)
		mask.push_back (0);
}

void Bitmask::free()
{
	mask.clear();
	mask.resize (0);
	holes = 0;
}

void Bitmask::add (const size_t id)
{
	size_t element = static_cast<size_t> (div_floor (id, sizeof(size_t)));
	while (element >= mask.size())
		mask.push_back(0);
	if (exists(id))
		return;

	size_t add_mask = 1 << (id - (element * sizeof(size_t)));
	mask[element] |= add_mask;
	if (id < _max_nonrepair)
		--holes;
}
void Bitmask::drop (const size_t id)
{
	size_t element = static_cast<size_t> (div_floor (id, sizeof(size_t)));
	if (element >= mask.size())
		return;
	if (!exists(id))
		return;

	size_t drop_mask = 1 << (id - (element * sizeof(size_t)));
	mask[element] &= ~drop_mask;
	if (id < _max_nonrepair)
		++holes;
}


bool Bitmask::exists (const size_t id) const
{
	size_t element = static_cast<size_t> (div_floor (id, sizeof(size_t)));
	if (element >= mask.size())
		return false;

	size_t check_mask = 1 << (id - (element * sizeof(size_t)));
	return (mask[element] & check_mask) != 0;
}

uint16_t Bitmask::get_holes () const
{
	return holes;
}

}	// namespace RaptorQ
}	// namespace Impl
