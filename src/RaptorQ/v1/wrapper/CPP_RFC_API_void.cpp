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

#include "RaptorQ/v1/wrapper/CPP_RFC_API_void.hpp"
#include "RaptorQ/v1/RFC.hpp"
#include <cmath>
#include <memory>
#include <utility>

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

// quic "casting" to avoid really long, repetitive lines.
union RAPTORQ_LOCAL cast_enc {
    void* const _raw;
    Impl::Encoder<uint8_t*,  uint8_t* >* const _8;
    Impl::Encoder<uint16_t*, uint16_t*>* const _16;
    Impl::Encoder<uint32_t*, uint32_t*>* const _32;
    Impl::Encoder<uint64_t*, uint64_t*>* const _64;
    cast_enc (void * raw) : _raw(raw) {}
};
union RAPTORQ_LOCAL cast_dec {
    void* const _raw;
    Impl::Decoder<uint8_t*,  uint8_t* >* const _8;
    Impl::Decoder<uint16_t*, uint16_t*>* const _16;
    Impl::Decoder<uint32_t*, uint32_t*>* const _32;
    Impl::Decoder<uint64_t*, uint64_t*>* const _64;
    cast_dec (void * raw) : _raw(raw) {}
};

// check that we have the right type, else keep error.
static RaptorQ_type init_t (const RaptorQ_type type, const bool is_encoder)
{
    switch (type) {
    case RaptorQ_type::RQ_ENC_8:
        return (is_encoder ? RaptorQ_type::RQ_ENC_8  : RaptorQ_type::RQ_NONE);
    case RaptorQ_type::RQ_ENC_16:
        return (is_encoder ? RaptorQ_type::RQ_ENC_16 : RaptorQ_type::RQ_NONE);
    case RaptorQ_type::RQ_ENC_32:
        return (is_encoder ? RaptorQ_type::RQ_ENC_32 : RaptorQ_type::RQ_NONE);
    case RaptorQ_type::RQ_ENC_64:
        return (is_encoder ? RaptorQ_type::RQ_ENC_64 : RaptorQ_type::RQ_NONE);
    case RaptorQ_type::RQ_DEC_8:
        return (is_encoder ? RaptorQ_type::RQ_NONE : RaptorQ_type::RQ_DEC_8);
    case RaptorQ_type::RQ_DEC_16:
        return (is_encoder ? RaptorQ_type::RQ_NONE : RaptorQ_type::RQ_DEC_16);
    case RaptorQ_type::RQ_DEC_32:
        return (is_encoder ? RaptorQ_type::RQ_NONE : RaptorQ_type::RQ_DEC_32);
    case RaptorQ_type::RQ_DEC_64:
        return (is_encoder ? RaptorQ_type::RQ_NONE : RaptorQ_type::RQ_DEC_64);
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return RaptorQ_type::RQ_NONE;
}


////////////////////
//// Encoder
////////////////////

Encoder_void::~Encoder_void()
{
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        delete reinterpret_cast<Impl::Encoder<uint8_t*,  uint8_t*> *>(_encoder);
        break;
    case RaptorQ_type::RQ_ENC_16:
        delete reinterpret_cast<Impl::Encoder<uint16_t*, uint16_t*>*>(_encoder);
        break;
    case RaptorQ_type::RQ_ENC_32:
        delete reinterpret_cast<Impl::Encoder<uint32_t*, uint32_t*>*>(_encoder);
        break;
    case RaptorQ_type::RQ_ENC_64:
        delete reinterpret_cast<Impl::Encoder<uint64_t*, uint64_t*>*>(_encoder);
        break;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    _type = RaptorQ_type::RQ_NONE;
    _encoder = nullptr;
}

Encoder_void::Encoder_void (const RaptorQ_type type, const void* data_from,
                                            const void* data_to,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block)
    : _type (init_t (type, true))
{
    _encoder = nullptr;
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        _encoder = new Impl::Encoder<uint8_t*, uint8_t*> (
                    reinterpret_cast<uint8_t*> (const_cast<void*>(data_from)),
                    reinterpret_cast<uint8_t*> (const_cast<void*>(data_to)),
                                                min_subsymbol_size,
                                                symbol_size, max_sub_block);
        break;
    case RaptorQ_type::RQ_ENC_16:
        _encoder = new Impl::Encoder<uint16_t*, uint16_t*> (
                    reinterpret_cast<uint16_t*> (const_cast<void*>(data_from)),
                    reinterpret_cast<uint16_t*> (const_cast<void*>(data_to)),
                                                min_subsymbol_size,
                                                symbol_size, max_sub_block);
        break;
    case RaptorQ_type::RQ_ENC_32:
        _encoder = new Impl::Encoder<uint32_t*, uint32_t*> (
                    reinterpret_cast<uint32_t*> (const_cast<void*>(data_from)),
                    reinterpret_cast<uint32_t*> (const_cast<void*>(data_to)),
                                                min_subsymbol_size,
                                                symbol_size, max_sub_block);
        break;
    case RaptorQ_type::RQ_ENC_64:
        _encoder = new Impl::Encoder<uint64_t*, uint64_t*> (
                    reinterpret_cast<uint64_t*> (const_cast<void*>(data_from)),
                    reinterpret_cast<uint64_t*> (const_cast<void*>(data_to)),
                                                min_subsymbol_size,
                                                symbol_size, max_sub_block);
        break;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

Encoder_void::operator bool() const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return static_cast<bool> (_enc._8);
    case RaptorQ_type::RQ_ENC_16:
        return static_cast<bool> (_enc._16);
    case RaptorQ_type::RQ_ENC_32:
        return static_cast<bool> (_enc._32);
    case RaptorQ_type::RQ_ENC_64:
        return static_cast<bool> (_enc._64);
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

RFC6330_OTI_Common_Data Encoder_void::OTI_Common() const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->OTI_Common();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->OTI_Common();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->OTI_Common();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->OTI_Common();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

RFC6330_OTI_Scheme_Specific_Data Encoder_void::OTI_Scheme_Specific() const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->OTI_Scheme_Specific();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._8->OTI_Scheme_Specific();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._8->OTI_Scheme_Specific();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._8->OTI_Scheme_Specific();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

Encoder_void::result Encoder_void::compute_sync (const Compute flags)
{
    const cast_enc _enc (_encoder);
    Encoder_void::result res (Error::INITIALIZATION, 0);
    std::future<std::pair<Error, uint8_t>> fut;
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        fut = _enc._8->compute (flags);
        break;
    case RaptorQ_type::RQ_ENC_16:
        fut = _enc._16->compute (flags);
        break;
    case RaptorQ_type::RQ_ENC_32:
        fut = _enc._32->compute (flags);
        break;
    case RaptorQ_type::RQ_ENC_64:
        fut = _enc._64->compute (flags);
        break;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        return res;
    }

    std::tie (res.err, res.sbn) = fut.get();
    return res;
}

std::future<std::pair<Error, uint8_t>> Encoder_void::compute (
                                                            const Compute flags)
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->compute (flags);
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->compute (flags);
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->compute (flags);
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->compute (flags);
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    std::promise<std::pair<Error, uint8_t>> p;
    p.set_value ({Error::INITIALIZATION, 0});
    return p.get_future();
}

