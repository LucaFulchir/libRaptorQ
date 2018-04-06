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

#include "RaptorQ/v1/wrapper/C_RFC_API.h"
#include "RaptorQ/v1/caches.hpp"
#include "RaptorQ/v1/RFC.hpp"
#include <chrono>
#include <future>
#include <memory>

struct RAPTORQ_LOCAL RFC6330_ptr
{
    void *const ptr;
    const RFC6330_type type;

    RFC6330_ptr ()
        : ptr (nullptr), type (RFC6330_type::RQ_NONE) {}
    RFC6330_ptr (const RFC6330_type _type, void *const _ptr)
        : ptr (_ptr), type (_type) {}
};

struct RAPTORQ_LOCAL RFC6330_future
{
    std::future<std::pair<RFC6330__v1::Error, uint8_t>> f;
    RFC6330_future(){}
};


////////////////////////
////
//// For ease of development please keep the function declaration
//// and implementation in the same orders as defined in the structs!
////
////////////////////////

// precomputation caching
static RFC6330_Compress v1_supported_compressions();
static RFC6330_Compress v1_get_compression();
static bool v1_set_compression (const RFC6330_Compress compression);
static size_t v1_local_cache_size (const size_t local_cache);
static size_t v1_get_local_cache_size ();
// constructors
static struct RFC6330_ptr* v1_Encoder (RFC6330_type type,
                                              const void *data_from,
                                              const size_t size,
                                              const uint16_t min_subsymbol_size,
                                              const uint16_t symbol_size,
                                              const size_t max_memory);
static struct RFC6330_ptr* v1_Decoder (RFC6330_type type,
                              const RFC6330_OTI_Common_Data common,
                              const RFC6330_OTI_Scheme_Specific_Data scheme);
static struct RFC6330_ptr* v1_Decoder_raw (RFC6330_type type,
                                                    const uint64_t size,
                                                    const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment);
static bool v1_initialized (const struct RFC6330_ptr *ptr);


// common functions
static uint8_t  v1_blocks (const struct RFC6330_ptr *ptr);
static uint16_t v1_symbols (const struct RFC6330_ptr *ptr, const uint8_t sbn);
static RFC6330_Block_Size v1_extended_symbols (const struct RFC6330_ptr *ptr,
                                                            const uint8_t sbn);
static size_t v1_symbol_size (const struct RFC6330_ptr *ptr);
static RFC6330_Error v1_future_state (const struct RFC6330_future *f);
static RFC6330_Error v1_future_wait_for (const struct RFC6330_future *f,
                                                const uint64_t time,
                                                const RFC6330_Unit_Time unit);
static void v1_future_wait (const struct RFC6330_future *f);
static void v1_future_free (struct RFC6330_future **f);
static struct RFC6330_Result v1_future_get (struct RFC6330_future *future);
static bool v1_set_thread_pool (const size_t threads,
                                const uint16_t max_block_concurrency,
                                const RFC6330_Work exit_type);
static struct RFC6330_future* v1_compute (const struct RFC6330_ptr *ptr,
                                                const RFC6330_Compute flags);
static void v1_free (struct RFC6330_ptr **ptr);
static void v1_free_block (const struct RFC6330_ptr *ptr, const uint8_t sbn);


// encoder-specific
static RFC6330_OTI_Common_Data v1_OTI_Common (const struct RFC6330_ptr *enc);
static RFC6330_OTI_Scheme_Specific_Data v1_OTI_Scheme_Specific (
                                                const struct RFC6330_ptr *enc);
static uint32_t v1_max_repair (const struct RFC6330_ptr *enc,
                                                            const uint8_t sbn);
static size_t v1_precompute_max_memory (const struct RFC6330_ptr *enc);

static size_t v1_encode_id (const struct RFC6330_ptr *enc, void **data,
                                                            const size_t size,
                                                            const uint32_t id);
static size_t v1_encode (const struct RFC6330_ptr *enc, void **data,
                                                            const size_t size,
                                                            const uint32_t esi,
                                                            const uint8_t sbn);
static uint32_t v1_id (const uint32_t esi, const uint8_t sbn);


// decoder-specific
static struct RFC6330_Byte_Tracker v1_end_of_input (
                                            const struct RFC6330_ptr *dec,
                                            const RFC6330_Fill_With_Zeros fill);
static struct RFC6330_Byte_Tracker v1_end_of_block_input (
                                            const struct RFC6330_ptr *dec,
                                            const RFC6330_Fill_With_Zeros fill,
                                            const uint8_t block);
static uint64_t v1_bytes (const struct RFC6330_ptr *dec);
static uint8_t v1_blocks_ready (const struct RFC6330_ptr *dec);
static bool v1_is_ready (const struct RFC6330_ptr *dec);
static bool v1_is_block_ready (const struct RFC6330_ptr *dec,
                                                        const uint8_t block);
static RFC6330_Error v1_add_symbol_id (const struct RFC6330_ptr *dec,
                                                            void **data,
                                                            const uint32_t size,
                                                            const uint32_t id);
static RFC6330_Error v1_add_symbol (const struct RFC6330_ptr *dec, void **data,
                                                            const uint32_t size,
                                                            const uint32_t esi,
                                                            const uint8_t sbn);
static struct RFC6330_Dec_Result v1_decode_aligned (
                                                const struct RFC6330_ptr *dec,
                                                void **data,
                                                const uint64_t size,
                                                const uint8_t skip);
static struct RFC6330_Dec_Result v1_decode_block_aligned (
                                                const struct RFC6330_ptr *dec,
                                                void **data,
                                                const size_t size,
                                                const uint8_t skip,
                                                const uint8_t sbn);
