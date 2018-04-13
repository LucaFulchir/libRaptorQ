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


#include "RaptorQ/v1/wrapper/C_RAW_API.h"
#include "RaptorQ/v1/RaptorQ.hpp"
#include <future>
#include <utility>

struct RAPTORQ_LOCAL RaptorQ_ptr
{
    void *const ptr;
    const RaptorQ_type type;

    RaptorQ_ptr ()
        : ptr (nullptr), type (RaptorQ_type::RQ_NONE) {}
    RaptorQ_ptr (const RaptorQ_type _type, void *const _ptr)
        : ptr (_ptr), type (_type) {}
};


typedef enum {
    RQ_FUTURE_ENCODER = 1,
    RQ_FUTURE_DECODER = 2
} RAPTORQ_LOCAL RaptorQ_Future_Type;

struct RAPTORQ_LOCAL RaptorQ_future
{
    const RaptorQ_Future_Type type;
    RaptorQ_future (const RaptorQ_Future_Type t)
        :type (t) {}
};

struct RAPTORQ_LOCAL RaptorQ_future_enc
{
    const struct RaptorQ_future base;
    std::shared_future<RaptorQ__v1::Error> f;
    RaptorQ_future_enc ()
        :base (RaptorQ_Future_Type::RQ_FUTURE_ENCODER) {}
};

struct RAPTORQ_LOCAL RaptorQ_future_dec
{
    const struct RaptorQ_future base;
    std::future<struct RaptorQ__v1::Decoder_wait_res> f;
    RaptorQ_future_dec ()
        :base (RaptorQ_Future_Type::RQ_FUTURE_DECODER) {}
};


////////////////////////
////
//// For ease of development please keep the function declaration
//// and implementation in the same orders as defined in the structs!
////
////////////////////////

// precomputation caching
static RaptorQ_Compress v1_supported_compressions();
static RaptorQ_Compress v1_get_compression();
static bool v1_set_compression (const RaptorQ_Compress compression);
static size_t v1_local_cache_size (const size_t local_cache);
static size_t v1_get_local_cache_size ();

// constructors
static struct RaptorQ_ptr* v1_Encoder (RaptorQ_type type,
                                            const RaptorQ_Block_Size symbols,
                                            const size_t symbol_size);
static struct RaptorQ_ptr* v1_Decoder (RaptorQ_type type,
                                            const RaptorQ_Block_Size symbols,
                                            const size_t symbol_size,
                                            const RQ_Dec_Report report);
static bool v1_initialized (const RaptorQ_ptr *ptr);

// common functions
static uint16_t v1_symbols (const RaptorQ_ptr *ptr);
static size_t v1_symbol_size (const RaptorQ_ptr *ptr);
static void v1_stop (const RaptorQ_ptr *ptr);
static RaptorQ_Error v1_future_state (struct RaptorQ_future *const f);
static RaptorQ_Error v1_future_wait_for (struct RaptorQ_future *const f,
                                                const uint64_t time,
                                                const RaptorQ_Unit_Time unit);
static void v1_future_wait (struct RaptorQ_future *const f);
static void v1_future_free (struct RaptorQ_future **f);
static void v1_free (struct RaptorQ_ptr **ptr);
static bool v1_ready (const RaptorQ_ptr *ptr);

// encoder-specific
static uint32_t v1_max_repair  (const RaptorQ_ptr *enc);
static size_t v1_set_data (const RaptorQ_ptr *enc, void **from,
                                                            const size_t size);
static bool v1_has_data (const RaptorQ_ptr *enc);
static void v1_clear_data (const RaptorQ_ptr *ptr);
static bool v1_precompute_sync (const RaptorQ_ptr *enc);
static bool v1_compute_sync (const RaptorQ_ptr *enc);
static RaptorQ_future_enc* v1_precompute (const RaptorQ_ptr *enc);
static RaptorQ_future_enc* v1_compute (const RaptorQ_ptr *enc);
static RaptorQ_Error v1_enc_future_get (struct RaptorQ_future_enc *f);
static size_t v1_encode (const RaptorQ_ptr *enc, void **from, const size_t size,
                                                            const uint32_t id);

// decoder-specific
static RaptorQ_Error v1_add_symbol (const RaptorQ_ptr *dec, void **from,
                                                            const size_t size,
                                                            const uint32_t esi);
static bool v1_can_decode (const RaptorQ_ptr *dec);
static uint16_t v1_needed_symbols (const RaptorQ_ptr *dec);
static RaptorQ_Dec_wait_res v1_poll (const RaptorQ_ptr *dec);
static RaptorQ_Dec_wait_res v1_wait_sync (const RaptorQ_ptr *dec);
static RaptorQ_future_dec* v1_wait (const RaptorQ_ptr *dec);
static RaptorQ_Dec_wait_res v1_dec_future_get (struct RaptorQ_future_dec *f);
static struct RaptorQ_Byte_Tracker v1_end_of_input (struct RaptorQ_ptr *dec,
                                            const RaptorQ_Fill_With_Zeros fill);