size_t Encoder_void::precompute_max_memory ()
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->precompute_max_memory();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->precompute_max_memory();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->precompute_max_memory();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->precompute_max_memory();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}


size_t Encoder_void::encode (void** output, const void* end, const uint32_t esi,
                                                            const uint8_t sbn)
{
    const cast_enc _enc (_encoder);
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    size_t ret = 0;
    if (output == nullptr || *output == nullptr || end == nullptr)
        return ret;
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        p_8 = reinterpret_cast<uint8_t*> (*output);
        ret = _enc._8->encode (p_8,
                            reinterpret_cast<uint8_t*> (const_cast<void*> (end)),
                                                                    esi, sbn);
        *output = p_8;
        break;
    case RaptorQ_type::RQ_ENC_16:
        p_16 = reinterpret_cast<uint16_t*> (*output);
        ret = _enc._16->encode (p_16,
                        reinterpret_cast<uint16_t*> (const_cast<void*> (end)),
                                                                    esi, sbn);

        *output = p_16;
        break;
    case RaptorQ_type::RQ_ENC_32:
        p_32 = reinterpret_cast<uint32_t*> (*output);
        ret = _enc._32->encode (p_32,
                        reinterpret_cast<uint32_t*> (const_cast<void*> (end)),
                                                                    esi, sbn);

        *output = p_32;
        break;
    case RaptorQ_type::RQ_ENC_64:
        p_64 = reinterpret_cast<uint64_t*> (*output);
        ret = _enc._64->encode (p_64,
                        reinterpret_cast<uint64_t*> (const_cast<void*> (end)),
                                                                    esi, sbn);

        *output = p_64;
        break;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return ret;
}

