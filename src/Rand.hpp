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

/* RFC, section 5.5:  Random Numbers
 *
 * The four arrays V0, V1, V2, and V3 used in Section 5.3.5.1 are
 * provided below.  There are 256 entries in each of the four arrays.
 * The indexing into each array starts at 0, and the entries are 32-bit
 * unsigned integers.
 */

#ifndef RAPTORQ_RAND_H
#define RAPTORQ_RAND_H

#include <array>
#include "common.hpp"

namespace RaptorQ {
namespace Impl {

class RAPTORQ_LOCAL Rand {
public:
	Rand() {}
	Rand (const Rand&) = delete; // Don't Implement
	Rand& operator= (const Rand&) = delete;// Don't implement
	Rand (Rand &&) = delete; // Don't Implement
	Rand& operator= (Rand &&) = delete;// Don't implement

	static uint32_t get (const uint32_t y, const uint8_t i, const uint32_t m);
private:
	static const std::array<uint32_t, 256> V0, V1, V2, V3;
};

}	// namespace Impl
}	// namespace RaptorQ

#endif
