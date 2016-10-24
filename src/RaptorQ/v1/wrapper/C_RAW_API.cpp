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


#include "RaptorQ/v1/wrapper/C_RAW_API.h"
#include "RaptorQ/v1/RaptorQ.hpp"
#include <future>
#include <utility>

struct RAPTORQ_LOCAL RaptorQ_ptr
{
    void *ptr;
    const RaptorQ_type type;

    RaptorQ_ptr (const RaptorQ_type _type) : ptr (nullptr), type (_type) {}
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
    std::future<RaptorQ__v1::Error> f;
    RaptorQ_future_enc ()
        :base (RaptorQ_Future_Type::RQ_FUTURE_ENCODER) {}
};
struct RAPTORQ_LOCAL RaptorQ_future_dec
{
    const struct RaptorQ_future base;
    std::future<std::pair<RaptorQ__v1::Error, uint16_t>> f;
    RaptorQ_future_dec ()
        :base (RaptorQ_Future_Type::RQ_FUTURE_DECODER) {}
};


// precomputation caching
static RaptorQ_Compress RAPTORQ_LOCAL v1_supported_compressions();
static RaptorQ_Compress RAPTORQ_LOCAL v1_get_compression();
static bool RAPTORQ_LOCAL v1_set_compression (
                                            const RaptorQ_Compress compression);
static size_t RAPTORQ_LOCAL v1_shared_cache_size (const size_t shared_cache);
static size_t RAPTORQ_LOCAL v1_local_cache_size (const size_t local_cache);
static size_t RAPTORQ_LOCAL v1_get_shared_cache_size ();
static size_t RAPTORQ_LOCAL v1_get_local_cache_size ();
// constructors
struct RaptorQ_ptr* RAPTORQ_LOCAL v1_Encoder (RaptorQ_type type,
                                            const RaptorQ_Block_Size symbols,
                                            const size_t symbol_size);
struct RaptorQ_ptr* RAPTORQ_LOCAL v1_Decoder (RaptorQ_type type,
                                            const RaptorQ_Block_Size symbols,
                                            const size_t symbol_size,
                                            const RaptorQ_Compute report);
// common functions
static uint16_t RAPTORQ_LOCAL v1_symbols (const RaptorQ_ptr *ptr);
static size_t   RAPTORQ_LOCAL v1_symbol_size (const RaptorQ_ptr *ptr);
static RaptorQ_Error RAPTORQ_LOCAL v1_future_state (struct RaptorQ_future *f);
static RaptorQ_Error RAPTORQ_LOCAL v1_future_wait_for (struct RaptorQ_future *f,
                                                const uint64_t time,
                                                const RaptorQ_Unit_Time unit);
static void RAPTORQ_LOCAL v1_future_wait (struct RaptorQ_future *f);
static void RAPTORQ_LOCAL v1_future_free (struct RaptorQ_future **f);

// encoder-specific
static uint32_t RAPTORQ_LOCAL v1_max_repair  (const RaptorQ_ptr *enc);
static size_t RAPTORQ_LOCAL v1_set_data (const RaptorQ_ptr *enc, void *from,
                                                                const void *to);
static bool RAPTORQ_LOCAL v1_has_data (const RaptorQ_ptr *enc);
static void RAPTORQ_LOCAL v1_clear_data (const RaptorQ_ptr *enc);
static bool RAPTORQ_LOCAL v1_precompute_sync (const RaptorQ_ptr *enc);
static bool RAPTORQ_LOCAL v1_compute_sync (const RaptorQ_ptr *enc);
static RaptorQ_future_enc* RAPTORQ_LOCAL v1_precompute (const RaptorQ_ptr *enc);
static RaptorQ_future_enc* RAPTORQ_LOCAL v1_compute (const RaptorQ_ptr *enc);
static RaptorQ_Error RAPTORQ_LOCAL v1_enc_future_get (
                                                struct RaptorQ_future_enc *f);
static size_t RAPTORQ_LOCAL v1_encode (const RaptorQ_ptr *enc, void *from,
                                            const void *to, const uint32_t id);