size_t Encoder_void::encode (void** output, const void* end, const uint32_t id)
{
    const cast_enc _enc (_encoder);
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    size_t ret = 0;
    if (output == nullptr || *output == nullptr || end == nullptr)
        return ret;
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        p_8 = reinterpret_cast<uint8_t*> (*output);
        ret = _enc._8->encode (p_8,
                            reinterpret_cast<uint8_t*> (const_cast<void*> (end)),
                                                                            id);
        *output = p_8;
        break;
    case RaptorQ_type::RQ_ENC_16:
        p_16 = reinterpret_cast<uint16_t*> (*output);
        ret = _enc._16->encode (p_16,
                        reinterpret_cast<uint16_t*> (const_cast<void*> (end)),
                                                                            id);

        *output = p_16;
        break;
    case RaptorQ_type::RQ_ENC_32:
        p_32 = reinterpret_cast<uint32_t*> (*output);
        ret = _enc._32->encode (p_32,
                        reinterpret_cast<uint32_t*> (const_cast<void*> (end)),
                                                                            id);

        *output = p_32;
        break;
    case RaptorQ_type::RQ_ENC_64:
        p_64 = reinterpret_cast<uint64_t*> (*output);
        ret = _enc._64->encode (p_64,
                        reinterpret_cast<uint64_t*> (const_cast<void*> (end)),
                                                                            id);

        *output = p_64;
        break;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return ret;
}

void Encoder_void::free (const uint8_t sbn)
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->free (sbn);
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->free (sbn);
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->free (sbn);
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->free (sbn);
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

uint8_t Encoder_void::blocks() const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->blocks();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->blocks();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->blocks();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->blocks();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

uint32_t Encoder_void::block_size (const uint8_t sbn) const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->block_size (sbn);
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->block_size (sbn);
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->block_size (sbn);
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->block_size (sbn);
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

uint16_t Encoder_void::symbol_size() const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->symbol_size();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->symbol_size();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->symbol_size();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->symbol_size();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

uint16_t Encoder_void::symbols (const uint8_t sbn) const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->symbols (sbn);
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->symbols (sbn);
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->symbols (sbn);
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->symbols (sbn);
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

Block_Size Encoder_void::extended_symbols (const uint8_t sbn) const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->extended_symbols (sbn);
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->extended_symbols (sbn);
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->extended_symbols (sbn);
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->extended_symbols (sbn);
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return static_cast<Block_Size> (0);
}

uint32_t Encoder_void::max_repair (const uint8_t sbn) const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->max_repair (sbn);
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->max_repair (sbn);
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->max_repair (sbn);
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->max_repair (sbn);
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

////////////////////
//// Decoder
////////////////////

Decoder_void::~Decoder_void()
{
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        delete reinterpret_cast<Impl::Decoder<uint8_t*,  uint8_t*> *>(_decoder);
        break;
    case RaptorQ_type::RQ_DEC_16:
        delete reinterpret_cast<Impl::Decoder<uint16_t*, uint16_t*>*>(_decoder);
        break;
    case RaptorQ_type::RQ_DEC_32:
        delete reinterpret_cast<Impl::Decoder<uint32_t*, uint32_t*>*>(_decoder);
        break;
    case RaptorQ_type::RQ_DEC_64:
        delete reinterpret_cast<Impl::Decoder<uint64_t*, uint64_t*>*>(_decoder);
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    _type = RaptorQ_type::RQ_NONE;
    _decoder = nullptr;
}