static uint64_t v1_decode_symbol (const struct RFC6330_ptr *dec, void **data,
                                                            const size_t size,
                                                            const uint16_t esi,
                                                            const uint8_t sbn);
static uint64_t v1_decode_bytes (const struct RFC6330_ptr *dec, void **data,
                                                            const uint64_t size,
                                                            const uint8_t skip);
static size_t v1_decode_block_bytes (const struct RFC6330_ptr *dec, void **data,
                                                            const size_t size,
                                                            const uint8_t skip,
                                                            const uint8_t sbn);



void RFC6330_free_api (struct RFC6330_base_api **api)
{
    if (api == nullptr || *api == nullptr)
        return;
    if ((*api)->version == 1) {
        delete reinterpret_cast<struct RFC6330_v1*> (*api);
    } // else it's all your fault anway
    *api = nullptr;
}

struct RFC6330_base_api* RFC6330_api (uint32_t version)
{
    if (version != 1)
        return nullptr;
    return reinterpret_cast<RFC6330_base_api *> (new RFC6330_v1());
}

RFC6330_v1::RFC6330_v1()
    : base (1),
    // precomputation caching
    supported_compressions (&v1_supported_compressions),
    get_compression (&v1_get_compression),
    set_compression (&v1_set_compression),
    local_cache_size (&v1_local_cache_size),
    get_local_cache_size (&v1_get_local_cache_size),

    // constructors
    Encoder (&v1_Encoder),
    Decoder (&v1_Decoder),
    Decoder_raw (&v1_Decoder_raw),
    initialized (&v1_initialized),

    // common functions
    blocks (&v1_blocks),
    symbols (&v1_symbols),
    extended_symbols (&v1_extended_symbols),
    symbol_size (&v1_symbol_size),
    future_state (&v1_future_state),
    future_wait_for (&v1_future_wait_for),
    future_wait (&v1_future_wait),
    future_free (&v1_future_free),
    future_get (&v1_future_get),
    set_thread_pool (&v1_set_thread_pool),
    compute (&v1_compute),
    free (&v1_free),
    free_block (&v1_free_block),

    // encoder-specific functions
    OTI_Common (&v1_OTI_Common),
    OTI_Scheme_Specific (&v1_OTI_Scheme_Specific),
    max_repair (&v1_max_repair),
    precompute_max_memory (&v1_precompute_max_memory),
    encode_id (&v1_encode_id),
    encode (&v1_encode),
    id (&v1_id),

    // decoder-specific functions
    end_of_input (&v1_end_of_input),
    end_of_block_input (&v1_end_of_block_input),
    bytes (&v1_bytes),
    blocks_ready (&v1_blocks_ready),
    is_ready (&v1_is_ready),
    is_block_ready (&v1_is_block_ready),
    add_symbol_id (&v1_add_symbol_id),
    add_symbol (&v1_add_symbol),
    decode_aligned (&v1_decode_aligned),
    decode_block_aligned (&v1_decode_block_aligned),
    decode_symbol (&v1_decode_symbol),
    decode_bytes (&v1_decode_bytes),
    decode_block_bytes (&v1_decode_block_bytes)
{}


///////////////////////////
// Precomputation caching
///////////////////////////

static RFC6330_Compress v1_supported_compressions()
{
    return static_cast<RFC6330_Compress>(RaptorQ__v1::supported_compressions());
}

static RFC6330_Compress v1_get_compression()
    { return static_cast<RFC6330_Compress>(RaptorQ__v1::get_compression()); }

static bool v1_set_compression (const RFC6330_Compress compression)
{
    return static_cast<RFC6330_Compress> (RaptorQ__v1::set_compression (
                            static_cast<RaptorQ__v1::Compress> (compression)));
}

static size_t v1_local_cache_size (const size_t local_cache)
    { return RFC6330__v1::local_cache_size (local_cache); }

static size_t v1_get_local_cache_size ()
    { return RFC6330__v1::get_local_cache_size(); }




/////////////////////
// Constructors
/////////////////////

static struct RFC6330_ptr* v1_Encoder (RFC6330_type type,
                                              const void *data_from,
                                              const size_t size,
                                              const uint16_t min_subsymbol_size,
                                              const uint16_t symbol_size,
                                              const size_t max_memory)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    void *raw_ptr = nullptr;
    switch (type) {
    case RFC6330_type::RQ_ENC_8:
        p_8 = reinterpret_cast<uint8_t*> (const_cast<void*> (data_from));
        raw_ptr = reinterpret_cast<void *> (
                        new RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*> (
                                                p_8, p_8 + size,
                                                min_subsymbol_size, symbol_size,
                                                max_memory));
        break;
    case RFC6330_type::RQ_ENC_16:
        p_16 = reinterpret_cast<uint16_t*> (const_cast<void*> (data_from));
        raw_ptr = reinterpret_cast<void *> (
                        new RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*> (
                                                p_16, p_16 + size,
                                                min_subsymbol_size, symbol_size,
                                                max_memory));
        break;
    case RFC6330_type::RQ_ENC_32:
        p_32 = reinterpret_cast<uint32_t*> (const_cast<void*> (data_from));
        raw_ptr = reinterpret_cast<void *> (
                    new RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*> (
                                                p_32, p_32 + size,
                                                min_subsymbol_size, symbol_size,
                                                max_memory));
        break;
    case RFC6330_type::RQ_ENC_64:
        p_64 = reinterpret_cast<uint64_t*> (const_cast<void*> (data_from));
        raw_ptr = reinterpret_cast<void *> (
                    new RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*> (
                                                p_64, p_64 + size,
                                                min_subsymbol_size, symbol_size,
                                                max_memory));
        break;
    case RFC6330_type::RQ_DEC_8:
    case RFC6330_type::RQ_DEC_16:
    case RFC6330_type::RQ_DEC_32:
    case RFC6330_type::RQ_DEC_64:
    case RFC6330_type::RQ_NONE:
        return nullptr;
    }
    if (raw_ptr == nullptr)
        return nullptr;
    return new RFC6330_ptr (type, raw_ptr);
}

