/*
 * Copyright (c) 2016-2018, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#include "RaptorQ/v1/wrapper/C_common.h"
#include "RaptorQ/v1/block_sizes.hpp"
#include <vector>
#if __cplusplus >= 201103L || _MSC_VER > 1900
#include <future>
#define RQ_EXPLICIT explicit
#else
#define RQ_EXPLICIT
#endif


/////////////////////
//
//  These templates are just a wrapper around the
//  functionalities offered by the RaptorQ__v1::Impl namespace
//  So if you want to see what the algorithm looks like,
//  you are in the wrong place
//
/////////////////////

namespace RFC6330__v1 {
namespace Impl {

class RAPTORQ_API Encoder_void final
{
public:
    ~Encoder_void();
    Encoder_void (const RaptorQ_type type, const void* data_from,
                                            const void *data_to,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block);
    Encoder_void() = delete;
    Encoder_void (const Encoder_void&) = delete;
    Encoder_void& operator= (const Encoder_void&) = delete;
    Encoder_void (Encoder_void&&) = default;
    Encoder_void& operator= (Encoder_void&&) = default;

    RQ_EXPLICIT operator bool() const;
    RFC6330_OTI_Common_Data OTI_Common() const;
    RFC6330_OTI_Scheme_Specific_Data OTI_Scheme_Specific() const;

    #if __cplusplus >= 201103L || _MSC_VER > 1900
    std::future<std::pair<Error, uint8_t>> compute (const Compute flags);
    #endif
    class result
    {
    public:
        result (const Error _err, const uint8_t _sbn)
            : err (_err), sbn (_sbn) {}
        Error err;
        uint8_t sbn;
    };
    result compute_sync (const Compute flags);

    size_t precompute_max_memory ();
    size_t encode (void** output, const void* end, const uint32_t esi,
                                                            const uint8_t sbn);
    size_t encode (void** output, const void* end, const uint32_t id);
    void free (const uint8_t sbn);
    uint8_t blocks() const;
    uint32_t block_size (const uint8_t sbn) const;
    uint16_t symbol_size() const;
    uint16_t symbols (const uint8_t sbn) const;
    Block_Size extended_symbols (const uint8_t sbn) const;
    uint32_t max_repair (const uint8_t sbn) const;
private:
    RaptorQ_type _type;
    void *_encoder;
};

////////////////////
//// Decoder
////////////////////

class RAPTORQ_API Decoder_void final
{
public:

    ~Decoder_void();
    Decoder_void (const RaptorQ_type type, const RFC6330_OTI_Common_Data common,
                            const RFC6330_OTI_Scheme_Specific_Data scheme);

    Decoder_void (const RaptorQ_type type, const uint64_t size,
                                                    const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment);
    Decoder_void() = delete;
    Decoder_void (const Decoder_void&) = delete;
    Decoder_void& operator= (const Decoder_void&) = delete;
    Decoder_void (Decoder_void&&) = default;
    Decoder_void& operator= (Decoder_void&&) = default;
    RQ_EXPLICIT operator bool() const;

    #if __cplusplus >= 201103L || _MSC_VER > 1900
    std::future<std::pair<Error, uint8_t>> compute (const Compute flags);
    #endif

    std::vector<bool> end_of_input (const Fill_With_Zeros fill,
                                                        const uint8_t block);
    std::vector<bool> end_of_input (const Fill_With_Zeros fill);

    uint64_t decode_symbol (void** start, const void* end, const uint16_t esi,
                                                            const uint8_t sbn);
    uint64_t decode_bytes (void** start, const void* end, const uint8_t skip);
    size_t decode_block_bytes (void** start, const void* end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn);
    Decoder_written decode_aligned (void** start, const void* end,
                                                            const uint8_t skip);
    Decoder_written decode_block_aligned (void** start, const void* end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn);
    Error add_symbol (void** start, const void* end, const uint32_t id);
    Error add_symbol (void** start, const void* end, const uint32_t esi,
                                                            const uint8_t sbn);
    uint8_t blocks_ready();
    bool is_ready();
    bool is_block_ready (const uint8_t block);
    void free (const uint8_t sbn);
    uint64_t bytes() const;
    uint8_t blocks() const;
    uint32_t block_size (const uint8_t sbn) const;
    uint16_t symbol_size() const;
    uint16_t symbols (const uint8_t sbn) const;
    Block_Size extended_symbols (const uint8_t sbn) const;
private:
    RaptorQ_type _type;
    void *_decoder;
};

} // namesapce Impl
} // namespace RFC6330__v1