Decoder_void::Decoder_void (const RaptorQ_type type,
                                const RFC6330_OTI_Common_Data common,
                                const RFC6330_OTI_Scheme_Specific_Data scheme)
    : _type (init_t (type, false))
{
    _decoder = nullptr;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        _decoder = new Impl::Decoder<uint8_t*, uint8_t*> (common, scheme);
        break;
    case RaptorQ_type::RQ_DEC_16:
        _decoder = new Impl::Decoder<uint16_t*, uint16_t*> (common, scheme);
        break;
    case RaptorQ_type::RQ_DEC_32:
        _decoder = new Impl::Decoder<uint32_t*, uint32_t*> (common, scheme);
        break;
    case RaptorQ_type::RQ_DEC_64:
        _decoder = new Impl::Decoder<uint64_t*, uint64_t*> (common, scheme);
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

Decoder_void::Decoder_void (const RaptorQ_type type, const uint64_t size,
                                                    const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment)
    : _type (init_t (type, false))
{
    _decoder = nullptr;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        _decoder = new Impl::Decoder<uint8_t*, uint8_t*> (size, symbol_size,
                                                sub_blocks, blocks, alignment);
        break;
    case RaptorQ_type::RQ_DEC_16:
        _decoder = new Impl::Decoder<uint16_t*, uint16_t*> (size, symbol_size,
                                                sub_blocks, blocks, alignment);
        break;
    case RaptorQ_type::RQ_DEC_32:
        _decoder = new Impl::Decoder<uint32_t*, uint32_t*> (size, symbol_size,
                                                sub_blocks, blocks, alignment);
        break;
    case RaptorQ_type::RQ_DEC_64:
        _decoder = new Impl::Decoder<uint64_t*, uint64_t*> (size, symbol_size,
                                                sub_blocks, blocks, alignment);
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

Decoder_void::operator bool() const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return static_cast<bool> (_dec._8);
    case RaptorQ_type::RQ_DEC_16:
        return static_cast<bool> (_dec._16);
    case RaptorQ_type::RQ_DEC_32:
        return static_cast<bool> (_dec._32);
    case RaptorQ_type::RQ_DEC_64:
        return static_cast<bool> (_dec._64);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

std::future<std::pair<Error, uint8_t>> Decoder_void::compute (
                                                            const Compute flags)
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->compute (flags);
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->compute (flags);
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->compute (flags);
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->compute (flags);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    std::promise<std::pair<Error, uint8_t>> p;
    p.set_value ({Error::INITIALIZATION, 0});
    return p.get_future();
}

std::vector<bool> Decoder_void::end_of_input (const Fill_With_Zeros fill,
                                                            const uint8_t block)
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->end_of_input (fill, block);
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->end_of_input (fill, block);
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->end_of_input (fill, block);
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->end_of_input (fill, block);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return std::vector<bool>();
}

std::vector<bool> Decoder_void::end_of_input (const Fill_With_Zeros fill)
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->end_of_input (fill);
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->end_of_input (fill);
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->end_of_input (fill);
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->end_of_input (fill);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return std::vector<bool>();
}


uint64_t Decoder_void::decode_symbol (void** start, const void* end,
                                        const uint16_t esi, const uint8_t sbn)
{
    const cast_dec _dec (_decoder);
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    size_t ret = 0;
    if (start == nullptr || *start == nullptr || end == nullptr)
        return ret;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*start);
        ret = _dec._8->decode_symbol (p_8,
                reinterpret_cast<uint8_t*> (const_cast<void*> (end)), esi, sbn);
        *start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*start);
        ret = _dec._16->decode_symbol (p_16,
                reinterpret_cast<uint16_t*> (const_cast<void*>(end)), esi, sbn);
        *start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*start);
        ret = _dec._32->decode_symbol (p_32,
                reinterpret_cast<uint32_t*> (const_cast<void*>(end)), esi, sbn);
        *start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*start);
        ret = _dec._64->decode_symbol (p_64,
                reinterpret_cast<uint64_t*> (const_cast<void*>(end)), esi, sbn);
        *start = p_64;
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return ret;
}