static struct RFC6330_ptr* v1_Decoder (RFC6330_type type,
                                  const RFC6330_OTI_Common_Data common,
                                  const RFC6330_OTI_Scheme_Specific_Data scheme)
{
    void *raw_ptr = nullptr;
    namespace RQ = RFC6330__v1::Impl;
    switch (type) {
    case RFC6330_type::RQ_DEC_8:
        raw_ptr = reinterpret_cast<void *> (
                        new RQ::Decoder<uint8_t*, uint8_t*> (common, scheme));
        break;
    case RFC6330_type::RQ_DEC_16:
        raw_ptr = reinterpret_cast<void *> (
                        new RQ::Decoder<uint16_t*, uint16_t*> (common, scheme));
        break;
    case RFC6330_type::RQ_DEC_32:
        raw_ptr = reinterpret_cast<void *> (
                        new RQ::Decoder<uint32_t*, uint32_t*> (common, scheme));
        break;
    case RFC6330_type::RQ_DEC_64:
        raw_ptr = reinterpret_cast<void *> (
                        new RQ::Decoder<uint64_t*, uint64_t*> (common, scheme));
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        return nullptr;
    }
    if (raw_ptr == nullptr)
        return nullptr;
    return new RFC6330_ptr (type, raw_ptr);
}

static struct RFC6330_ptr* v1_Decoder_raw (RFC6330_type type,
                                                  const uint64_t size,
                                                  const uint16_t symbol_size,
                                                  const uint16_t sub_blocks,
                                                  const uint8_t blocks,
                                                  const uint8_t alignment)
{
    void *raw_ptr = nullptr;
    namespace RQ = RFC6330__v1::Impl;
    switch (type) {
    case RFC6330_type::RQ_DEC_8:
        raw_ptr = reinterpret_cast<void *> (
                        new RQ::Decoder<uint8_t*, uint8_t*> (size, symbol_size,
                                                                sub_blocks,
                                                                blocks,
                                                                alignment));
        break;
    case RFC6330_type::RQ_DEC_16:
        raw_ptr = reinterpret_cast<void *> (
                        new RQ::Decoder<uint16_t*, uint16_t*> (size,symbol_size,
                                                                    sub_blocks,
                                                                    blocks,
                                                                    alignment));
        break;
    case RFC6330_type::RQ_DEC_32:
        raw_ptr = reinterpret_cast<void *> (
                        new RQ::Decoder<uint32_t*, uint32_t*> (size,symbol_size,
                                                                   sub_blocks,
                                                                   blocks,
                                                                   alignment));
        break;
    case RFC6330_type::RQ_DEC_64:
        raw_ptr = reinterpret_cast<void *> (
                        new RQ::Decoder<uint64_t*, uint64_t*> (size,symbol_size,
                                                                   sub_blocks,
                                                                   blocks,
                                                                   alignment));
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        return nullptr;
    }
    if (raw_ptr == nullptr)
        return nullptr;
    return new RFC6330_ptr (type, raw_ptr);
}

static bool v1_initialized (const struct RFC6330_ptr *ptr)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return false;
    switch (ptr->type) {
    case RFC6330_type::RQ_ENC_8:
        return (*reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                                    ptr->ptr));
    case RFC6330_type::RQ_ENC_16:
        return (*reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                                    ptr->ptr));
    case RFC6330_type::RQ_ENC_32:
        return (*reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                                    ptr->ptr));
    case RFC6330_type::RQ_ENC_64:
        return (*reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                                    ptr->ptr));
    case RFC6330_type::RQ_DEC_8:
        return (*reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                                    ptr->ptr));
    case RFC6330_type::RQ_DEC_16:
        return (*reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                                    ptr->ptr));
    case RFC6330_type::RQ_DEC_32:
        return (*reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                                    ptr->ptr));
    case RFC6330_type::RQ_DEC_64:
        return (*reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                                    ptr->ptr));
    case RFC6330_type::RQ_NONE:
        break;
    }
    return false;
}


/////////////////////
// Common functions
/////////////////////

