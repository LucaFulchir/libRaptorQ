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
#include <cstdlib>
#include <stdlib.h>
#include <stdint.h>
#if defined(_MSC_VER)
# include <intrin.h>
#endif


namespace RaptorQ__v1 {
namespace Impl {

class RAPTORQ_LOCAL CPU_Info
{
public:
    static bool has_ssse3()
    {
#ifdef __SSSE3__
        uint32_t abcd[4];
        uint32_t mask = ((uint32_t)1 <<  0) | ((uint32_t)1 <<  9); //SSE3, SSSE3
        run_cpuid( 1, 0, abcd );
        if ((abcd[2] & mask) == mask )
            return true;
#endif
        return false;
    }

    static bool has_avx2()
    {
#ifdef __AVX2__
        uint32_t abcd[4];
        uint32_t mask = ((uint32_t)1 <<  5);
        run_cpuid( 7, 0, abcd );
        if ((abcd[1] & mask) == mask )
            return true;
#endif
        return false;
    }
private:
    /// From Intel: How to detect New Instruction support in the 4th generation
    // Intel® Core™ processor family
    static void run_cpuid(uint32_t eax, uint32_t ecx, uint32_t* abcd)
    {
#if defined(_MSC_VER)
        __cpuidex(abcd, eax, ecx);
#else
        uint32_t ebx, edx;
        ebx = 0;
        edx = 0;
# if defined( __i386__ ) && defined ( __PIC__ )
        // in case of PIC under 32-bit EBX cannot be clobbered
        __asm__ ( "movl %%ebx, %%edi \n\t cpuid \n\t xchgl %%ebx, %%edi" : "=D" (ebx),
# else
        __asm__ ( "cpuid" : "+b" (ebx),
# endif
                  "+a" (eax), "+c" (ecx), "=d" (edx) );
        abcd[0] = eax; abcd[1] = ebx; abcd[2] = ecx; abcd[3] = edx;
#endif
    }
};
} // namespace Impl
} // namespace RaptorQ__v1