// result in BYTES
uint64_t Decoder_void::decode_bytes (void** start, const void* end,
                                                            const uint8_t skip)
{
    const cast_dec _dec (_decoder);
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    size_t ret = 0;
    if (start == nullptr || *start == nullptr || end == nullptr)
        return ret;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*start);
        ret = _dec._8->decode_bytes (p_8,
                    reinterpret_cast<uint8_t*> (const_cast<void*> (end)), skip);
        *start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*start);
        ret = _dec._16->decode_bytes (p_16,
                    reinterpret_cast<uint16_t*> (const_cast<void*>(end)), skip);
        *start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*start);
        ret = _dec._32->decode_bytes (p_32,
                    reinterpret_cast<uint32_t*> (const_cast<void*>(end)), skip);
        *start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*start);
        ret = _dec._64->decode_bytes (p_64,
                    reinterpret_cast<uint64_t*> (const_cast<void*>(end)), skip);
        *start = p_64;
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return ret;
}

size_t Decoder_void::decode_block_bytes (void** start, const void* end,
                                                        const uint8_t skip,
                                                        const uint8_t sbn)
{
    const cast_dec _dec (_decoder);
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    size_t ret = 0;
    if (start == nullptr || *start == nullptr || end == nullptr)
        return ret;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*start);
        ret = _dec._8->decode_block_bytes (p_8,
                        reinterpret_cast<uint8_t*> (const_cast<void*> (end)),
                                                                    skip, sbn);
        *start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*start);
        ret = _dec._16->decode_block_bytes (p_16,
                        reinterpret_cast<uint16_t*> (const_cast<void*>(end)),
                                                                    skip, sbn);
        *start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*start);
        ret = _dec._32->decode_block_bytes (p_32,
                        reinterpret_cast<uint32_t*> (const_cast<void*>(end)),
                                                                    skip, sbn);
        *start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*start);
        ret = _dec._64->decode_block_bytes (p_64,
                        reinterpret_cast<uint64_t*> (const_cast<void*>(end)),
                                                                    skip, sbn);
        *start = p_64;
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return ret;
}

Decoder_written Decoder_void::decode_aligned (void** start,
                                                            const void* end,
                                                            const uint8_t skip)
{
    const cast_dec _dec (_decoder);
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    Decoder_written cpp_res {0, 0};
    if (start == nullptr || *start == nullptr || end == nullptr)
        return cpp_res;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*start);
        cpp_res = _dec._8->decode_aligned (p_8,
                        reinterpret_cast<uint8_t*> (const_cast<void*> (end)),
                                                                        skip);
        *start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*start);
        cpp_res = _dec._16->decode_aligned (p_16,
                        reinterpret_cast<uint16_t*> (const_cast<void*>(end)),
                                                                        skip);
        *start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*start);
        cpp_res = _dec._32->decode_aligned (p_32,
                        reinterpret_cast<uint32_t*> (const_cast<void*>(end)),
                                                                        skip);
        *start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*start);
        cpp_res = _dec._64->decode_aligned (p_64,
                        reinterpret_cast<uint64_t*> (const_cast<void*>(end)),
                                                                        skip);
        *start = p_64;
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return cpp_res;
}

Decoder_written Decoder_void::decode_block_aligned (void** start,
                                                        const void* end,
                                                        const uint8_t skip,
                                                        const uint8_t sbn)
{
    const cast_dec _dec (_decoder);
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    Decoder_written cpp_res {0, 0};
    if (start == nullptr || *start == nullptr || end == nullptr)
        return cpp_res;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*start);
        cpp_res = _dec._8->decode_block_aligned (p_8,
                        reinterpret_cast<uint8_t*> (const_cast<void*> (end)),
                                                                    skip, sbn);
        *start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*start);
        cpp_res = _dec._16->decode_block_aligned (p_16,
                        reinterpret_cast<uint16_t*> (const_cast<void*>(end)),
                                                                    skip, sbn);
        *start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*start);
        cpp_res = _dec._32->decode_block_aligned (p_32,
                        reinterpret_cast<uint32_t*> (const_cast<void*>(end)),
                                                                    skip, sbn);
        *start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*start);
        cpp_res = _dec._64->decode_block_aligned (p_64,
                        reinterpret_cast<uint64_t*> (const_cast<void*>(end)),
                                                                    skip, sbn);
        *start = p_64;
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return cpp_res;
}