static uint8_t v1_blocks (const struct RFC6330_ptr *ptr)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return 0;
    switch (ptr->type) {
    case RFC6330_type::RQ_ENC_8:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                        ptr->ptr))->blocks();
    case RFC6330_type::RQ_ENC_16:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                        ptr->ptr))->blocks();
    case RFC6330_type::RQ_ENC_32:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                        ptr->ptr))->blocks();
    case RFC6330_type::RQ_ENC_64:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                        ptr->ptr))->blocks();
    case RFC6330_type::RQ_DEC_8:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        ptr->ptr))->blocks();
    case RFC6330_type::RQ_DEC_16:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        ptr->ptr))->blocks();
    case RFC6330_type::RQ_DEC_32:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        ptr->ptr))->blocks();
    case RFC6330_type::RQ_DEC_64:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                        ptr->ptr))->blocks();
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static uint16_t v1_symbols (const struct RFC6330_ptr *ptr, const uint8_t sbn)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return 0;
    switch (ptr->type) {
    case RFC6330_type::RQ_ENC_8:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    ptr->ptr))->symbols (sbn);
    case RFC6330_type::RQ_ENC_16:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    ptr->ptr))->symbols (sbn);
    case RFC6330_type::RQ_ENC_32:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    ptr->ptr))->symbols (sbn);
    case RFC6330_type::RQ_ENC_64:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                    ptr->ptr))->symbols (sbn);
    case RFC6330_type::RQ_DEC_8:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    ptr->ptr))->symbols (sbn);
    case RFC6330_type::RQ_DEC_16:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    ptr->ptr))->symbols (sbn);
    case RFC6330_type::RQ_DEC_32:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    ptr->ptr))->symbols (sbn);
    case RFC6330_type::RQ_DEC_64:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                    ptr->ptr))->symbols (sbn);
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static RFC6330_Block_Size v1_extended_symbols (const struct RFC6330_ptr *ptr,
                                                            const uint8_t sbn)
{
    RFC6330__v1::Block_Size ret = static_cast<RFC6330__v1::Block_Size> (0);
    if (ptr == nullptr || ptr->ptr == nullptr)
        return static_cast<RFC6330_Block_Size> (ret);
    switch (ptr->type) {
    case RFC6330_type::RQ_ENC_8:
        ret = (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                            ptr->ptr))->extended_symbols (sbn);
        break;
    case RFC6330_type::RQ_ENC_16:
        ret = (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                            ptr->ptr))->extended_symbols (sbn);
        break;
    case RFC6330_type::RQ_ENC_32:
        ret = (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                            ptr->ptr))->extended_symbols (sbn);
        break;
    case RFC6330_type::RQ_ENC_64:
        ret = (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                            ptr->ptr))->extended_symbols (sbn);
        break;
    case RFC6330_type::RQ_DEC_8:
        ret = (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                            ptr->ptr))->extended_symbols (sbn);
        break;
    case RFC6330_type::RQ_DEC_16:
        ret = (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                            ptr->ptr))->extended_symbols (sbn);
        break;
    case RFC6330_type::RQ_DEC_32:
        ret = (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                            ptr->ptr))->extended_symbols (sbn);
        break;
    case RFC6330_type::RQ_DEC_64:
        ret = (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                            ptr->ptr))->extended_symbols (sbn);
        break;
    case RFC6330_type::RQ_NONE:
        break;
    }
    return static_cast<RFC6330_Block_Size> (ret);
}

static size_t v1_symbol_size (const struct RFC6330_ptr *ptr)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return 0;
    switch (ptr->type) {
    case RFC6330_type::RQ_ENC_8:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RFC6330_type::RQ_ENC_16:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RFC6330_type::RQ_ENC_32:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RFC6330_type::RQ_ENC_64:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RFC6330_type::RQ_DEC_8:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RFC6330_type::RQ_DEC_16:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RFC6330_type::RQ_DEC_32:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RFC6330_type::RQ_DEC_64:
        return (reinterpret_cast<const
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static RFC6330_Error v1_future_state (const struct RFC6330_future *f)
{
    if (f == nullptr)
        return RFC6330_Error::RQ_ERR_WRONG_INPUT;

    if (f->f.valid()) {
        if ( f->f.wait_for(std::chrono::microseconds(0)) == std::future_status::ready)
            return RFC6330_Error::RQ_ERR_NONE; // ready
        return RFC6330_Error::RQ_ERR_WORKING; // working
    }
    // future to something that has already been get()
    return RaptorQ_Error::RQ_ERR_NOT_NEEDED;
}

static RFC6330_Error v1_future_wait_for (const struct RFC6330_future *f,
                                                const uint64_t time,
                                                const RFC6330_Unit_Time unit)
{
    if (f == nullptr)
        return RQ_ERR_WRONG_INPUT;
    std::future_status status = std::future_status::timeout;
    switch (unit) {
    case RQ_TIME_NANOSEC:
        status = f->f.wait_for (std::chrono::nanoseconds (time));
        break;
    case RQ_TIME_MICROSEC:
        status = f->f.wait_for (std::chrono::microseconds (time));
        break;
    case RQ_TIME_MILLISEC:
        status = f->f.wait_for (std::chrono::milliseconds (time));
        break;
    case RQ_TIME_SEC:
        status = f->f.wait_for (std::chrono::seconds (time));
        break;
    case RQ_TIME_MIN:
        status = f->f.wait_for (std::chrono::minutes (time));
        break;
    case RQ_TIME_HOUR:
        status = f->f.wait_for (std::chrono::hours (time));
        break;
    }
    if (status == std::future_status::ready)
        return RaptorQ_Error::RQ_ERR_NONE;
    return RaptorQ_Error::RQ_ERR_WORKING;
}

static void v1_future_wait (const struct RFC6330_future *f)
{
    if (f == nullptr)
        return;
    f->f.wait();
}

static void v1_future_free (struct RFC6330_future **f)
{
    if (f == nullptr || *f == nullptr)
        return;

    delete *f;
    *f = nullptr;
}

static struct RFC6330_Result v1_future_get (struct RFC6330_future *const future)
{
    RFC6330_Result res = {RQ_ERR_WRONG_INPUT, 0 };
    if (future != nullptr && future->f.valid()) {
        const auto cpp_res = future->f.get();
        return RFC6330_Result {static_cast<RFC6330_Error> (cpp_res.first),
                                                                cpp_res.second};
    }
    return res;
}

static bool v1_set_thread_pool (const size_t threads,
                                        const uint16_t max_block_concurrency,
                                        const RFC6330_Work exit_type)
{
    return RFC6330__v1::set_thread_pool(threads, max_block_concurrency,
                            static_cast<RaptorQ__v1::Work_State> (exit_type));
}

static struct RFC6330_future* v1_compute (const struct RFC6330_ptr *ptr,
                                                 const RFC6330_Compute flags)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return nullptr;
    std::unique_ptr<RFC6330_future> ret = std::unique_ptr<RFC6330_future> (
                                                        new RFC6330_future());
    switch (ptr->type) {
    case RFC6330_type::RQ_ENC_8:
        ret->f = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                ptr->ptr))->compute(static_cast<RFC6330__v1::Compute> (flags));
        return ret.release();
    case RFC6330_type::RQ_ENC_16:
        ret->f = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                ptr->ptr))->compute(static_cast<RFC6330__v1::Compute> (flags));
        return ret.release();
    case RFC6330_type::RQ_ENC_32:
        ret->f = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                ptr->ptr))->compute(static_cast<RFC6330__v1::Compute> (flags));
        return ret.release();
    case RFC6330_type::RQ_ENC_64:
        ret->f = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                ptr->ptr))->compute(static_cast<RFC6330__v1::Compute> (flags));
        return ret.release();
    case RFC6330_type::RQ_DEC_8:
        ret->f = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                ptr->ptr))->compute(static_cast<RFC6330__v1::Compute> (flags));
        return ret.release();
    case RFC6330_type::RQ_DEC_16:
        ret->f = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                ptr->ptr))->compute(static_cast<RFC6330__v1::Compute> (flags));
        return ret.release();
    case RFC6330_type::RQ_DEC_32:
        ret->f = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                ptr->ptr))->compute(static_cast<RFC6330__v1::Compute> (flags));
        return ret.release();
    case RFC6330_type::RQ_DEC_64:
        ret->f = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                ptr->ptr))->compute(static_cast<RFC6330__v1::Compute> (flags));
        return ret.release();
    case RFC6330_type::RQ_NONE:
        break;
    }
    return nullptr;
}