// decoder-specific
static RaptorQ_Error RAPTORQ_LOCAL v1_add_symbol (const RaptorQ_ptr *dec,
                                                            void *from,
                                                            const void *to,
                                                            const uint32_t esi);
static bool RAPTORQ_LOCAL v1_can_decode (const RaptorQ_ptr *dec);
static void RAPTORQ_LOCAL v1_stop (const RaptorQ_ptr *dec);
static uint16_t RAPTORQ_LOCAL v1_needed_symbols (const RaptorQ_ptr *dec);
static RaptorQ_dec_result RAPTORQ_LOCAL v1_poll (const RaptorQ_ptr *dec);
static RaptorQ_dec_result RAPTORQ_LOCAL v1_wait_sync (const RaptorQ_ptr *dec);
static RaptorQ_future_dec* RAPTORQ_LOCAL v1_wait (const RaptorQ_ptr *dec);
static RaptorQ_dec_result RAPTORQ_LOCAL v1_dec_future_get (
                                                struct RaptorQ_future_dec *f);




struct RaptorQ_base_api* RAPTORQ_API RaptorQ_api (uint32_t version)
{
    if (version != 1)
        return nullptr;
    auto api = new RaptorQ_v1();
    api->base.version = 1;

    // precomputation caching
    api->supported_compressions = &v1_supported_compressions;
    api->get_compression = &v1_get_compression;
    api->set_compression = &v1_set_compression;
    api->shared_cache_size = &v1_shared_cache_size;
    api->local_cache_size = &v1_local_cache_size;
    api->get_shared_cache_size = &v1_get_shared_cache_size;
    api->get_local_cache_size = &v1_get_local_cache_size;

    // constructors
    api->Encoder = &v1_Encoder;
    api->Decoder = &v1_Decoder;

    // common functions
    api->symbols = &v1_symbols;
    api->symbol_size = &v1_symbol_size;
    api->future_state = &v1_future_state;
    api->future_wait_for = &v1_future_wait_for;
    api->future_wait = &v1_future_wait;
    api->future_free = &v1_future_free;

    // encoder-specific functions
    api->max_repair = &v1_max_repair;
    api->set_data = &v1_set_data;
    api->has_data = &v1_has_data;
    api->clear_data = &v1_clear_data;
    api->precompute_sync = &v1_precompute_sync;
    api->compute_sync = &v1_compute_sync;
    api->precompute = &v1_precompute;
    api->compute = &v1_compute;
    api->enc_future_get = &v1_enc_future_get;
    api->encode = &v1_encode;

    // decoder-specific functions
    api->add_symbol = &v1_add_symbol;
    api->can_decode = &v1_can_decode;
    api->stop = &v1_stop;
    api->needed_symbols = &v1_needed_symbols;
    api->poll = &v1_poll;
    api->wait_sync = &v1_wait_sync;
    api->wait = &v1_wait;
    api->dec_future_get = &v1_dec_future_get;

    return reinterpret_cast<RaptorQ_base_api *> (api);
}

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

static size_t v1_shared_cache_size (const size_t shared_cache)
    { return RaptorQ__v1::shared_cache_size (shared_cache); }

static size_t v1_local_cache_size (const size_t local_cache)
    { return RaptorQ__v1::local_cache_size (local_cache); }

static size_t v1_get_shared_cache_size ()
    { return RFC6330__v1::get_shared_cache_size(); }

static size_t v1_get_local_cache_size ()
    {return RFC6330__v1::get_local_cache_size(); }


/////////////////////
// Constructors
/////////////////////

struct RaptorQ_ptr* RAPTORQ_LOCAL v1_Encoder (RaptorQ_type type,
                                            const RaptorQ_Block_Size symbols,
                                            const size_t symbol_size)
{
    std::unique_ptr<RaptorQ_ptr> ret (new RaptorQ_ptr (type));

    switch (type) {
    case RaptorQ_type::RQ_ENC_8:
        ret->ptr = reinterpret_cast<void *> (
                    new RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                                                symbol_size));
        break;
    case RaptorQ_type::RQ_ENC_16:
        ret->ptr = reinterpret_cast<void *> (
                    new RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                                                symbol_size));
        break;
    case RaptorQ_type::RQ_ENC_32:
        ret->ptr = reinterpret_cast<void *> (
                    new RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                                                symbol_size));
        break;
    case RaptorQ_type::RQ_ENC_64:
        ret->ptr = reinterpret_cast<void *> (
                    new RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*> (
                                static_cast<RaptorQ__v1::Block_Size>(symbols),
                                                                symbol_size));
        break;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        return new RaptorQ_ptr (RaptorQ_type::RQ_NONE);
    }
    if (ret->ptr == nullptr)
        return nullptr;
    return ret.release();
}

