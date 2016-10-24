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

#include "RaptorQ/v1/wrapper/C_common.h"
#include "RaptorQ/v1/common.hpp" // includes RaptorQ_Errors
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif
    typedef uint64_t RaptorQ_OTI_Common_Data;
    typedef uint32_t RaptorQ_OTI_Scheme_Specific_Data;

    struct RAPTORQ_LOCAL RaptorQ_ptr;

    RAPTORQ_API struct RaptorQ_ptr* RaptorQ_Enc (const RaptorQ_type type,
                                            void *data,
                                            const uint64_t size,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_memory);
    RAPTORQ_API struct RaptorQ_ptr* RaptorQ_Dec (const RaptorQ_type type,
                                const RaptorQ_OTI_Common_Data common,
                                const RaptorQ_OTI_Scheme_Specific_Data scheme);

    ///////////////////////////
    // Precomputation caching
    ///////////////////////////
    RaptorQ_Compress RAPTORQ_API RaptorQ_supported_compressions();
    RaptorQ_Compress RAPTORQ_API RaptorQ_get_compression();
    bool RAPTORQ_API RaptorQ_set_compression (
                                            const RaptorQ_Compress compression);

    size_t RAPTORQ_API RaptorQ_shared_cache_size (const size_t shared_cache);
    RaptorQ_Error RAPTORQ_API RaptorQ_local_cache_size (
                                                    const size_t local_cache);
    size_t RAPTORQ_API RaptorQ_get_shared_cache_size ();
    size_t RAPTORQ_API RaptorQ_get_local_cache_size ();

    /////////////////////
    // Common functions
    /////////////////////

    // C++11 async API into C pointers
    struct RaptorQ_Result {
        RaptorQ_Error error;
        uint8_t sbn;
    };
    struct RAPTORQ_LOCAL RaptorQ_future;

    RaptorQ_Error RAPTORQ_API RaptorQ_future_valid (
                                                struct RaptorQ_future *future);
    RaptorQ_Error RAPTORQ_API RaptorQ_future_wait_for (
                                                struct RaptorQ_future *future,
                                                const uint64_t time,
                                                const RaptorQ_Unit_Time unit);
    void RAPTORQ_API RaptorQ_future_wait (struct RaptorQ_future *future);
    void RAPTORQ_API RaptorQ_future_free (struct RaptorQ_future **future);
    struct RaptorQ_Result RAPTORQ_API RaptorQ_future_get (
                                                struct RaptorQ_future *future);


    bool RAPTORQ_API RaptorQ_set_thread_pool (const size_t threads,
                                        const uint16_t max_block_concurrency,
                                        const RaptorQ_Work exit_type);

    RAPTORQ_API struct RaptorQ_future* RaptorQ_compute (struct RaptorQ_ptr *ptr,
                                                const RaptorQ_Compute flags);

    ///////////
    // Encoding
    ///////////

    RaptorQ_OTI_Common_Data RAPTORQ_API RaptorQ_OTI_Common (
                                                    struct RaptorQ_ptr *enc);
    RaptorQ_OTI_Scheme_Specific_Data RAPTORQ_API RaptorQ_OTI_Scheme (
                                                    struct RaptorQ_ptr *enc);

    uint16_t RAPTORQ_API RaptorQ_symbol_size (struct RaptorQ_ptr *ptr);
    uint8_t RAPTORQ_API RaptorQ_blocks (struct RaptorQ_ptr *ptr);
    uint32_t RAPTORQ_API RaptorQ_block_size (struct RaptorQ_ptr *ptr,
                                                            const uint8_t sbn);
    uint16_t RAPTORQ_API RaptorQ_symbols (struct RaptorQ_ptr *ptr,
                                                            const uint8_t sbn);
    uint32_t RAPTORQ_API RaptorQ_max_repair (struct RaptorQ_ptr *enc,
                                                            const uint8_t sbn);
    size_t RAPTORQ_API RaptorQ_precompute_max_memory (struct RaptorQ_ptr *enc);

    size_t RAPTORQ_API RaptorQ_encode_id (struct RaptorQ_ptr *enc,
                                                            void **data,
                                                            const size_t size,
                                                            const uint32_t id);
    size_t RAPTORQ_API RaptorQ_encode (struct RaptorQ_ptr *enc, void **data,
                                                            const size_t size,
                                                            const uint32_t esi,
                                                            const uint8_t sbn);
    uint32_t RAPTORQ_API RaptorQ_id (const uint32_t esi, const uint8_t sbn);


    ///////////
    // Decoding
    ///////////

    void RAPTORQ_API RaptorQ_end_of_input (struct RaptorQ_ptr *dec);
    void RAPTORQ_API RaptorQ_end_of_block_input (struct RaptorQ_ptr *dec,
                                                        const uint8_t block);

    uint64_t RAPTORQ_API RaptorQ_bytes (struct RaptorQ_ptr *dec);

    struct RaptorQ_Dec_Result {
        uint64_t written;
        uint8_t skip;
    };

    struct RaptorQ_Dec_Result RAPTORQ_API RaptorQ_decode_aligned (
                                                        struct RaptorQ_ptr *dec,
                                                        void **data,
                                                        const uint64_t size,
                                                        const uint8_t skip);
    struct RaptorQ_Dec_Result RAPTORQ_API RaptorQ_decode_block_aligned (
                                                        struct RaptorQ_ptr *dec,
                                                        void **data,
                                                        const size_t size,
                                                        const uint8_t skip,
                                                        const uint8_t sbn);
    uint64_t RAPTORQ_API RaptorQ_decode_bytes (struct RaptorQ_ptr *dec,
                                                        void **data,
                                                        const uint64_t size,
                                                        const uint8_t skip);
    size_t RAPTORQ_API RaptorQ_decode_block_bytes (struct RaptorQ_ptr *dec,
                                                        void **data,
                                                        const size_t size,
                                                        const uint8_t skip,
                                                        const uint8_t sbn);

    RaptorQ_Error RAPTORQ_API RaptorQ_add_symbol_id (struct RaptorQ_ptr *dec,
                                                        void **data,
                                                        const uint32_t size,
                                                        const uint32_t id);
    RaptorQ_Error RAPTORQ_API RaptorQ_add_symbol (struct RaptorQ_ptr *dec,
                                                            void **data,
                                                            const uint32_t size,
                                                            const uint32_t esi,
                                                            const uint8_t sbn);

    ///////////////////////
    // General: free memory
    ///////////////////////

    void RAPTORQ_API RaptorQ_free (struct RaptorQ_ptr **ptr);
    void RAPTORQ_API RaptorQ_free_block (struct RaptorQ_ptr *ptr,
                                                            const uint8_t sbn);

#ifdef __cplusplus
}   // extern "C"
#endif