void v1_free (struct RFC6330_ptr **ptr)
{
    if (ptr == nullptr || *ptr == nullptr)
        return;
    if ((*ptr)->ptr == nullptr) {
        delete *ptr;
        return;
    }
    switch ((*ptr)->type) {
    case RFC6330_type::RQ_ENC_8:
        delete reinterpret_cast<
                RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> ((*ptr)->ptr);
        break;
    case RFC6330_type::RQ_ENC_16:
        delete reinterpret_cast<
                RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*>((*ptr)->ptr);
        break;
    case RFC6330_type::RQ_ENC_32:
        delete reinterpret_cast<
                RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*>((*ptr)->ptr);
        break;
    case RFC6330_type::RQ_ENC_64:
        delete reinterpret_cast<
                RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*>((*ptr)->ptr);
        break;
    case RFC6330_type::RQ_DEC_8:
        delete reinterpret_cast<
                RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> ((*ptr)->ptr);
        break;
    case RFC6330_type::RQ_DEC_16:
        delete reinterpret_cast<
                RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*>((*ptr)->ptr);
        break;
    case RFC6330_type::RQ_DEC_32:
        delete reinterpret_cast<
                RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*>((*ptr)->ptr);
        break;
    case RFC6330_type::RQ_DEC_64:
        delete reinterpret_cast<
                RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*>((*ptr)->ptr);
        break;
    case RFC6330_type::RQ_NONE:
        break;
    }
    delete *ptr;
    *ptr = nullptr;
}

static void v1_free_block (const struct RFC6330_ptr *ptr, const uint8_t sbn)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return;
    switch (ptr->type) {
    case RFC6330_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                        ptr->ptr))->free (sbn);
    case RFC6330_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                        ptr->ptr))->free (sbn);
    case RFC6330_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                        ptr->ptr))->free (sbn);
    case RFC6330_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                        ptr->ptr))->free (sbn);
    case RFC6330_type::RQ_DEC_8:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        ptr->ptr))->free (sbn);
    case RFC6330_type::RQ_DEC_16:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        ptr->ptr))->free (sbn);
    case RFC6330_type::RQ_DEC_32:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        ptr->ptr))->free (sbn);
    case RFC6330_type::RQ_DEC_64:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                        ptr->ptr))->free (sbn);
    case RFC6330_type::RQ_NONE:
        break;
    }
    return;
}


/////////////
// Encoding
/////////////