struct RaptorQ_ptr* RAPTORQ_LOCAL v1_Decoder (RaptorQ_type type,
                                            const RaptorQ_Block_Size symbols,
                                            const size_t symbol_size,
                                            const RaptorQ_Compute report)
{
    if (report != RQ_COMPUTE_PARTIAL_FROM_BEGINNING &&
        report != RQ_COMPUTE_PARTIAL_ANY &&
        report != RQ_COMPUTE_COMPLETE) {
        return nullptr;
    }
    std::unique_ptr<RaptorQ_ptr> ret (new RaptorQ_ptr (type));

    namespace RQ = RaptorQ__v1::Impl;
    switch (type) {
    case RaptorQ_type::RQ_DEC_8:
        ret->ptr = reinterpret_cast<void *> (
                    new RQ::Decoder<uint8_t*, uint8_t*> (
                        static_cast<RaptorQ__v1::Block_Size>(symbols),
                        symbol_size,
                        static_cast<RQ::Decoder<uint8_t*, uint8_t*>::Report>
                                                                    (report)));
        break;
    case RaptorQ_type::RQ_DEC_16:
        ret->ptr = reinterpret_cast<void *> (
                    new RQ::Decoder<uint16_t*, uint16_t*> (
                        static_cast<RaptorQ__v1::Block_Size>(symbols),
                        symbol_size,
                        static_cast<RQ::Decoder<uint16_t*, uint16_t*>::Report>
                                                                    (report)));
        break;
    case RaptorQ_type::RQ_DEC_32:
        ret->ptr = reinterpret_cast<void *> (
                    new RQ::Decoder<uint32_t*, uint32_t*> (
                        static_cast<RaptorQ__v1::Block_Size>(symbols),
                        symbol_size,
                        static_cast<RQ::Decoder<uint32_t*, uint32_t*>::Report>
                                                                    (report)));
        break;
    case RaptorQ_type::RQ_DEC_64:
        ret->ptr = reinterpret_cast<void *> (
                    new RQ::Decoder<uint64_t*, uint64_t*> (
                        static_cast<RaptorQ__v1::Block_Size>(symbols),
                        symbol_size,
                        static_cast<RQ::Decoder<uint64_t*, uint64_t*>::Report>
                                                                    (report)));
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        return new RaptorQ_ptr (RaptorQ_type::RQ_NONE);
    }
    if (ret->ptr == nullptr)
        return nullptr;
    return ret.release();
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
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_DEC_8:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_DEC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_DEC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        ptr->ptr))->symbols();
    case RaptorQ_type::RQ_DEC_64:
        return (reinterpret_cast<
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
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_DEC_8:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_DEC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_DEC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_DEC_64:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                    ptr->ptr))->symbol_size();
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

static RaptorQ_Error v1_future_state (struct RaptorQ_future *f)
{
    if (f == nullptr)
        return RaptorQ_Error::RQ_ERR_WRONG_INPUT;
    switch (f->type)
    {
    case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
        if (reinterpret_cast<RaptorQ_future_enc*> (f)->f.valid())
            return RaptorQ_Error::RQ_ERR_NONE;
        break;
    case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
        if (reinterpret_cast<RaptorQ_future_dec*> (f)->f.valid())
            return RaptorQ_Error::RQ_ERR_NONE;
    }
    return RaptorQ_Error::RQ_ERR_WORKING;
}