static RaptorQ_Decoder_Result v1_decode_once (struct RaptorQ_ptr *dec);
static RaptorQ_Error v1_decode_symbol (struct RaptorQ_ptr *dec, void** start,
                                        const size_t size, const uint16_t esi);
static RaptorQ_Dec_Written v1_decode_bytes (struct RaptorQ_ptr *dec,
                                                        void **start,
                                                        const size_t size,
                                                        const size_t from_byte,
                                                        const size_t skip);


void RaptorQ_free_api (struct RaptorQ_base_api **api)
{
    if (api == nullptr || *api == nullptr)
        return;
    if ((*api)->version == 1) {
        delete reinterpret_cast<struct RaptorQ_v1*> (*api);
    } // else it's all your fault anway
    *api = nullptr;
}

struct RaptorQ_base_api* RaptorQ_api (uint32_t version)
{
    if (version != 1)
        return nullptr;
    return reinterpret_cast<RaptorQ_base_api*> (new RaptorQ_v1());
}

RaptorQ_v1::RaptorQ_v1()
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
    initialized (&v1_initialized),

    // common functions
    symbols (&v1_symbols),
    symbol_size (&v1_symbol_size),
    stop (&v1_stop),
    future_state (&v1_future_state),
    future_wait_for (&v1_future_wait_for),
    future_wait (&v1_future_wait),
    future_free (&v1_future_free),
    free (&v1_free),
    ready (&v1_ready),

    // encoder-specific functions
    max_repair (&v1_max_repair),
    set_data (&v1_set_data),
    has_data (&v1_has_data),
    clear_data (&v1_clear_data),
    precompute_sync (&v1_precompute_sync),
    compute_sync (&v1_compute_sync),
    precompute (&v1_precompute),
    compute (&v1_compute),
    enc_future_get (&v1_enc_future_get),
    encode (&v1_encode),

    // decoder-specific functions
    add_symbol (&v1_add_symbol),
    can_decode (&v1_can_decode),
    needed_symbols (&v1_needed_symbols),
    poll (&v1_poll),
    wait_sync (&v1_wait_sync),
    wait (&v1_wait),
    dec_future_get (&v1_dec_future_get),
    end_of_input (&v1_end_of_input),
    decode_once (&v1_decode_once),
    decode_symbol (&v1_decode_symbol),
    decode_bytes (&v1_decode_bytes)
{}

///////////////////////////
// Precomputation caching
///////////////////////////

static RaptorQ_Compress v1_supported_compressions()
{
    return static_cast<RaptorQ_Compress>(RaptorQ__v1::supported_compressions());
}

static RaptorQ_Compress v1_get_compression()
    { return static_cast<RaptorQ_Compress>(RaptorQ__v1::get_compression()); }

static bool v1_set_compression (const RaptorQ_Compress compression)
{
    return static_cast<RaptorQ_Compress> (RaptorQ__v1::set_compression (
                            static_cast<RaptorQ__v1::Compress> (compression)));
}

static size_t v1_local_cache_size (const size_t local_cache)
    { return RaptorQ__v1::local_cache_size (local_cache); }

static size_t v1_get_local_cache_size ()
    { return RFC6330__v1::get_local_cache_size(); }


/////////////////////
// Constructors
/////////////////////

static struct RaptorQ_ptr* v1_Encoder (RaptorQ_type type,
                                            const RaptorQ_Block_Size symbols,
                                            const size_t symbol_size)
{
    void *raw_ptr = nullptr;
    switch (type) {
    case RaptorQ_type::RQ_ENC_8:
        raw_ptr = reinterpret_cast<void *> (
                    new RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                                                symbol_size));
        break;
    case RaptorQ_type::RQ_ENC_16:
        raw_ptr = reinterpret_cast<void *> (
                    new RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                                                symbol_size));
        break;
    case RaptorQ_type::RQ_ENC_32:
        raw_ptr = reinterpret_cast<void *> (
                    new RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                                                symbol_size));
        break;
    case RaptorQ_type::RQ_ENC_64:
        raw_ptr = reinterpret_cast<void *> (
                    new RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                                                symbol_size));
        break;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        return nullptr;
    }
    if (raw_ptr == nullptr)
        return nullptr;
    return new RaptorQ_ptr (type, raw_ptr);
}