static RFC6330_OTI_Common_Data v1_OTI_Common (const struct RFC6330_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return 0;
    switch (enc->type) {
    case RFC6330_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    enc->ptr))->OTI_Common();
    case RFC6330_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    enc->ptr))->OTI_Common();
    case RFC6330_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    enc->ptr))->OTI_Common();
    case RFC6330_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                    enc->ptr))->OTI_Common();
    case RFC6330_type::RQ_DEC_8:
    case RFC6330_type::RQ_DEC_16:
    case RFC6330_type::RQ_DEC_32:
    case RFC6330_type::RQ_DEC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static RFC6330_OTI_Scheme_Specific_Data v1_OTI_Scheme_Specific (
                                                const struct RFC6330_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return 0;
    switch (enc->type) {
    case RFC6330_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                            enc->ptr))->OTI_Scheme_Specific();
    case RFC6330_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                            enc->ptr))->OTI_Scheme_Specific();
    case RFC6330_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                            enc->ptr))->OTI_Scheme_Specific();
    case RFC6330_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                            enc->ptr))->OTI_Scheme_Specific();
    case RFC6330_type::RQ_DEC_8:
    case RFC6330_type::RQ_DEC_16:
    case RFC6330_type::RQ_DEC_32:
    case RFC6330_type::RQ_DEC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static uint32_t v1_max_repair (const struct RFC6330_ptr *enc, const uint8_t sbn)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return 0;
    switch (enc->type) {
    case RFC6330_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                enc->ptr))->max_repair (sbn);
    case RFC6330_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                enc->ptr))->max_repair (sbn);
    case RFC6330_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                enc->ptr))->max_repair (sbn);
    case RFC6330_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                enc->ptr))->max_repair (sbn);
    case RFC6330_type::RQ_DEC_8:
    case RFC6330_type::RQ_DEC_16:
    case RFC6330_type::RQ_DEC_32:
    case RFC6330_type::RQ_DEC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static size_t v1_precompute_max_memory (const struct RFC6330_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return 0;
    switch (enc->type) {
    case RFC6330_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                            enc->ptr))->precompute_max_memory();
    case RFC6330_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                            enc->ptr))->precompute_max_memory();
    case RFC6330_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                            enc->ptr))->precompute_max_memory();
    case RFC6330_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                            enc->ptr))->precompute_max_memory();
    case RFC6330_type::RQ_DEC_8:
    case RFC6330_type::RQ_DEC_16:
    case RFC6330_type::RQ_DEC_32:
    case RFC6330_type::RQ_DEC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static size_t v1_encode_id (const struct RFC6330_ptr *enc, void **data,
                                                            const size_t size,
                                                            const uint32_t id)
{
    if (enc == nullptr || enc->ptr == nullptr || data == nullptr)
        return 0;
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    size_t ret;
    switch (enc->type) {
    case RFC6330_type::RQ_ENC_8:
        p_8 = reinterpret_cast<uint8_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    enc->ptr))->encode (
                                                        p_8, p_8 + size,id);
        *data = p_8;
        return ret;
    case RFC6330_type::RQ_ENC_16:
        p_16 = reinterpret_cast<uint16_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    enc->ptr))->encode (
                                                        p_16, p_16 + size,id);
        *data = p_16;
        return ret;
    case RFC6330_type::RQ_ENC_32:
        p_32 = reinterpret_cast<uint32_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    enc->ptr))->encode (
                                                        p_32, p_32 + size,id);
        *data = p_32;
        return ret;
    case RFC6330_type::RQ_ENC_64:
        p_64 = reinterpret_cast<uint64_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                    enc->ptr))->encode (
                                                        p_64, p_64 + size,id);
        *data = p_64;
        return ret;
    case RFC6330_type::RQ_DEC_8:
    case RFC6330_type::RQ_DEC_16:
    case RFC6330_type::RQ_DEC_32:
    case RFC6330_type::RQ_DEC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static size_t v1_encode (const struct RFC6330_ptr *enc, void **data,
                                                            const size_t size,
                                                            const uint32_t esi,
                                                            const uint8_t sbn)
{
    if (enc == nullptr || enc->ptr == nullptr || data == nullptr)
        return 0;
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    size_t ret;
    switch (enc->type) {
    case RFC6330_type::RQ_ENC_8:
        p_8 = reinterpret_cast<uint8_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                        enc->ptr))->encode (
                                                                p_8, p_8 + size,
                                                                    esi, sbn);
        *data = p_8;
        return ret;
    case RFC6330_type::RQ_ENC_16:
        p_16 = reinterpret_cast<uint16_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                        enc->ptr))->encode (
                                                            p_16, p_16 + size,
                                                                    esi, sbn);
        *data = p_16;
        return ret;
    case RFC6330_type::RQ_ENC_32:
        p_32 = reinterpret_cast<uint32_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                        enc->ptr))->encode (
                                                            p_32, p_32 + size,
                                                                    esi, sbn);
        *data = p_32;
        return ret;
    case RFC6330_type::RQ_ENC_64:
        p_64 = reinterpret_cast<uint64_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                        enc->ptr))->encode (
                                                            p_64, p_64 + size,
                                                                    esi, sbn);
        *data = p_64;
        return ret;
    case RFC6330_type::RQ_DEC_8:
    case RFC6330_type::RQ_DEC_16:
    case RFC6330_type::RQ_DEC_32:
    case RFC6330_type::RQ_DEC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static uint32_t v1_id (const uint32_t esi, const uint8_t sbn)
{
    uint32_t ret = static_cast<uint32_t> (sbn) << 24;
    ret += esi & 0x00FFFFFF;
    return RaptorQ__v1::Impl::Endian::h_to_b<uint32_t> (ret);
}



/////////////
// Decoding
/////////////

static struct RFC6330_Byte_Tracker v1_end_of_input (
                                            const struct RFC6330_ptr *dec,
                                            const RFC6330_Fill_With_Zeros fill)
{
    struct RFC6330_Byte_Tracker ret;
    ret.length = 0;
    ret.bitmask = nullptr;

    if (dec == nullptr || dec->ptr == nullptr)
        return ret;

    RFC6330__v1::Fill_With_Zeros cpp_fill =
                            static_cast<RFC6330__v1::Fill_With_Zeros> (fill);
    std::vector<bool> cpp_res;
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                            dec->ptr))->end_of_input (cpp_fill);
        break;
    case RFC6330_type::RQ_DEC_16:
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                            dec->ptr))->end_of_input (cpp_fill);
        break;
    case RFC6330_type::RQ_DEC_32:
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                            dec->ptr))->end_of_input (cpp_fill);
        break;
    case RFC6330_type::RQ_DEC_64:
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                            dec->ptr))->end_of_input (cpp_fill);
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    if (cpp_res.size() != 0) {
        size_t size = cpp_res.size() / 8;
        if ((cpp_res.size() % 8) != 0)
            ++size;
        ret.length = size;
        ret.bitmask = reinterpret_cast<uint8_t*> (calloc (size, 1));
        size_t idx = 0;
        for (const auto b : cpp_res) {
            if (b) {
                const size_t byte_idx = idx / 8;
                uint8_t mask = 0x01;
                mask <<= (byte_idx % 8);
                ret.bitmask[byte_idx] |= mask;
            }
            ++idx;
        }
    }
    return ret;
}

