/*
 * Copyright (c) 2015-2018, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

#include "RaptorQ/v1/wrapper/C_common.h"
#include "RaptorQ/v1/common.hpp"
#include "RaptorQ/v1/block_sizes.hpp"
#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    struct RAPTORQ_LOCAL RFC6330_ptr;
    struct RAPTORQ_LOCAL RFC6330_future;

    // do NOT mark the members of these structs as const.
    // MSVC breaks in annoyingly ways.
    // tracks the result of the future in RFC.hpp.compute()
    struct RAPTORQ_API RFC6330_Result {
        RFC6330_Error error;
        uint8_t sbn;
    };
    // tracks common.hpp/RFC6330__v1::Impl::Decoder_written
    struct RAPTORQ_API RFC6330_Dec_Result {
        uint64_t written;
        uint8_t offset;
    };

    struct RAPTORQ_API RFC6330_Byte_Tracker {
        uint64_t length;
        uint8_t *bitmask;
    };


    RAPTORQ_API struct RFC6330_base_api* RFC6330_api (uint32_t version);
    RAPTORQ_API void RFC6330_free_api (struct RFC6330_base_api **api);

    struct RFC6330_base_api
    {
        const size_t version;
        #ifdef __cplusplus
        RFC6330_base_api (const size_t ver) : version (ver) {}
        #endif
    };

    struct RFC6330_v1
    {
        const struct RFC6330_base_api base;
        #ifdef __cplusplus
        RFC6330_v1();
        #endif

        // precomputation caching
        RFC6330_Compress (*const supported_compressions) (void);
        RFC6330_Compress (*const get_compression) (void);
        bool (*const set_compression) (const RFC6330_Compress);
        size_t  (*const local_cache_size) (const size_t);
        size_t  (*const get_local_cache_size) (void);


        // constructos
        struct RFC6330_ptr* (*const Encoder) (RFC6330_type type,
                                            const void *data_from,
                                            const size_t size,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block);
        struct RFC6330_ptr* (*const Decoder) (RFC6330_type type,
                                const RFC6330_OTI_Common_Data common,
                                const RFC6330_OTI_Scheme_Specific_Data scheme);
        struct RFC6330_ptr* (*const Decoder_raw) (RFC6330_type type,
                                                    const uint64_t size,
                                                    const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment);
        bool (*const initialized) (const struct RFC6330_ptr *ptr);


        // common functions
        uint8_t (*const blocks) (const struct RFC6330_ptr *ptr);
        uint16_t (*const symbols)     (const struct RFC6330_ptr *ptr,
                                                            const uint8_t sbn);
        RFC6330_Block_Size (*const extended_symbols) (
                                                const struct RFC6330_ptr *ptr,
                                                const uint8_t sbn);
        size_t   (*const symbol_size) (const struct RFC6330_ptr *ptr);
        RFC6330_Error (*const future_state) (const struct RFC6330_future *f);
        RFC6330_Error (*const future_wait_for) (const struct RFC6330_future *f,
                                                const uint64_t time,
                                                const RFC6330_Unit_Time unit);
        void (*const future_wait) (const struct RFC6330_future *f);
        void (*const future_free) (struct RFC6330_future **f);
        struct RFC6330_Result (*const future_get) (
                                                struct RFC6330_future *future);
        bool (*const set_thread_pool) (const size_t threads,
                                        const uint16_t max_block_concurrency,
                                        const RFC6330_Work exit_type);
        struct RFC6330_future* (*const compute) (const struct RFC6330_ptr *ptr,
                                                const RFC6330_Compute flags);
        void (*const free) (struct RFC6330_ptr **ptr);
        void (*const free_block) (const struct RFC6330_ptr *ptr,
                                                            const uint8_t sbn);


        // encoder-specific
        RFC6330_OTI_Common_Data (*const OTI_Common) (
                                                const struct RFC6330_ptr *enc);
        RFC6330_OTI_Scheme_Specific_Data (*const OTI_Scheme_Specific) (
                                                const struct RFC6330_ptr *enc);
        uint32_t (*const max_repair) (const struct RFC6330_ptr *enc,
                                                            const uint8_t sbn);
        size_t (*const precompute_max_memory) (const struct RFC6330_ptr *enc);

        size_t (*const encode_id) (const struct RFC6330_ptr *enc, void **data,
                                                            const size_t size,
                                                            const uint32_t id);
        size_t (*const encode) (const struct RFC6330_ptr *enc, void **data,
                                                            const size_t size,
                                                            const uint32_t esi,
                                                            const uint8_t sbn);
        uint32_t (*const id) (const uint32_t esi, const uint8_t sbn);


        // decoder-specific
        struct RFC6330_Byte_Tracker (*const end_of_input) (
                                            const struct RFC6330_ptr *dec,
                                            const RaptorQ_Fill_With_Zeros fill);
        struct RFC6330_Byte_Tracker (*const end_of_block_input) (
                                            const struct RFC6330_ptr *dec,
                                            const RaptorQ_Fill_With_Zeros fill,
                                            const uint8_t block);
        uint64_t (*const bytes) (const struct RFC6330_ptr *dec);
        uint8_t (*blocks_ready) (const struct RFC6330_ptr *dec);
        bool (*is_ready) (const struct RFC6330_ptr *dec);
        bool (*is_block_ready) (const struct RFC6330_ptr *dec,
                                                        const uint8_t block);
        RFC6330_Error (*const add_symbol_id) (const struct RFC6330_ptr *dec,
                                                            void **data,
                                                            const uint32_t size,
                                                            const uint32_t id);
        RFC6330_Error (*const add_symbol) (const struct RFC6330_ptr *dec,
                                                            void **data,
                                                            const uint32_t size,
                                                            const uint32_t esi,
                                                            const uint8_t sbn);
        struct RFC6330_Dec_Result (*const decode_aligned) (
                                                const struct RFC6330_ptr *dec,
                                                void **data,
                                                const uint64_t size,
                                                const uint8_t skip);
        struct RFC6330_Dec_Result (*const decode_block_aligned) (
                                                const struct RFC6330_ptr *dec,
                                                void **data,
                                                const size_t size,
                                                const uint8_t skip,
                                                const uint8_t sbn);
        uint64_t (*const decode_symbol) (const struct RFC6330_ptr *dec,
                                                            void **data,
                                                            const size_t size,
                                                            const uint16_t esi,
                                                            const uint8_t sbn);
        uint64_t (*const decode_bytes) (const struct RFC6330_ptr *dec,
                                                            void **data,
                                                            const uint64_t size,
                                                            const uint8_t skip);
        size_t (*const decode_block_bytes) (const struct RFC6330_ptr *dec,
                                                            void **data,
                                                            const size_t size,
                                                            const uint8_t skip,
                                                            const uint8_t sbn);
    };


#ifdef __cplusplus
}   // extern "C"
#endif