static struct RaptorQ_ptr* v1_Decoder (RaptorQ_type type,
                                            const RaptorQ_Block_Size symbols,
                                            const size_t symbol_size,
                                            const RQ_Dec_Report report)
{
    const auto _report = static_cast<RFC6330_Compute> (report);
    if (_report != RQ_COMPUTE_PARTIAL_FROM_BEGINNING &&
        _report != RQ_COMPUTE_PARTIAL_ANY &&
        _report != RQ_COMPUTE_COMPLETE) {
        return nullptr;
    }

    void *raw_ptr = nullptr;
    namespace RQ = RaptorQ__v1::Impl;
    switch (type) {
    case RaptorQ_type::RQ_DEC_8:
        raw_ptr = reinterpret_cast<void *> (
                            new RQ::Decoder<uint8_t*, uint8_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                symbol_size,
                                static_cast<RaptorQ__v1::Dec_Report> (report)));
        break;
    case RaptorQ_type::RQ_DEC_16:
        raw_ptr = reinterpret_cast<void *> (
                            new RQ::Decoder<uint16_t*, uint16_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                symbol_size,
                                static_cast<RaptorQ__v1::Dec_Report> (report)));
        break;
    case RaptorQ_type::RQ_DEC_32:
        raw_ptr = reinterpret_cast<void *> (
                            new RQ::Decoder<uint32_t*, uint32_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                symbol_size,
                                static_cast<RaptorQ__v1::Dec_Report> (report)));
        break;
    case RaptorQ_type::RQ_DEC_64:
        raw_ptr = reinterpret_cast<void *> (
                            new RQ::Decoder<uint64_t*, uint64_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                symbol_size,
                                static_cast<RaptorQ__v1::Dec_Report> (report)));
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        return nullptr;
    }
    if (raw_ptr == nullptr)
        return nullptr;
    return new RaptorQ_ptr (type, raw_ptr);
}

static bool v1_initialized (const RaptorQ_ptr *ptr)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return false;
    switch (ptr->type) {
    case RaptorQ_type::RQ_ENC_8:
        return static_cast<bool> (*reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                                    ptr->ptr));
    case RaptorQ_type::RQ_ENC_16:
        return static_cast<bool> (*reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                                    ptr->ptr));
    case RaptorQ_type::RQ_ENC_32:
        return static_cast<bool> (*reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                                    ptr->ptr));
    case RaptorQ_type::RQ_ENC_64:
        return static_cast<bool> (*reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                                    ptr->ptr));
    case RaptorQ_type::RQ_DEC_8:
        return static_cast<bool> (*reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                                    ptr->ptr));
    case RaptorQ_type::RQ_DEC_16:
        return static_cast<bool> (*reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                                    ptr->ptr));
    case RaptorQ_type::RQ_DEC_32:
        return static_cast<bool> (*reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                                    ptr->ptr));
    case RaptorQ_type::RQ_DEC_64:
        return static_cast<bool> (*reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                                    ptr->ptr));
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}


/////////////////////
// Common functions
/////////////////////


static uint16_t v1_symbols (const RaptorQ_ptr *ptr)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return 0;
    switch (ptr->type) {
    case RaptorQ_type::RQ_ENC_8:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_DEC_8:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_DEC_16:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_DEC_32:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_DEC_64:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

static size_t v1_symbol_size (const RaptorQ_ptr *ptr)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return 0;
    switch (ptr->type) {
    case RaptorQ_type::RQ_ENC_8:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_DEC_8:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_DEC_16:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_DEC_32:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_DEC_64:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

static void v1_stop (const RaptorQ_ptr *ptr)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return;
    switch (ptr->type) {
    case RaptorQ_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                            ptr->ptr))->stop();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                            ptr->ptr))->stop();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                            ptr->ptr))->stop();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                            ptr->ptr))->stop();
    case RaptorQ_type::RQ_DEC_8:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                            ptr->ptr))->stop();
    case RaptorQ_type::RQ_DEC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                            ptr->ptr))->stop();
    case RaptorQ_type::RQ_DEC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                            ptr->ptr))->stop();
    case RaptorQ_type::RQ_DEC_64:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                            ptr->ptr))->stop();
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

static RaptorQ_Error v1_future_state (struct RaptorQ_future *const f)
{
    if (f == nullptr)
        return RaptorQ_Error::RQ_ERR_WRONG_INPUT;
    switch (f->type)
    {
    case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
        if (reinterpret_cast<const RaptorQ_future_enc*> (f)->f.valid()) {
            if (reinterpret_cast<const RaptorQ_future_enc*>(f)->f.wait_for (
                                            std::chrono::milliseconds (0)) ==
                                                std::future_status::ready) {
                return RaptorQ_Error::RQ_ERR_NONE; // ready
            }
            return RaptorQ_Error::RQ_ERR_WORKING; // still working
        }
        break;
    case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
        if (reinterpret_cast<const RaptorQ_future_dec*> (f)->f.valid()) {
            if (reinterpret_cast<const RaptorQ_future_dec*>(f)->f.wait_for (
                                            std::chrono::milliseconds (0)) ==
                                                std::future_status::ready) {
                return RaptorQ_Error::RQ_ERR_NONE; // ready
            }
            return RaptorQ_Error::RQ_ERR_WORKING; // still working
        }
    }
    // future not valid
    return RaptorQ_Error::RQ_ERR_NOT_NEEDED;
}