static struct RFC6330_Byte_Tracker v1_end_of_block_input (
                                            const struct RFC6330_ptr *dec,
                                            const RFC6330_Fill_With_Zeros fill,
                                            const uint8_t block)
{
    struct RFC6330_Byte_Tracker ret;
    ret.length = 0;
    ret.bitmask = nullptr;


    if (dec == nullptr || dec->ptr == nullptr)
        return ret;

    RFC6330__v1::Fill_With_Zeros cpp_fill =
                            static_cast<RFC6330__v1::Fill_With_Zeros> (fill);
    std::vector<bool> cpp_res;
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                    dec->ptr))->end_of_input (cpp_fill, block);
        break;
    case RFC6330_type::RQ_DEC_16:
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                    dec->ptr))->end_of_input (cpp_fill, block);
        break;
    case RFC6330_type::RQ_DEC_32:
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                    dec->ptr))->end_of_input (cpp_fill, block);
        break;
    case RFC6330_type::RQ_DEC_64:
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                    dec->ptr))->end_of_input (cpp_fill, block);
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    if (cpp_res.size() != 0) {
        size_t size = cpp_res.size() / 8;
        if ((cpp_res.size() % 8) != 0)
            ++size;
        ret.length = size;
        ret.bitmask = reinterpret_cast<uint8_t*> (calloc (size, 1));
        size_t idx = 0;
        for (const auto b : cpp_res) {
            if (b) {
                const size_t byte_idx = idx / 8;
                uint8_t mask = 0x01;
                mask <<= (byte_idx % 8);
                ret.bitmask[byte_idx] |= mask;
            }
            ++idx;
        }
    }
    return ret;
}

static uint64_t v1_bytes (const struct RFC6330_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return 0;
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                            dec->ptr))->bytes();
    case RFC6330_type::RQ_DEC_16:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                            dec->ptr))->bytes();
    case RFC6330_type::RQ_DEC_32:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                            dec->ptr))->bytes();
    case RFC6330_type::RQ_DEC_64:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                            dec->ptr))->bytes();
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static uint8_t v1_blocks_ready (const struct RFC6330_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return 0;
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    dec->ptr))->blocks_ready();
    case RFC6330_type::RQ_DEC_16:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    dec->ptr))->blocks_ready();
    case RFC6330_type::RQ_DEC_32:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    dec->ptr))->blocks_ready();
    case RFC6330_type::RQ_DEC_64:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                    dec->ptr))->blocks_ready();
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return 0;
}

static bool v1_is_ready (const struct RFC6330_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return false;
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        dec->ptr))->is_ready();
    case RFC6330_type::RQ_DEC_16:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        dec->ptr))->is_ready();
    case RFC6330_type::RQ_DEC_32:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        dec->ptr))->is_ready();
    case RFC6330_type::RQ_DEC_64:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                        dec->ptr))->is_ready();
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return false;
}

static bool v1_is_block_ready (const struct RFC6330_ptr *dec,
                                                        const uint8_t block)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return false;
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                            dec->ptr))->is_block_ready (block);
    case RFC6330_type::RQ_DEC_16:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                            dec->ptr))->is_block_ready (block);
    case RFC6330_type::RQ_DEC_32:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                            dec->ptr))->is_block_ready (block);
    case RFC6330_type::RQ_DEC_64:
        return (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                            dec->ptr))->is_block_ready (block);
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return false;
}

static struct RFC6330_Dec_Result v1_decode_aligned (
                                                const struct RFC6330_ptr *dec,
                                                void **data,
                                                const uint64_t size,
                                                const uint8_t skip)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    if (dec == nullptr || dec->ptr == nullptr ||
                                        data == nullptr || *data == nullptr) {
        return RFC6330_Dec_Result {0, 0};
    }
    RFC6330__v1::Decoder_written cpp_res {0, 0};
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*data);
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    dec->ptr))->decode_aligned (
                                                            p_8, p_8 + size,
                                                                        skip);
        *data = p_8;
        break;
    case RFC6330_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*data);
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    dec->ptr))->decode_aligned (
                                                            p_16, p_16 + size,
                                                                        skip);
        *data = p_16;
        break;
    case RFC6330_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*data);
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    dec->ptr))->decode_aligned (
                                                            p_32, p_32 + size,
                                                                        skip);
        *data = p_32;
        break;
    case RFC6330_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*data);
        cpp_res = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                    dec->ptr))->decode_aligned (
                                                            p_64, p_64 + size,
                                                                        skip);
        *data = p_64;
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return RFC6330_Dec_Result {cpp_res.written, cpp_res.offset};
}

static struct RFC6330_Dec_Result v1_decode_block_aligned(
                                                const struct RFC6330_ptr *dec,
                                                void **data,
                                                const size_t size,
                                                const uint8_t skip,
                                                const uint8_t sbn)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    if (dec == nullptr || dec->ptr == nullptr ||
                                        data == nullptr || *data == nullptr) {
        return RFC6330_Dec_Result {0, 0};
    }
    RFC6330__v1::Decoder_written ret = {0, 0};
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                            dec->ptr))->decode_block_aligned (
                                                            p_8, p_8 + size,
                                                                    skip, sbn);
        *data = p_8;
        break;
    case RFC6330_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                            dec->ptr))->decode_block_aligned (
                                                            p_16, p_16 + size,
                                                                    skip, sbn);
        *data = p_16;
        break;
    case RFC6330_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                            dec->ptr))->decode_block_aligned (
                                                            p_32, p_32 + size,
                                                                    skip, sbn);
        *data = p_32;
        break;
    case RFC6330_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                            dec->ptr))->decode_block_aligned (
                                                            p_64, p_64 + size,
                                                                    skip, sbn);
        *data = p_64;
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return RFC6330_Dec_Result {ret.written, ret.offset};
}