Error Decoder_void::add_symbol (void** start, const void* end,const uint32_t id)
{
    const cast_dec _dec (_decoder);
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    Error err = Error::INITIALIZATION;
    if (start == nullptr || *start == nullptr || end == nullptr)
        return Error::WRONG_INPUT;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*start);
        err = _dec._8->add_symbol (p_8,
                    reinterpret_cast<uint8_t*> (const_cast<void*> (end)), id);
        *start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*start);
        err = _dec._16->add_symbol (p_16,
                    reinterpret_cast<uint16_t*> (const_cast<void*> (end)), id);
        *start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*start);
        err = _dec._32->add_symbol (p_32,
                    reinterpret_cast<uint32_t*> (const_cast<void*> (end)), id);
        *start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*start);
        err = _dec._64->add_symbol (p_64,
                    reinterpret_cast<uint64_t*> (const_cast<void*> (end)), id);
        *start = p_64;
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return err;
}

Error Decoder_void::add_symbol (void** start, const void* end,
                                                            const uint32_t esi,
                                                            const uint8_t sbn)
{
    const cast_dec _dec (_decoder);
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    Error err = Error::INITIALIZATION;
    if (start == nullptr || *start == nullptr || end == nullptr)
        return Error::WRONG_INPUT;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*start);
        err = _dec._8->add_symbol (p_8,
                        reinterpret_cast<uint8_t*> (const_cast<void*> (end)),
                                                                    esi, sbn);
        *start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*start);
        err = _dec._16->add_symbol (p_16,
                        reinterpret_cast<uint16_t*> (const_cast<void*> (end)),
                                                                    esi, sbn);
        *start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*start);
        err = _dec._32->add_symbol (p_32,
                        reinterpret_cast<uint32_t*> (const_cast<void*> (end)),
                                                                    esi, sbn);
        *start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*start);
        err = _dec._64->add_symbol (p_64,
                        reinterpret_cast<uint64_t*> (const_cast<void*> (end)),
                                                                    esi, sbn);
        *start = p_64;
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return err;
}

uint8_t Decoder_void::blocks_ready ()
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->blocks_ready();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->blocks_ready();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->blocks_ready();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->blocks_ready();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

bool Decoder_void::is_ready ()
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->is_ready();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->is_ready();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->is_ready();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->is_ready();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

bool Decoder_void::is_block_ready (const uint8_t sbn)
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->is_block_ready (sbn);
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->is_block_ready (sbn);
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->is_block_ready (sbn);
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->is_block_ready (sbn);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

void Decoder_void::free (const uint8_t sbn)
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->free (sbn);
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->free (sbn);
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->free (sbn);
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->free (sbn);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

uint64_t Decoder_void::bytes() const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->bytes();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->bytes();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->bytes();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->bytes();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

uint8_t Decoder_void::blocks() const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->blocks();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->blocks();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->blocks();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->blocks();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

uint32_t Decoder_void::block_size (const uint8_t sbn) const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->block_size (sbn);
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->block_size (sbn);
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->block_size (sbn);
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->block_size (sbn);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

uint16_t Decoder_void::symbol_size() const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->symbol_size();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->symbol_size();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->symbol_size();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->symbol_size();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

uint16_t Decoder_void::symbols (const uint8_t sbn) const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->symbols (sbn);
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->symbols (sbn);
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->symbols (sbn);
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->symbols (sbn);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

Block_Size Decoder_void::extended_symbols (const uint8_t sbn) const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->extended_symbols (sbn);
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->extended_symbols (sbn);
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->extended_symbols (sbn);
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->extended_symbols (sbn);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return static_cast<Block_Size> (0);
}

} // namespace Impl
} // namespace RFC6330__v1
