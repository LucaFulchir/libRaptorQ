/*
 * Copyright (c) 2018 by Intinor AB. All rights reserved.
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
#include "../common.hpp"
#include "RaptorQ/v1/gf256.hpp"
#include "RaptorQ/v1/multiplication.hpp"
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <x86intrin.h>

namespace RaptorQ__v1 {
namespace Impl {

class RAPTORQ_LOCAL Matrix_SSSE3
{
public:
    static void add(uint8_t *dest, uint8_t *src, int32_t bytes)
    {
#ifdef __SSSE3__
        uint8_t *src_ptr, *dest_ptr;
        __m128i s, d;

        assert(bytes % 16 == 0);
        assert(bytes >= 16);

        src_ptr = src;
        dest_ptr = dest;

        int32_t limit = bytes / 16;
        for (int32_t i = 0; i < limit; i++) {
            s = _mm_load_si128((__m128i *) (src_ptr));
            d = _mm_load_si128((__m128i *) (dest_ptr));
            d = _mm_xor_si128(d, s);
            _mm_store_si128((__m128i *) (dest_ptr), d);
            dest_ptr += 16;
            src_ptr += 16;
        }
#endif
    }

    static void div(uint8_t *data, uint8_t num, int32_t bytes)
    {
#ifdef __SSSE3__
        uint8_t *high, *low, *data_ptr;
        __m128i mask, t, r, v, m_table_high, m_table_low;

        assert(bytes % 16 == 0);
        assert(bytes >= 16);

        // Inverse num (div is multiply with 1/num)
        num = oct_exp[255 - oct_log[num - 1]];
        int32_t limit = bytes / 16;

        high = gf256_high_ptr + (num << 4) * 2;
        low = gf256_low_ptr + (num << 4) * 2;

        mask = _mm_set1_epi8(0x0f);
        m_table_high = _mm_loadu_si128((__m128i *) (high));
        m_table_low = _mm_loadu_si128((__m128i *) (low));

        data_ptr = data;
        for (int32_t i = 0; i < limit; i++) {
            v = _mm_load_si128((__m128i *) (data_ptr));
            t = _mm_and_si128(mask, v);
            r = _mm_shuffle_epi8(m_table_low, t);
            v = _mm_srli_epi64(v, 4);
            t = _mm_and_si128(mask, v);
            r = _mm_xor_si128(r, _mm_shuffle_epi8(m_table_high, t));
            _mm_store_si128((__m128i *) (data_ptr), r);
            data_ptr += 16;
        }
#endif
    }

    static void multiply_and_add(uint8_t *dest, uint8_t *src, uint8_t num,
                                                                int32_t bytes)
    {
#ifdef __SSSE3__
        uint8_t *high, *low, *src_ptr, *dest_ptr;
        __m128i mask, t, r, v, m_table_high, m_table_low;

        assert(bytes % 16 == 0);
        assert(bytes >= 16);

        int32_t limit = bytes / 16;

        high = gf256_high_ptr + (num << 4) * 2;
        low = gf256_low_ptr + (num << 4) * 2;

        mask = _mm_set1_epi8(0x0f);
        m_table_high = _mm_loadu_si128((__m128i *) (high));
        m_table_low = _mm_loadu_si128((__m128i *) (low));

        src_ptr = src;
        dest_ptr = dest;
        for (int32_t i = 0; i < limit; i++) {
            v = _mm_load_si128((__m128i *) (src_ptr));
            t = _mm_and_si128(mask, v);
            r = _mm_shuffle_epi8(m_table_low, t);
            v = _mm_srli_epi64(v, 4);
            t = _mm_and_si128(mask, v);
            r = _mm_xor_si128(r, _mm_shuffle_epi8(m_table_high, t));
            v = _mm_load_si128((__m128i *) (dest_ptr));
            r = _mm_xor_si128(r, v);
            _mm_store_si128((__m128i *) (dest_ptr), r);
            dest_ptr += 16;
            src_ptr += 16;
        }
#endif
    }
};
} // namespace Impl
} // namespace RaptorQ__v1