static uint64_t v1_decode_symbol (const struct RFC6330_ptr *dec, void **data,
                                                            const size_t size,
                                                            const uint16_t esi,
                                                            const uint8_t sbn)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    uint64_t ret = 0;
    if (dec == nullptr || dec->ptr == nullptr ||
                                        data == nullptr || *data == nullptr) {
        return 0;
    }
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    dec->ptr))->decode_symbol (
                                                            p_8, p_8 + size,
                                                                    esi, sbn);
        *data = p_8;
        break;
    case RFC6330_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    dec->ptr))->decode_symbol (
                                                            p_16, p_16 + size,
                                                                    esi, sbn);
        *data = p_16;
        break;
    case RFC6330_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    dec->ptr))->decode_symbol (
                                                            p_32, p_32 + size,
                                                                    esi, sbn);
        *data = p_32;
        break;
    case RFC6330_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                    dec->ptr))->decode_symbol (
                                                            p_64, p_64 + size,
                                                                    esi,sbn);
        *data = p_64;
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return ret;
}

static uint64_t v1_decode_bytes (const struct RFC6330_ptr *dec, void **data,
                                                            const uint64_t size,
                                                            const uint8_t skip)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    uint64_t ret = 0;
    if (dec == nullptr || dec->ptr == nullptr ||
                                        data == nullptr || *data == nullptr) {
        return 0;
    }
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    dec->ptr))->decode_bytes (
                                                            p_8, p_8 + size,
                                                                        skip);
        *data = p_8;
        break;
    case RFC6330_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    dec->ptr))->decode_bytes (
                                                            p_16, p_16 + size,
                                                                        skip);
        *data = p_16;
        break;
    case RFC6330_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    dec->ptr))->decode_bytes (
                                                            p_32, p_32 + size,
                                                                        skip);
        *data = p_32;
        break;
    case RFC6330_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                    dec->ptr))->decode_bytes (
                                                            p_64, p_64 + size,
                                                                        skip);
        *data = p_64;
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return ret;
}

static size_t v1_decode_block_bytes (const struct RFC6330_ptr *dec, void **data,
                                                            const size_t size,
                                                            const uint8_t skip,
                                                            const uint8_t sbn)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    size_t ret = 0;
    if (dec == nullptr || dec->ptr == nullptr ||
                                            data == nullptr || *data == nullptr) {
        return ret;
    }
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                            dec->ptr))->decode_block_bytes (
                                                            p_8, p_8 + size,
                                                                    skip, sbn);
        *data = p_8;
        break;
    case RFC6330_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                            dec->ptr))->decode_block_bytes (
                                                            p_16, p_16 + size,
                                                                    skip, sbn);
        *data = p_16;
        break;
    case RFC6330_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                            dec->ptr))->decode_block_bytes (
                                                            p_32, p_32 + size,
                                                                    skip, sbn);
        *data = p_32;
        break;
    case RFC6330_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*data);
        ret = (reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                            dec->ptr))->decode_block_bytes (
                                                            p_64, p_64 + size,
                                                                    skip, sbn);
        *data = p_64;
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return ret;
}

static RFC6330_Error v1_add_symbol_id (const struct RFC6330_ptr *dec,
                                                            void **data,
                                                            const uint32_t size,
                                                            const uint32_t id)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    RFC6330_Error ret = RFC6330_Error::RQ_ERR_WRONG_INPUT;
    if (dec == nullptr || dec->ptr == nullptr || data == nullptr ||
                                                            *data == nullptr) {
        return ret;
    }
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*data);
        ret = static_cast<RFC6330_Error> ((reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        dec->ptr))->add_symbol (
                                                            p_8, p_8 + size,
                                                                        id));
        *data = p_8;
        break;
    case RFC6330_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*data);
        ret = static_cast<RFC6330_Error> ((reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        dec->ptr))->add_symbol (
                                                              p_16, p_16 + size,
                                                                          id));
        *data = p_16;
        break;
    case RFC6330_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*data);
        ret = static_cast<RFC6330_Error> ((reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        dec->ptr))->add_symbol (
                                                              p_32, p_32 + size,
                                                                          id));
        *data = p_32;
        break;
    case RFC6330_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*data);
        ret = static_cast<RFC6330_Error> ((reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                        dec->ptr))->add_symbol (
                                                              p_64, p_64 + size,
                                                                          id));
        *data = p_64;
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return ret;
}

static RFC6330_Error v1_add_symbol (const struct RFC6330_ptr *dec, void **data,
                                                            const uint32_t size,
                                                            const uint32_t esi,
                                                            const uint8_t sbn)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    RFC6330_Error ret = RFC6330_Error::RQ_ERR_WRONG_INPUT;
    if (dec == nullptr || dec->ptr == nullptr || data == nullptr ||
                                                            *data == nullptr) {
        return ret;
    }
    switch (dec->type) {
    case RFC6330_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*data);
        ret = static_cast<RFC6330_Error> ((reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        dec->ptr))->add_symbol (
                                                            p_8, p_8 + size,
                                                                    esi, sbn));
        *data = p_8;
        break;
    case RFC6330_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*data);
        ret = static_cast<RFC6330_Error> ((reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        dec->ptr))->add_symbol (
                                                              p_16, p_16 + size,
                                                                    esi, sbn));
        *data = p_16;
        break;
    case RFC6330_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*data);
        ret = static_cast<RFC6330_Error> ((reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        dec->ptr))->add_symbol (
                                                              p_32, p_32 + size,
                                                                    esi, sbn));
        *data = p_32;
        break;
    case RFC6330_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*data);
        ret = static_cast<RFC6330_Error> ((reinterpret_cast<
                            RFC6330__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                        dec->ptr))->add_symbol (
                                                              p_64, p_64 + size,
                                                                    esi, sbn));
        *data = p_64;
        break;
    case RFC6330_type::RQ_ENC_8:
    case RFC6330_type::RQ_ENC_16:
    case RFC6330_type::RQ_ENC_32:
    case RFC6330_type::RQ_ENC_64:
    case RFC6330_type::RQ_NONE:
        break;
    }
    return ret;
}