static RaptorQ_Error v1_future_wait_for (struct RaptorQ_future *const f,
                                                const uint64_t time,
                                                const RaptorQ_Unit_Time unit)
{
    if (f == nullptr)
        return RQ_ERR_WRONG_INPUT;
    std::future_status status = std::future_status::timeout;
    switch (unit) {
    case RQ_TIME_NANOSEC:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<const RaptorQ_future_enc*>(f)->f.wait_for (
                                            std::chrono::nanoseconds (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<const RaptorQ_future_dec*>(f)->f.wait_for (
                                            std::chrono::nanoseconds (time));
        }
        break;
    case RQ_TIME_MICROSEC:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<const RaptorQ_future_enc*>(f)->f.wait_for (
                                            std::chrono::microseconds (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<const RaptorQ_future_dec*>(f)->f.wait_for (
                                            std::chrono::microseconds (time));
        }
        break;
    case RQ_TIME_MILLISEC:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<const RaptorQ_future_enc*>(f)->f.wait_for (
                                            std::chrono::milliseconds (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<const RaptorQ_future_dec*>(f)->f.wait_for (
                                            std::chrono::milliseconds (time));
        }
        break;
    case RQ_TIME_SEC:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<const RaptorQ_future_enc*>(f)->f.wait_for (
                                                std::chrono::seconds (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<const RaptorQ_future_dec*>(f)->f.wait_for (
                                                std::chrono::seconds (time));
        }
        break;
    case RQ_TIME_MIN:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<const RaptorQ_future_enc*>(f)->f.wait_for (
                                                std::chrono::minutes (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<const RaptorQ_future_dec*>(f)->f.wait_for (
                                                std::chrono::minutes (time));
        }
        break;
    case RQ_TIME_HOUR:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<const RaptorQ_future_enc*>(f)->f.wait_for (
                                                    std::chrono::hours (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<const RaptorQ_future_dec*>(f)->f.wait_for (
                                                    std::chrono::hours (time));
        }
        break;
    }
    if (status == std::future_status::ready)
        return RaptorQ_Error::RQ_ERR_NONE;
    return RaptorQ_Error::RQ_ERR_WORKING;
}

static void v1_future_wait (struct RaptorQ_future *const f)
{
    if (f == nullptr)
        return;
    switch (f->type)
    {
    case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
        reinterpret_cast<const RaptorQ_future_enc*>(f)->f.wait();
        break;
    case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
        reinterpret_cast<const RaptorQ_future_dec*>(f)->f.wait();
    }
}

static void v1_future_free (struct RaptorQ_future **f)
{
    if (f == nullptr || *f == nullptr)
        return;
    switch ((*f)->type) {
    case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
        delete reinterpret_cast<RaptorQ_future_enc*>(*f);
        break;
    case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
        delete reinterpret_cast<RaptorQ_future_dec*>(*f);
    }
    *f = nullptr;
}

static void v1_free (struct RaptorQ_ptr **ptr)
{
    if (ptr == nullptr || *ptr == nullptr)
        return;
    if ((*ptr)->ptr == nullptr) {
        delete *ptr;
        *ptr = nullptr;
        return;
    }
    switch ((*ptr)->type) {
    case RaptorQ_type::RQ_ENC_8:
        delete reinterpret_cast<
                RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> ((*ptr)->ptr);
        break;
    case RaptorQ_type::RQ_ENC_16:
        delete reinterpret_cast<
                RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*>((*ptr)->ptr);
        break;
    case RaptorQ_type::RQ_ENC_32:
        delete reinterpret_cast<
                RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*>((*ptr)->ptr);
        break;
    case RaptorQ_type::RQ_ENC_64:
        delete reinterpret_cast<
                RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*>((*ptr)->ptr);
        break;
    case RaptorQ_type::RQ_DEC_8:
        delete reinterpret_cast<
                RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> ((*ptr)->ptr);
        break;
    case RaptorQ_type::RQ_DEC_16:
        delete reinterpret_cast<
                RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*>((*ptr)->ptr);
        break;
    case RaptorQ_type::RQ_DEC_32:
        delete reinterpret_cast<
                RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*>((*ptr)->ptr);
        break;
    case RaptorQ_type::RQ_DEC_64:
        delete reinterpret_cast<
                RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*>((*ptr)->ptr);
        break;
    case RaptorQ_type::RQ_NONE:
        break;
    }
    delete *ptr;
    *ptr = nullptr;
}

static bool v1_ready (const RaptorQ_ptr *ptr)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return false;
    switch (ptr->type) {
    case RaptorQ_type::RQ_ENC_8:
        return static_cast<bool> (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                            ptr->ptr)->ready());
    case RaptorQ_type::RQ_ENC_16:
        return static_cast<bool> (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                            ptr->ptr)->ready());
    case RaptorQ_type::RQ_ENC_32:
        return static_cast<bool> (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                            ptr->ptr)->ready());
    case RaptorQ_type::RQ_ENC_64:
        return static_cast<bool> (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                            ptr->ptr)->ready());
    case RaptorQ_type::RQ_DEC_8:
        return static_cast<bool> (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                            ptr->ptr)->ready());
    case RaptorQ_type::RQ_DEC_16:
        return static_cast<bool> (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                            ptr->ptr)->ready());
    case RaptorQ_type::RQ_DEC_32:
        return static_cast<bool> (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                            ptr->ptr)->ready());
    case RaptorQ_type::RQ_DEC_64:
        return static_cast<bool> (reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                            ptr->ptr)->ready());
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