static RaptorQ_Error v1_future_wait_for (struct RaptorQ_future *f,
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
            status = reinterpret_cast<RaptorQ_future_enc*>(f)->f.wait_for (
                                            std::chrono::nanoseconds (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<RaptorQ_future_dec*>(f)->f.wait_for (
                                            std::chrono::nanoseconds (time));
        }
        break;
    case RQ_TIME_MICROSEC:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<RaptorQ_future_enc*>(f)->f.wait_for (
                                            std::chrono::microseconds (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<RaptorQ_future_dec*>(f)->f.wait_for (
                                            std::chrono::microseconds (time));
        }
        break;
    case RQ_TIME_MILLISEC:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<RaptorQ_future_enc*>(f)->f.wait_for (
                                            std::chrono::milliseconds (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<RaptorQ_future_dec*>(f)->f.wait_for (
                                            std::chrono::milliseconds (time));
        }
        break;
    case RQ_TIME_SEC:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<RaptorQ_future_enc*>(f)->f.wait_for (
                                                std::chrono::seconds (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<RaptorQ_future_dec*>(f)->f.wait_for (
                                                std::chrono::seconds (time));
        }
        break;
    case RQ_TIME_MIN:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<RaptorQ_future_enc*>(f)->f.wait_for (
                                                std::chrono::minutes (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<RaptorQ_future_dec*>(f)->f.wait_for (
                                                std::chrono::minutes (time));
        }
        break;
    case RQ_TIME_HOUR:
        switch (f->type)
        {
        case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
            status = reinterpret_cast<RaptorQ_future_enc*>(f)->f.wait_for (
                                                    std::chrono::hours (time));
            break;
        case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
            status = reinterpret_cast<RaptorQ_future_dec*>(f)->f.wait_for (
                                                    std::chrono::hours (time));
        }
        break;
    }
    if (status == std::future_status::ready)
        return RaptorQ_Error::RQ_ERR_NONE;
    return RaptorQ_Error::RQ_ERR_WORKING;
}

static void v1_future_wait (struct RaptorQ_future *f)
{
    if (f == nullptr)
        return;
    switch (f->type)
    {
    case RaptorQ_Future_Type::RQ_FUTURE_ENCODER:
        reinterpret_cast<RaptorQ_future_enc*>(f)->f.wait();
        break;
    case RaptorQ_Future_Type::RQ_FUTURE_DECODER:
        reinterpret_cast<RaptorQ_future_dec*>(f)->f.wait();
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

//////////////////////////////
// Encoder-specific functions
//////////////////////////////

static uint32_t v1_max_repair  (const RaptorQ_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return 0;
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    enc->ptr))->max_repair();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    enc->ptr))->max_repair();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    enc->ptr))->max_repair();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<
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

static size_t v1_set_data (const RaptorQ_ptr *enc, void *from, const void *to)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return 0;
    uint8_t *f_8 = reinterpret_cast<uint8_t*> (from);
    uint16_t *f_16 = reinterpret_cast<uint16_t*> (from);
    uint32_t *f_32 = reinterpret_cast<uint32_t*> (from);
    uint64_t *f_64= reinterpret_cast<uint64_t*> (from);
    size_t ret;
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                enc->ptr)->set_data (f_8,
                                                    reinterpret_cast<uint8_t*> (
                                                        const_cast<void*>(to)));
        from = reinterpret_cast<void*> (f_8);
        return ret;
    case RaptorQ_type::RQ_ENC_16:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                enc->ptr)->set_data (f_16,
                                                    reinterpret_cast<uint16_t*>(
                                                        const_cast<void*>(to)));
        from = reinterpret_cast<void*> (f_16);
        return ret;
    case RaptorQ_type::RQ_ENC_32:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                enc->ptr)->set_data (f_32,
                                                    reinterpret_cast<uint32_t*>(
                                                        const_cast<void*>(to)));
        from = reinterpret_cast<void*> (f_32);
        return ret;
    case RaptorQ_type::RQ_ENC_64:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                enc->ptr)->set_data (f_64,
                                                    reinterpret_cast<uint64_t*>(
                                                        const_cast<void*>(to)));
        from = reinterpret_cast<void*> (f_64);
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
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                        enc->ptr))->has_data();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                        enc->ptr))->has_data();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                        enc->ptr))->has_data();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<
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

