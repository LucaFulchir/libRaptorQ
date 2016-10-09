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
#include <cmath>

namespace RaptorQ__v1 {
namespace Impl {
    template<typename T>
    inline T RAPTORQ_LOCAL div_floor (const T a, const T b)
        { return a / b; }
    template<typename T>
    inline T RAPTORQ_LOCAL div_ceil (const T a, const T b)
    {
        T res = a / b;
        if ((a % b) != 0)
            ++res;
        return res;
    }

    template<>
    inline float RAPTORQ_LOCAL div_floor (const float a, const float b)
        { return std::floor(a / b); }
    template<>
    inline double RAPTORQ_LOCAL div_floor (const double a, const double b)
        { return std::floor(a / b); }
    template<>
    inline float RAPTORQ_LOCAL div_ceil (const float a, const float b)
        { return std::ceil(a / b); }
    template<>
    inline double RAPTORQ_LOCAL div_ceil (const double a, const double b)
        { return std::ceil(a / b); }
} // namespace Impl
} // namespace RaptorQ__v1

namespace RFC6330__v1 {
namespace Impl {
    template<typename T>
    inline T RAPTORQ_LOCAL div_floor (const T a, const T b)
        { return RaptorQ__v1::Impl::div_floor (a, b); }
    template<typename T>
    inline T RAPTORQ_LOCAL div_ceil (const T a, const T b)
        { return RaptorQ__v1::Impl::div_ceil (a, b); }
} // namespace Impl
} // namespace RFC6330__v1