//////////////////////////////
// Encoder-specific functions
//////////////////////////////

static uint32_t v1_max_repair  (const RaptorQ_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return 0;
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    enc->ptr))->max_repair();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    enc->ptr))->max_repair();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    enc->ptr))->max_repair();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                    enc->ptr))->max_repair();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

static size_t v1_set_data (const RaptorQ_ptr *enc, void **from,
                                                            const size_t size)
{
    if (enc == nullptr || enc->ptr == nullptr ||
                                        from == nullptr || *from == nullptr) {
        return 0;
    }
    uint8_t *f_8 = reinterpret_cast<uint8_t*> (*from);
    uint16_t *f_16 = reinterpret_cast<uint16_t*> (*from);
    uint32_t *f_32 = reinterpret_cast<uint32_t*> (*from);
    uint64_t *f_64= reinterpret_cast<uint64_t*> (*from);
    size_t ret;
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                enc->ptr)->set_data (f_8,
                                                                f_8 + size);
        *from = reinterpret_cast<void*> (f_8);
        return ret;
    case RaptorQ_type::RQ_ENC_16:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                enc->ptr)->set_data (f_16,
                                                                f_16 + size);
        *from = reinterpret_cast<void*> (f_16);
        return ret;
    case RaptorQ_type::RQ_ENC_32:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                enc->ptr)->set_data (f_32,
                                                                f_32 + size);
        *from = reinterpret_cast<void*> (f_32);
        return ret;
    case RaptorQ_type::RQ_ENC_64:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                enc->ptr)->set_data (f_64,
                                                                f_64 + size);
        *from = reinterpret_cast<void*> (f_64);
        return ret;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

static bool v1_has_data (const RaptorQ_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return false;
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                        enc->ptr))->has_data();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                        enc->ptr))->has_data();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                        enc->ptr))->has_data();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<const
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                        enc->ptr))->has_data();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

static void v1_clear_data (const RaptorQ_ptr *ptr)
{
    if (ptr == nullptr || ptr->ptr == nullptr)
        return;
    switch (ptr->type) {
    case RaptorQ_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    ptr->ptr))->clear_data();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    ptr->ptr))->clear_data();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    ptr->ptr))->clear_data();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                    ptr->ptr))->clear_data();
    case RaptorQ_type::RQ_DEC_8:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    ptr->ptr))->clear_data();
    case RaptorQ_type::RQ_DEC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    ptr->ptr))->clear_data();
    case RaptorQ_type::RQ_DEC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    ptr->ptr))->clear_data();
    case RaptorQ_type::RQ_DEC_64:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                    ptr->ptr))->clear_data();
    case RaptorQ_type::RQ_NONE:
        return;
    }
}

static bool v1_precompute_sync (const RaptorQ_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return false;
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                enc->ptr))->precompute_sync();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                enc->ptr))->precompute_sync();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                enc->ptr))->precompute_sync();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                enc->ptr))->precompute_sync();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

static bool v1_compute_sync (const RaptorQ_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return false;
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    enc->ptr))->compute_sync();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    enc->ptr))->compute_sync();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    enc->ptr))->compute_sync();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                    enc->ptr))->compute_sync();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

static RaptorQ_future_enc* v1_precompute (const RaptorQ_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return nullptr;
    RaptorQ_future_enc *fut = new RaptorQ_future_enc();
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        fut->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                        enc->ptr)->precompute();
        break;
    case RaptorQ_type::RQ_ENC_16:
        fut->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                        enc->ptr)->precompute();
        break;
    case RaptorQ_type::RQ_ENC_32:
        fut->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                        enc->ptr)->precompute();
        break;
    case RaptorQ_type::RQ_ENC_64:
        fut->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                        enc->ptr)->precompute();
        break;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        return nullptr;
    }
    return fut;
}