static void v1_clear_data (const RaptorQ_ptr *enc)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return;
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                    enc->ptr))->clear_data();
    case RaptorQ_type::RQ_ENC_16:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                    enc->ptr))->clear_data();
    case RaptorQ_type::RQ_ENC_32:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                    enc->ptr))->clear_data();
    case RaptorQ_type::RQ_ENC_64:
        return (reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                    enc->ptr))->clear_data();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
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

static size_t v1_encode (const RaptorQ_ptr *enc, void *from, const void *to,
                                                            const uint32_t id)
{
    if (enc == nullptr || enc->ptr == nullptr)
        return 0;
    uint8_t *f_8 = reinterpret_cast<uint8_t*> (from);
    uint16_t *f_16 = reinterpret_cast<uint16_t*> (from);
    uint32_t *f_32 = reinterpret_cast<uint32_t*> (from);
    uint64_t *f_64= reinterpret_cast<uint64_t*> (from);
    size_t ret;
    switch (enc->type) {
    case RaptorQ_type::RQ_ENC_8:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint8_t*, uint8_t*>*> (
                                                enc->ptr)->encode (f_8,
                                                    reinterpret_cast<uint8_t*> (
                                                    const_cast<void*>(to)), id);
        from = reinterpret_cast<void*> (f_8);
        return ret;
    case RaptorQ_type::RQ_ENC_16:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint16_t*, uint16_t*>*> (
                                                enc->ptr)->encode (f_16,
                                                    reinterpret_cast<uint16_t*>(
                                                    const_cast<void*>(to)), id);
        from = reinterpret_cast<void*> (f_16);
        return ret;
    case RaptorQ_type::RQ_ENC_32:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint32_t*, uint32_t*>*> (
                                                enc->ptr)->encode (f_32,
                                                    reinterpret_cast<uint32_t*>(
                                                    const_cast<void*>(to)), id);
        from = reinterpret_cast<void*> (f_32);
        return ret;
    case RaptorQ_type::RQ_ENC_64:
        ret = reinterpret_cast<
                            RaptorQ__v1::Impl::Encoder<uint64_t*, uint64_t*>*> (
                                                enc->ptr)->encode (f_64,
                                                    reinterpret_cast<uint64_t*>(
                                                    const_cast<void*>(to)), id);
        from = reinterpret_cast<void*> (f_64);
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

static RaptorQ_Error v1_add_symbol (const RaptorQ_ptr *dec, void *from,
                                                    const void *to,
                                                    const uint32_t esi)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return RaptorQ_Error::RQ_ERR_WRONG_INPUT;
    uint8_t *f_8 = reinterpret_cast<uint8_t*> (from);
    uint16_t *f_16 = reinterpret_cast<uint16_t*> (from);
    uint32_t *f_32 = reinterpret_cast<uint32_t*> (from);
    uint64_t *f_64= reinterpret_cast<uint64_t*> (from);
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        return static_cast<RaptorQ_Error> (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                            dec->ptr)->add_symbol (f_8,
                                                reinterpret_cast<uint8_t*> (
                                                const_cast<void*>(to)), esi));
    case RaptorQ_type::RQ_DEC_16:
        return static_cast<RaptorQ_Error> (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                            dec->ptr)->add_symbol (f_16,
                                                reinterpret_cast<uint16_t*>(
                                                const_cast<void*>(to)), esi));
    case RaptorQ_type::RQ_DEC_32:
        return static_cast<RaptorQ_Error> (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                            dec->ptr)->add_symbol (f_32,
                                                reinterpret_cast<uint32_t*>(
                                                const_cast<void*>(to)), esi));
    case RaptorQ_type::RQ_DEC_64:
        return static_cast<RaptorQ_Error> (reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                            dec->ptr)->add_symbol (f_64,
                                                reinterpret_cast<uint64_t*>(
                                                const_cast<void*>(to)), esi));
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
        return reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        dec->ptr)->can_decode();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        dec->ptr)->can_decode();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        dec->ptr)->can_decode();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<
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

