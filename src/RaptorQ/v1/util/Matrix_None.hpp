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

class RAPTORQ_LOCAL Matrix_None
{
public:

    static void add(uint8_t *dest, uint8_t *src, int32_t bytes)
    {
        for (int32_t i = 0; i < bytes;i++) {
            dest[i] = dest[i] ^ src[i];
        }
    }

    static void div(uint8_t *data, uint8_t num, int32_t bytes)
    {
        num = oct_log[num - 1];
        for (int32_t i = 0; i < bytes;i++) {
            if (data[i] != 0) {
                data[i] = oct_exp[oct_log[data[i] - 1] - num + 255];
            }
        }
    }

    // The SIMD versions are: dest[i] ^= (high[src[i]>>4] ^ low[src[i]&0xf]);
    // high and low are 16 byte lookup tables (high four bits and low four bits)
    static void multiply_and_add(uint8_t *dest, uint8_t *src, uint8_t num,
                                                                int32_t bytes)
    {
        if (num == 0) {
            return;
        }
        // TODO: Probably faster with a single lookup based on num, a single
        //       lookup require an additional lookup table
        uint16_t log_num = oct_log_no_if[num];
        for (int32_t i = 0; i < bytes;i++) {
            dest[i] = dest[i] ^ oct_exp_no_if[oct_log_no_if[src[i]] + log_num];
        }
    }
};
} // namespace Impl
} // namespace RaptorQ__v1