static RaptorQ_future_enc* v1_compute (const RaptorQ_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return nullptr;
    RaptorQ_future_enc *fut = new RaptorQ_future_enc();
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        fut->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                        enc->ptr)->compute();
        break;
    case RaptorQ_type::RQ_ENC_16:
        fut->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                        enc->ptr)->compute();
        break;
    case RaptorQ_type::RQ_ENC_32:
        fut->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                        enc->ptr)->compute();
        break;
    case RaptorQ_type::RQ_ENC_64:
        fut->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                        enc->ptr)->compute();
        break;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        return nullptr;
    }
    return fut;
}

static RaptorQ_Error v1_enc_future_get (struct RaptorQ_future_enc *f)
{
    if (f == nullptr)
        return RaptorQ_Error::RQ_ERR_WRONG_INPUT;
    if (f->f.valid())
        return static_cast<RaptorQ_Error> (f->f.get());
    return RaptorQ_Error::RQ_ERR_WORKING;
}

static size_t v1_encode (const RaptorQ_ptr *enc, void **from, const size_t size,
                                                            const uint32_t id)
{
    if (enc == nullptr || enc->ptr == nullptr ||
                                        from == nullptr || *from == nullptr) {
        return 0;
    }
    uint8_t *f_8;
    uint16_t *f_16;
    uint32_t *f_32;
    uint64_t *f_64;
    size_t ret;
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        f_8 = reinterpret_cast<uint8_t*> (*from);
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                enc->ptr)->encode (f_8,
                                                            f_8 + size, id);
        *from = reinterpret_cast<void*> (f_8);
        return ret;
    case RaptorQ_type::RQ_ENC_16:
        f_16 = reinterpret_cast<uint16_t*> (*from);
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                enc->ptr)->encode (f_16,
                                                            f_16 + size, id);
        *from = reinterpret_cast<void*> (f_16);
        return ret;
    case RaptorQ_type::RQ_ENC_32:
        f_32 = reinterpret_cast<uint32_t*> (*from);
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                enc->ptr)->encode (f_32,
                                                            f_32 + size, id);
        *from = reinterpret_cast<void*> (f_32);
        return ret;
    case RaptorQ_type::RQ_ENC_64:
        f_64 = reinterpret_cast<uint64_t*> (*from);
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                enc->ptr)->encode (f_64,
                                                            f_64 + size, id);
        *from = reinterpret_cast<void*> (f_64);
        return ret;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}


//////////////////////////////
// Decoder-specific functions
//////////////////////////////

static RaptorQ_Error v1_add_symbol (const RaptorQ_ptr *dec, void **from,
                                                    const size_t size,
                                                    const uint32_t esi)
{
    if (dec == nullptr || dec->ptr == nullptr ||
                                        from == nullptr || *from == nullptr)
        return RaptorQ_Error::RQ_ERR_WRONG_INPUT;
    uint8_t *f_8;
    uint16_t *f_16;
    uint32_t *f_32;
    uint64_t *f_64;
    RaptorQ_Error err = RaptorQ_Error::RQ_ERR_NONE;
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        f_8 = reinterpret_cast<uint8_t*> (*from);
        err = static_cast<RaptorQ_Error> (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                            dec->ptr)->add_symbol (f_8,
                                                            f_8 + size, esi));
        *from = f_8;
        return err;
    case RaptorQ_type::RQ_DEC_16:
        f_16 = reinterpret_cast<uint16_t*> (*from);
        err = static_cast<RaptorQ_Error> (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                            dec->ptr)->add_symbol (f_16,
                                                            f_16 + size, esi));
        *from = f_16;
        return err;
    case RaptorQ_type::RQ_DEC_32:
        f_32 = reinterpret_cast<uint32_t*> (*from);
        err = static_cast<RaptorQ_Error> (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                            dec->ptr)->add_symbol (f_32,
                                                            f_32 + size, esi));
        *from = f_32;
        return err;
    case RaptorQ_type::RQ_DEC_64:
        f_64 = reinterpret_cast<uint64_t*> (*from);
        err = static_cast<RaptorQ_Error> (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                            dec->ptr)->add_symbol (f_64,
                                                            f_64 + size, esi));
        *from = f_64;
        return err;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return RaptorQ_Error::RQ_ERR_WRONG_INPUT;
}

static bool v1_can_decode (const RaptorQ_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return false;
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        dec->ptr)->can_decode();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        dec->ptr)->can_decode();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        dec->ptr)->can_decode();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                        dec->ptr)->can_decode();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

static uint16_t v1_needed_symbols (const RaptorQ_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return false;
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    dec->ptr)->needed_symbols();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    dec->ptr)->needed_symbols();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    dec->ptr)->needed_symbols();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<const
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                    dec->ptr)->needed_symbols();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