static void v1_stop (const RaptorQ_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return;
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        dec->ptr)->stop();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        dec->ptr)->stop();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        dec->ptr)->stop();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                        dec->ptr)->stop();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        return;
    }
}

static uint16_t v1_needed_symbols (const RaptorQ_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return false;
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                    dec->ptr)->needed_symbols();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                    dec->ptr)->needed_symbols();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                    dec->ptr)->needed_symbols();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<
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

static RaptorQ_dec_result v1_poll (const RaptorQ_ptr *dec)
{
    RaptorQ_dec_result res;
    res.err = RaptorQ_Error::RQ_ERR_WRONG_INPUT;
    res.esi = 0;
    if (dec == nullptr || dec->ptr == nullptr)
        return res;
    std::pair<RaptorQ__v1::Error, uint16_t> cpp_res = {
                                            RaptorQ__v1::Error::WRONG_INPUT, 0};
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        cpp_res = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                            dec->ptr)->poll();
        break;
    case RaptorQ_type::RQ_DEC_16:
        cpp_res = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                            dec->ptr)->poll();
        break;
    case RaptorQ_type::RQ_DEC_32:
        cpp_res = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                            dec->ptr)->poll();
        break;
    case RaptorQ_type::RQ_DEC_64:
        cpp_res = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                            dec->ptr)->poll();
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    res.err = static_cast<RaptorQ_Error> (cpp_res.first);
    res.esi = cpp_res.second;
    return res;
}

static RaptorQ_dec_result v1_wait_sync (const RaptorQ_ptr *dec)
{
    RaptorQ_dec_result res;
    res.err = RaptorQ_Error::RQ_ERR_WRONG_INPUT;
    res.esi = 0;
    if (dec == nullptr || dec->ptr == nullptr)
        return res;
    std::pair<RaptorQ__v1::Error, uint16_t> cpp_res = {
                                            RaptorQ__v1::Error::WRONG_INPUT, 0};
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        cpp_res = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                        dec->ptr)->wait_sync();
        break;
    case RaptorQ_type::RQ_DEC_16:
        cpp_res = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                        dec->ptr)->wait_sync();
        break;
    case RaptorQ_type::RQ_DEC_32:
        cpp_res = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                        dec->ptr)->wait_sync();
        break;
    case RaptorQ_type::RQ_DEC_64:
        cpp_res = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                        dec->ptr)->wait_sync();
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    res.err = static_cast<RaptorQ_Error> (cpp_res.first);
    res.esi = cpp_res.second;
    return res;
}

static RaptorQ_future_dec* v1_wait (const RaptorQ_ptr *dec)
{
    if (dec == nullptr || dec->ptr == nullptr)
        return nullptr;
    auto ret = std::unique_ptr<RaptorQ_future_dec> (new RaptorQ_future_dec());
    switch (dec->type) {
    case RaptorQ_type::RQ_DEC_8:
        ret->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint8_t*, uint8_t*>*> (
                                                            dec->ptr)->wait();
        return ret.release();
    case RaptorQ_type::RQ_DEC_16:
        ret->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint16_t*, uint16_t*>*> (
                                                            dec->ptr)->wait();
        return ret.release();
    case RaptorQ_type::RQ_DEC_32:
        ret->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint32_t*, uint32_t*>*> (
                                                            dec->ptr)->wait();
        return ret.release();
    case RaptorQ_type::RQ_DEC_64:
        ret->f = reinterpret_cast<
                            RaptorQ__v1::Impl::Decoder<uint64_t*, uint64_t*>*> (
                                                            dec->ptr)->wait();
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

static RaptorQ_dec_result RAPTORQ_LOCAL v1_dec_future_get (
                                                struct RaptorQ_future_dec *f)
{
    RaptorQ_dec_result res;
    res.err = RaptorQ_Error::RQ_ERR_WRONG_INPUT;
    res.esi = 0;
    if (f == nullptr)
        return res;
    if (f->f.valid()) {
        const auto cpp_res = f->f.get();
        res.err = static_cast<RaptorQ_Error> (cpp_res.first);
        res.esi = cpp_res.second;
    } else {
        res.err = RaptorQ_Error::RQ_ERR_WORKING;
    }
    return res;
}
