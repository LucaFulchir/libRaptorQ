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
#include "RaptorQ/v1/multiplication.hpp"
#include <cmath>
#include <Eigen/Core>
#include <vector>

namespace RaptorQ__v1 {
namespace Impl {

class RAPTORQ_LOCAL Octet
{
public:
    Octet () {}
    Octet (const uint8_t val) noexcept : data(val) {}
    Octet (const Octet&) noexcept = default;
    Octet& operator= (const Octet&) noexcept = default;
    Octet (Octet&&) noexcept = default;
    Octet& operator= (Octet&&) noexcept = default;

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
            data = RaptorQ__v1::Impl::oct_exp[oct_log[data - 1] +
                                            oct_log[a.data - 1]];
        } else {
            data = 0;
        }
        return *this;
    }
    friend Octet operator* (Octet lhs, const Octet rhs)
    {
        if (lhs.data != 0 && rhs.data != 0) {
            lhs.data = RaptorQ__v1::Impl::oct_exp[oct_log[lhs.data - 1] +
                                                        oct_log[rhs.data - 1]];
        } else {
            lhs.data = 0;
        }
        return lhs;
    }
    Octet& operator/= (const Octet a)
    {
        if (a.data != 0 && data != 0) {
            data = RaptorQ__v1::Impl::oct_exp[oct_log[data - 1] -
                                                    oct_log[a.data - 1] + 255];
        }
        return *this;
    }

    friend Octet operator/ (Octet lhs, const Octet rhs)
    {
        lhs /= rhs;
        return lhs;
    }

    Octet inverse() const
    {
        return Octet (RaptorQ__v1::Impl::oct_exp[255 - oct_log[data - 1]]);
    }

    bool operator== (const Octet a) const
    { return data == a.data; }
    bool operator!= (const Octet a) const
    { return data != a.data; }

    friend std::ostream &operator<< (std::ostream &os, const Octet m) {
        // used to print
        os << static_cast<uint32_t> (m.data);
        return os;
    }
private:
    uint8_t data;
};

inline uint8_t abs (Octet x) { return static_cast<uint8_t> (x); }

}   // namespace Impl
}   // namespace RaptorQ

namespace Eigen {
template<>
struct NumTraits<RaptorQ__v1::Impl::Octet> : NumTraits<uint8_t> {};
}