static RaptorQ_Dec_wait_res v1_poll (const RaptorQ_ptr *dec)
{

    if (dec == nullptr || dec->ptr == nullptr)
        return {RaptorQ_Error::RQ_ERR_WRONG_INPUT, 0};

    using D_T8 = RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>;
    using D_T16 = RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>;
    using D_T32 = RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>;
    using D_T64 = RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>;

    struct RaptorQ__v1::Decoder_wait_res res;

    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        res = reinterpret_cast<D_T8*> (dec->ptr)->poll();
        return {static_cast<RaptorQ_Error>(res.error), res.symbol};
    case RaptorQ_type::RQ_DEC_16:
        res = reinterpret_cast<D_T16*> (dec->ptr)->poll();
        return {static_cast<RaptorQ_Error>(res.error), res.symbol};
    case RaptorQ_type::RQ_DEC_32:
        res = reinterpret_cast<D_T32*> (dec->ptr)->poll();
        return {static_cast<RaptorQ_Error>(res.error), res.symbol};
    case RaptorQ_type::RQ_DEC_64:
        res = reinterpret_cast<D_T64*> (dec->ptr)->poll();
        return {static_cast<RaptorQ_Error>(res.error), res.symbol};
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return {RaptorQ_Error::RQ_ERR_WRONG_INPUT, 0};
}

static RaptorQ_Dec_wait_res v1_wait_sync (const RaptorQ_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return {RaptorQ_Error::RQ_ERR_WRONG_INPUT, 0};

    using D_T8 = RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>;
    using D_T16 = RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>;
    using D_T32 = RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>;
    using D_T64 = RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>;
    struct RaptorQ__v1::Decoder_wait_res  res;

    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        res = reinterpret_cast<D_T8*>(dec->ptr)->wait_sync();
        return {static_cast<RaptorQ_Error>(res.error), res.symbol};
    case RaptorQ_type::RQ_DEC_16:
        res = reinterpret_cast<D_T16*>(dec->ptr)->wait_sync();
        return {static_cast<RaptorQ_Error>(res.error), res.symbol};
    case RaptorQ_type::RQ_DEC_32:
        res = reinterpret_cast<D_T32*>(dec->ptr)->wait_sync();
        return {static_cast<RaptorQ_Error>(res.error), res.symbol};
    case RaptorQ_type::RQ_DEC_64:
        res = reinterpret_cast<D_T64*>(dec->ptr)->wait_sync();
        return {static_cast<RaptorQ_Error>(res.error), res.symbol};
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return {RaptorQ_Error::RQ_ERR_WRONG_INPUT, 0};
}

static RaptorQ_future_dec* v1_wait (const RaptorQ_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return nullptr;
    auto ret = std::unique_ptr<RaptorQ_future_dec> (new RaptorQ_future_dec());
    using D_T8 = RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>;
    using D_T16 = RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>;
    using D_T32 = RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>;
    using D_T64 = RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>;
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        ret->f = reinterpret_cast<D_T8*>(dec->ptr)->wait();
        return ret.release();
    case RaptorQ_type::RQ_DEC_16:
        ret->f = reinterpret_cast<D_T16*>(dec->ptr)->wait();
        return ret.release();
    case RaptorQ_type::RQ_DEC_32:
        ret->f = reinterpret_cast<D_T32*>(dec->ptr)->wait();
        return ret.release();
    case RaptorQ_type::RQ_DEC_64:
        ret->f = reinterpret_cast<D_T64*>(dec->ptr)->wait();
        return ret.release();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return nullptr;
}

static RaptorQ_Dec_wait_res v1_dec_future_get (struct RaptorQ_future_dec *f)
{
    RaptorQ__v1::Decoder_wait_res cpp_res;
    if (f == nullptr)
        return {RaptorQ_Error::RQ_ERR_WRONG_INPUT, 0};
    if (f->f.valid()) {
        cpp_res = f->f.get();
        return {static_cast<RaptorQ_Error> (cpp_res.error), cpp_res.symbol};
    } else {
        return {RaptorQ_Error::RQ_ERR_WORKING, 0};
    }
}

