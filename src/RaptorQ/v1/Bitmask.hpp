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

#pragma once

#include "RaptorQ/v1/common.hpp"
#include <vector>

namespace RaptorQ__v1 {
namespace Impl {

// track the bitmask of holes for received symbols.
// also make it easy to know how many non-repair symbols we are missing.
class RAPTORQ_API Bitmask
{
public:
	const uint16_t _max_nonrepair;

	Bitmask (const uint16_t symbols);

	void add (const size_t id);
	void drop (const size_t id);
	bool exists (const size_t id) const;
	uint16_t get_holes () const;
	void free();
private:
	// NOTE: this is not just vector<bool> 'cause this gives us a bit more
	// breating space and less reallocations, but really, why don't we just
	// use vector<bool> with more allocated space?
	std::vector<size_t> mask;
	uint16_t holes;
};

} // namespace Impl
} // namespace RaptorQ__v1
