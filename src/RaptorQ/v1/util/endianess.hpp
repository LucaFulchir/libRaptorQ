/*
 * Copyright (c) 2018, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

// Some will argue that this is unnecessary,
// that we do not really need it, and just using
// htonl every time is enough.
// but consider:
// * different os might have different includes and names for htonl
//   so you need at least a single file like this that
//   will check which is the right include, or you will
//   duplicate a lot of code.
// * we use uintX_t, as long, short and so on have
//   different meaning on different architectures
// * we need to convert the endianness of fields that
//   have more than 32 bits anyway
// * If you specify the correct endianness, this becomes constexpr. htonl isn't
// so no, do not simplify this into htonl().


namespace RaptorQ__v1 {
namespace Impl {
namespace Endian {

enum class Endianness : uint8_t {
    LITTLE, // x86
    BIG     // arm, network...
};

constexpr Endianness RAPTORQ_LOCAL get_endianness()
{
#if (defined(RQ_BIG_ENDIAN) && !defined(RQ_LITTLE_ENDIAN))
    return Endianness::BIG;
#elif (defined(RQ_LITTLE_ENDIAN) && !defined(RQ_BIG_ENDIAN))
    return Endianness::LITTLE;
#else
    // no real way to check for the endianness at compile time.
    // also, if you can do some trick at compile time with constexpr,
    // note that it will not work when cross-compiling, since the compiler
    // that executes the constexpr is still the one with the same endianness
    // or at least, that is what I could get to with https://gcc.godbolt.org/
    static_assert (false, "RQ: Please specify the endianness with "
                                                        "-DRQ_LITTLE_ENDIAN "
                                                        "or -DRQ_BIG_ENDIAN");
#endif
}


template<typename T>
constexpr T rev (const T in, const T acc, const uint8_t i)
{
    return (i >= sizeof(T)) ? acc :
                        rev<T> (in >> 8, (acc << 8) | (in & T{0xFF}), i + 1);
}

template<typename T>
constexpr T RAPTORQ_LOCAL rev (const T el)
    { return rev<T> (el, T{0}, 0); }

template<typename T>
constexpr T RAPTORQ_LOCAL h_to_b (const T host)
    { return (get_endianness() == Endianness::BIG) ? host : rev<T> (host); }

template<typename T>
constexpr T RAPTORQ_LOCAL h_to_l (const T host)
    { return (get_endianness() == Endianness::LITTLE) ? host : rev<T> (host); }

template<typename T>
constexpr T RAPTORQ_LOCAL b_to_h (const T big)
    { return (get_endianness() == Endianness::BIG) ? big : rev<T> (big); }

template<typename T>
constexpr T RAPTORQ_LOCAL l_to_h (const T little)
{
    return (get_endianness() == Endianness::LITTLE) ? little : rev<T> (little);
}


} // namespace Endian
} // namespace Impl
} // namespace RaptorQ__v1