static struct RaptorQ_Byte_Tracker v1_end_of_input (struct RaptorQ_ptr *dec,
                                            const RaptorQ_Fill_With_Zeros fill)
{
    struct RaptorQ_Byte_Tracker ret;
    ret.length = 0;
    ret.bitmask = nullptr;
    if (dec == nullptr || dec->ptr == nullptr)
        return ret;

    RaptorQ__v1::Fill_With_Zeros cpp_fill =
                            static_cast<RaptorQ__v1::Fill_With_Zeros> (fill);
    std::vector<bool> cpp_res;
    using D_T8 = RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>;
    using D_T16 = RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>;
    using D_T32 = RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>;
    using D_T64 = RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>;
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        cpp_res = reinterpret_cast<D_T8*>(dec->ptr)->end_of_input (cpp_fill);
        break;
    case RaptorQ_type::RQ_DEC_16:
        cpp_res = reinterpret_cast<D_T16*>(dec->ptr)->end_of_input (cpp_fill);
        break;
    case RaptorQ_type::RQ_DEC_32:
        cpp_res = reinterpret_cast<D_T32*>(dec->ptr)->end_of_input (cpp_fill);
        break;
    case RaptorQ_type::RQ_DEC_64:
        cpp_res = reinterpret_cast<D_T64*>(dec->ptr)->end_of_input (cpp_fill);
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
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

static RaptorQ_Decoder_Result v1_decode_once (struct RaptorQ_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return RQ_DEC_WRONG_INPUT;

    using D_T8 = RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>;
    using D_T16 = RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>;
    using D_T32 = RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>;
    using D_T64 = RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>;
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        return static_cast<RaptorQ_Decoder_Result> (
                            reinterpret_cast<D_T8*>(dec->ptr)->decode_once());
    case RaptorQ_type::RQ_DEC_16:
        return static_cast<RaptorQ_Decoder_Result> (
                            reinterpret_cast<D_T16*>(dec->ptr)->decode_once());
    case RaptorQ_type::RQ_DEC_32:
        return static_cast<RaptorQ_Decoder_Result> (
                            reinterpret_cast<D_T32*>(dec->ptr)->decode_once());
    case RaptorQ_type::RQ_DEC_64:
        return static_cast<RaptorQ_Decoder_Result> (
                            reinterpret_cast<D_T64*>(dec->ptr)->decode_once());
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return RQ_DEC_WRONG_INPUT;
}

static RaptorQ_Error v1_decode_symbol (struct RaptorQ_ptr *dec, void** start,
                                        const size_t size, const uint16_t esi)
{
    RaptorQ__v1::Error err = RaptorQ__v1::Error::WRONG_INPUT;
    if (dec == nullptr || dec->ptr == nullptr || start == nullptr ||
                                                            *start == nullptr) {
        return static_cast<RaptorQ_Error> (err);
    }
    auto ret = std::unique_ptr<RaptorQ_future_dec> (new RaptorQ_future_dec());
    using D_T8 = RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>;
    using D_T16 = RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>;
    using D_T32 = RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>;
    using D_T64 = RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>;

    uint8_t *f_8;
    uint16_t *f_16;
    uint32_t *f_32;
    uint64_t *f_64;
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        f_8 = reinterpret_cast<uint8_t*> (*start);
        err = reinterpret_cast<D_T8*>(dec->ptr)->decode_symbol (
                                                        f_8, f_8 + size, esi);
         *start = f_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        f_16 = reinterpret_cast<uint16_t*> (*start);
        err =  reinterpret_cast<D_T16*>(dec->ptr)->decode_symbol (
                                                        f_16, f_16 + size, esi);
        *start = f_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        f_32 = reinterpret_cast<uint32_t*> (*start);
        err =  reinterpret_cast<D_T32*>(dec->ptr)->decode_symbol (
                                                        f_32, f_32 + size, esi);
        *start = f_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        f_64 = reinterpret_cast<uint64_t*> (*start);
        err =  reinterpret_cast<D_T64*>(dec->ptr)->decode_symbol (
                                                        f_64, f_64 + size, esi);
        *start = f_64;
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return static_cast<RaptorQ_Error> (err);
}

static RaptorQ_Dec_Written v1_decode_bytes (struct RaptorQ_ptr *dec,
                                                        void **start,
                                                        const size_t size,
                                                        const size_t from_byte,
                                                        const size_t skip)
{
    RaptorQ__v1::Decoder_written out {0, 0};
    if (dec == nullptr || dec->ptr == nullptr || start == nullptr ||
                                                            *start == nullptr) {
        return {out.written, out.offset};
    }
    auto ret = std::unique_ptr<RaptorQ_future_dec> (new RaptorQ_future_dec());
    using D_T8 = RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>;
    using D_T16 = RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>;
    using D_T32 = RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>;
    using D_T64 = RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>;

    uint8_t *f_8;
    uint16_t *f_16;
    uint32_t *f_32;
    uint64_t *f_64;
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        f_8 = reinterpret_cast<uint8_t*> (*start);
        out = reinterpret_cast<D_T8*>(dec->ptr)->decode_bytes (
                                            f_8, f_8 + size, from_byte, skip);
         *start = f_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        f_16 = reinterpret_cast<uint16_t*> (*start);
        out =  reinterpret_cast<D_T16*>(dec->ptr)->decode_bytes (
                                            f_16, f_16 + size, from_byte, skip);
        *start = f_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        f_32 = reinterpret_cast<uint32_t*> (*start);
        out =  reinterpret_cast<D_T32*>(dec->ptr)->decode_bytes (
                                            f_32, f_32 + size, from_byte, skip);
        *start = f_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        f_64 = reinterpret_cast<uint64_t*> (*start);
        out =  reinterpret_cast<D_T64*>(dec->ptr)->decode_bytes (
                                            f_64, f_64 + size, from_byte, skip);
        *start = f_64;
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return {out.written, out.offset};
}
