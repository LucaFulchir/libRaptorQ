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

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif
#include "RaptorQ/v1/block_sizes.hpp"
#include "RaptorQ/v1/common.hpp"
#include "RaptorQ/v1/wrapper/C_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct RAPTORQ_LOCAL RaptorQ_ptr;
    struct RAPTORQ_LOCAL RaptorQ_future;
    struct RAPTORQ_LOCAL RaptorQ_future_enc;
    struct RAPTORQ_LOCAL RaptorQ_future_dec;

    // do NOT mark the members of these structs as const.
    // MSVC breaks in annoyingly ways.
    // tracks RaptorQ__v1::Impl::Decoder_wait_res
    struct RAPTORQ_API RaptorQ_Dec_wait_res {
        RaptorQ_Error error;
        uint16_t symbol;
    };
    // tracks RaptorQ__v1::Impl::Decoder_written
    struct RAPTORQ_API RaptorQ_Dec_Written {
        size_t written;
        size_t offset;
    };
    struct RAPTORQ_API RaptorQ_Byte_Tracker {
        uint64_t length;
        uint8_t *bitmask;
    };
    // tracks RaptorQ__v1::Impl::Dec_Report
    typedef enum {
        RQ_PARTIAL_FROM_BEGINNING = RQ_COMPUTE_PARTIAL_FROM_BEGINNING,
        RQ_PARTIAL_ANY = RQ_COMPUTE_PARTIAL_ANY,
        RQ_COMPLETE = RQ_COMPUTE_COMPLETE
    } RAPTORQ_API RQ_Dec_Report;


    RAPTORQ_API struct RaptorQ_base_api* RaptorQ_api (uint32_t version);
    RAPTORQ_API void RaptorQ_free_api (struct RaptorQ_base_api **api);

    struct RAPTORQ_API RaptorQ_base_api
    {
        const size_t version;
        #ifdef __cplusplus
        RaptorQ_base_api (size_t ver) : version (ver) {}
        #endif
    };

    struct RAPTORQ_API RaptorQ_v1
    {
        const struct RaptorQ_base_api base;
        #ifdef __cplusplus
        RaptorQ_v1();
        #endif

        // precomputation caching
        RaptorQ_Compress (*const supported_compressions) (void);
        RaptorQ_Compress (*const get_compression) (void);
        bool (*const set_compression) (const RaptorQ_Compress);
        size_t  (*const local_cache_size) (const size_t);
        size_t  (*const get_local_cache_size) (void);

        // constructos
        struct RaptorQ_ptr* (*const Encoder) (RaptorQ_type type,
                                            const RaptorQ_Block_Size symbols,
                                            const size_t symbol_size);
        struct RaptorQ_ptr* (*const Decoder) (RaptorQ_type type,
                                            const RaptorQ_Block_Size symbols,
                                            const size_t symbol_size,
                                            const RQ_Dec_Report report);
        bool (*const initialized) (const struct RaptorQ_ptr *ptr);

        // common functions
        uint16_t (*const symbols)     (const struct RaptorQ_ptr *ptr);
        size_t   (*const symbol_size) (const struct RaptorQ_ptr *ptr);
        void (*const stop) (const struct RaptorQ_ptr *ptr);
        RaptorQ_Error (*const future_state) (struct RaptorQ_future *const f);
        RaptorQ_Error (*const future_wait_for) (struct RaptorQ_future *const f,
                                                const uint64_t time,
                                                const RaptorQ_Unit_Time unit);
        void (*const future_wait) (struct RaptorQ_future *const f);
        void (*const future_free) (struct RaptorQ_future **f);
        void (*const free) (struct RaptorQ_ptr **ptr);
        bool (*const ready) (const struct RaptorQ_ptr *ptr);

        // encoder-specific
        uint32_t (*const max_repair)  (const struct RaptorQ_ptr *enc);
        size_t (*const set_data) (const struct RaptorQ_ptr *enc, void **from,
                                                            const size_t size);
        bool (*const has_data) (const struct RaptorQ_ptr *enc);
        void (*const clear_data) (const struct RaptorQ_ptr *ptr);
        bool (*const precompute_sync) (const struct RaptorQ_ptr *enc);
        bool (*const compute_sync) (const struct RaptorQ_ptr *enc);
        struct RaptorQ_future_enc* (*const precompute) (
                                                const struct RaptorQ_ptr *enc);
        struct RaptorQ_future_enc* (*const compute) (
                                                const struct RaptorQ_ptr *enc);
        RaptorQ_Error (*const enc_future_get) (struct RaptorQ_future_enc *f);
        size_t (*const encode) (const struct RaptorQ_ptr *enc, void **from,
                                        const size_t size, const uint32_t ESI);

        // decoder-specific
        RaptorQ_Error (*const add_symbol) (const struct RaptorQ_ptr *dec,
                                                            void **from,
                                                            const size_t size,
                                                            const uint32_t esi);

        bool (*const can_decode) (const struct RaptorQ_ptr *dec);
        uint16_t (*const needed_symbols) (const struct RaptorQ_ptr *dec);

        struct RaptorQ_Dec_wait_res (*const poll) (
                                                const struct RaptorQ_ptr *dec);
        struct RaptorQ_Dec_wait_res (*const wait_sync) (
                                                const struct RaptorQ_ptr *dec);
        struct RaptorQ_future_dec* (*const wait) (
                                                const struct RaptorQ_ptr *dec);
        struct RaptorQ_Dec_wait_res (*const dec_future_get) (
                                                struct RaptorQ_future_dec *f);
        struct RaptorQ_Byte_Tracker (*const end_of_input) (
                                        struct RaptorQ_ptr *dec,
                                        const RaptorQ_Fill_With_Zeros fill);
        RaptorQ_Decoder_Result (*const decode_once) (struct RaptorQ_ptr *dec);
        RaptorQ_Error (*const decode_symbol) (struct RaptorQ_ptr *dec,
                                                            void** start,
                                                            const size_t size,
                                                            const uint16_t esi);
        struct RaptorQ_Dec_Written (*const decode_bytes) (
                                                        struct RaptorQ_ptr *dec,
                                                        void **start,
                                                        const size_t size,
                                                        const size_t from_byte,
                                                        const size_t skip);

    };


#ifdef __cplusplus
}   // extern "C"
#endif
