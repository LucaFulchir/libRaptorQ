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

#ifndef RAPTORQ_PARAMETERS_HPP
#define RAPTORQ_PARAMETERS_HPP

#include "common.hpp"
#include "multiplication.hpp"
#include "table2.hpp"
#include <cmath>
#include <Eigen/Core>
#include <vector>

//
// This implements both phase 1 and phase 2 of the encoding
// algorithm. Phase2 was so small it made no sense splitting them.
//

namespace RaptorQ {
namespace Impl {

class RAPTORQ_LOCAL Tuple
{
	// d  1-30	LT-degree of encoded symbol
	// a  0-(W-1)
	// b  0-(W-1)
	// d1 PI-degree of encoded symbol (2 or 3)
	// a1 0-(P1-1)
	// b1 0-(P1-1)
public:
	uint16_t d, a, b, d1, a1, b1;	// great names. thanks rfc6330!
};

class RAPTORQ_API Parameters
{
public:
	explicit Parameters (const uint16_t symbols);
	uint16_t Deg (const uint32_t v) const;
	Tuple tuple (const uint32_t ISI) const;
	std::vector<uint16_t> get_idxs (const uint32_t ISI) const;

	uint16_t K_padded, S, H, W, L, P, P1, U, B; // RFC 6330, pg 22
	uint16_t J;
private:
	static bool is_prime (const uint16_t n);
};

class RAPTORQ_LOCAL Octet
{
public:
	Octet () {}
	Octet (const uint8_t val) : data(val) {}

	explicit operator uint8_t() const { return data; }

	Octet& operator-= (const Octet a)
	{
		data ^= a.data;
		return *this;
	}
	friend Octet operator- (Octet lhs, const Octet rhs)
	{
		lhs.data ^= rhs.data;
		return lhs;
	}
	Octet& operator+= (const Octet a)
	{
		data ^= a.data;
		return *this;
	}
	friend Octet operator+ (Octet lhs, const Octet rhs)
	{
		lhs.data ^= rhs.data;
		return lhs;
	}
	// xor, addition, subtraction... they're the same to me...
	Octet& operator^= (const Octet &a)
	{
		data ^= a.data;
		return *this;
	}
	friend Octet operator^ (Octet lhs, const Octet rhs)
	{
		lhs.data ^= rhs.data;
		return lhs;
	}
	Octet& operator*= (const Octet a)
	{
		if (data != 0 && a.data != 0) {
			data = RaptorQ::Impl::oct_exp[oct_log[data - 1] +
											oct_log[a.data - 1]];
		} else {
			data = 0;
		}
		return *this;
	}
	friend Octet operator* (Octet lhs, const Octet rhs)
	{
		if (lhs.data != 0 && rhs.data != 0) {
			lhs.data = RaptorQ::Impl::oct_exp[oct_log[lhs.data - 1] +
														oct_log[rhs.data - 1]];
		} else {
			lhs.data = 0;
		}
		return lhs;
	}
	Octet& operator/= (const Octet a)
	{
		if (a.data != 0 && data != 0) {
			data = RaptorQ::Impl::oct_exp[oct_log[data - 1] -
													oct_log[a.data - 1] + 255];
		}
		return *this;
	}
	friend Octet operator/ (Octet lhs, const Octet rhs)
	{
		lhs /= rhs;
		return lhs;
	}
	friend std::ostream &operator<< (std::ostream &os, Octet const &m) {
		os << std::hex << static_cast<ssize_t> (m.data) << std::dec;
		return os;
	}
private:
	uint8_t data;
};

inline uint8_t abs (Octet x) { return static_cast<uint8_t> (x); }

}	// namespace Impl
}	// namespace RaptorQ

namespace Eigen {
template<>
struct NumTraits<RaptorQ::Impl::Octet> : NumTraits<uint8_t> {};
}


#endif
