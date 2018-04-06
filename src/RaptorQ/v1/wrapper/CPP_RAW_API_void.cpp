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

#include "RaptorQ/v1/wrapper/CPP_RAW_API_void.hpp"
#include "RaptorQ/v1/RaptorQ.hpp"

namespace RaptorQ__v1 {
namespace Impl {

// quick "casting" to avoid really long, repetitive lines.
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

////////////////
//// Encoder
/////////////////

Encoder_void::Encoder_void (const RaptorQ_type type, const Block_Size symbols,
                                                    const size_t symbol_size)
    : _type (init_t (type, true))
{
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        _encoder = new Encoder<uint8_t*, uint8_t*> (symbols, symbol_size);
        return;
    case RaptorQ_type::RQ_ENC_16:
        _encoder = new Encoder<uint16_t*, uint16_t*> (symbols, symbol_size);
        return;
    case RaptorQ_type::RQ_ENC_32:
        _encoder = new Encoder<uint32_t*, uint32_t*> (symbols, symbol_size);
        return;
    case RaptorQ_type::RQ_ENC_64:
        _encoder = new Encoder<uint64_t*, uint64_t*> (symbols, symbol_size);
        return;
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    _encoder = nullptr;
}

Encoder_void::~Encoder_void()
{
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        delete reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder);
        break;
    case RaptorQ_type::RQ_ENC_16:
        delete reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder);
        break;
    case RaptorQ_type::RQ_ENC_32:
        delete reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder);
        break;
    case RaptorQ_type::RQ_ENC_64:
        delete reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder);
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

Encoder_void::operator bool() const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return static_cast<bool> (*_enc._8);
    case RaptorQ_type::RQ_ENC_16:
        return static_cast<bool> (*_enc._16);
    case RaptorQ_type::RQ_ENC_32:
        return static_cast<bool> (*_enc._32);
    case RaptorQ_type::RQ_ENC_64:
        return static_cast<bool> (*_enc._64);
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

uint16_t Encoder_void::symbols() const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->symbols();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->symbols();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->symbols();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->symbols();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

size_t Encoder_void::symbol_size() const
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

uint32_t Encoder_void::max_repair() const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->max_repair();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->max_repair();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->max_repair();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->max_repair();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

bool Encoder_void::has_data() const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->has_data();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->has_data();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->has_data();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->has_data();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

size_t Encoder_void::set_data (const void* from, const void* to)
{
    if (from == nullptr || to == nullptr)
        return 0;
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->set_data(
                        reinterpret_cast<uint8_t*> (const_cast<void*> (from)),
                        reinterpret_cast<uint8_t*> (const_cast<void*> (to)));
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->set_data(
                        reinterpret_cast<uint16_t*> (const_cast<void*> (from)),
                        reinterpret_cast<uint16_t*> (const_cast<void*> (to)));
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->set_data(
                        reinterpret_cast<uint32_t*> (const_cast<void*> (from)),
                        reinterpret_cast<uint32_t*> (const_cast<void*> (to)));
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->set_data(
                        reinterpret_cast<uint64_t*> (const_cast<void*> (from)),
                        reinterpret_cast<uint64_t*> (const_cast<void*> (to)));
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

void Encoder_void::clear_data()
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->clear_data();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->clear_data();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->clear_data();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->clear_data();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

bool Encoder_void::ready() const
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->ready();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->ready();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->ready();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->ready();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

void Encoder_void::stop()
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->stop();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->stop();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->stop();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->stop();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

bool Encoder_void::precompute_sync()
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->precompute_sync();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->precompute_sync();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->precompute_sync();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->precompute_sync();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

bool Encoder_void::compute_sync()
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->compute_sync();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->compute_sync();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->compute_sync();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->compute_sync();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

std::shared_future<Error> Encoder_void::precompute()
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->precompute();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->precompute();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->precompute();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->precompute();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    std::promise<Error> p;
    p.set_value(Error::INITIALIZATION);
    return p.get_future();
}

std::shared_future<Error> Encoder_void::compute()
{
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return _enc._8->compute();
    case RaptorQ_type::RQ_ENC_16:
        return _enc._16->compute();
    case RaptorQ_type::RQ_ENC_32:
        return _enc._32->compute();
    case RaptorQ_type::RQ_ENC_64:
        return _enc._64->compute();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    std::promise<Error> p;
    p.set_value(Error::INITIALIZATION);
    return p.get_future();
}

size_t Encoder_void::encode (void** output, const void* end, const uint32_t id)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    size_t ret = 0;
    if (output == nullptr || end == nullptr) {
        return ret;
    }
    const cast_enc _enc (_encoder);
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        p_8 = reinterpret_cast<uint8_t*> (*output);
        ret = _enc._8->encode (p_8, reinterpret_cast<uint8_t*> (
                                                const_cast<void*> (end)), id);
        *output = p_8;
        break;
    case RaptorQ_type::RQ_ENC_16:
        p_16 = reinterpret_cast<uint16_t*> (*output);
        ret = _enc._16->encode (p_16, reinterpret_cast<uint16_t*> (
                                                const_cast<void*> (end)), id);
        *output = p_16;
        break;
    case RaptorQ_type::RQ_ENC_32:
        p_32 = reinterpret_cast<uint32_t*> (*output);
        ret = _enc._32->encode (p_32, reinterpret_cast<uint32_t*> (
                                                const_cast<void*> (end)), id);
        *output = p_32;
        break;
    case RaptorQ_type::RQ_ENC_64:
        p_64 = reinterpret_cast<uint64_t*> (*output);
        ret = _enc._64->encode (p_64, reinterpret_cast<uint64_t*> (
                                                const_cast<void*> (end)), id);
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


////////////////
//// Decoder
/////////////////

Decoder_void::Decoder_void (const RaptorQ_type type,
                                                const Block_Size symbols,
                                                const size_t symbol_size,
                                                const Dec_Report computation)
    : _type (init_t (type, false))
{
    _decoder = nullptr;

    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        _decoder = new Decoder<uint8_t*, uint8_t*> (symbols,
                                                    symbol_size, computation);
        return;
    case RaptorQ_type::RQ_DEC_16:
        _decoder = new Decoder<uint16_t*, uint16_t*> (symbols,
                                                    symbol_size, computation);
        return;
    case RaptorQ_type::RQ_DEC_32:
        _decoder = new Decoder<uint32_t*, uint32_t*> (symbols,
                                                    symbol_size, computation);
        return;
    case RaptorQ_type::RQ_DEC_64:
        _decoder = new Decoder<uint64_t*, uint64_t*> (symbols,
                                                    symbol_size, computation);
        return;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        _type = RaptorQ_type::RQ_NONE;
        break;
    }
}

Decoder_void::~Decoder_void()
{
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        delete reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder);
        break;
    case RaptorQ_type::RQ_DEC_16:
        delete reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder);
        break;
    case RaptorQ_type::RQ_DEC_32:
        delete reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder);
        break;
    case RaptorQ_type::RQ_DEC_64:
        delete reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder);
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

uint16_t Decoder_void::symbols() const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->symbols();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->symbols();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->symbols();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->symbols();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

size_t Decoder_void::symbol_size() const
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

Error Decoder_void::add_symbol (void** from, const void* to, const uint32_t esi)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    Error err = Error::INITIALIZATION;
    if (from == nullptr || to == nullptr)
        return err;
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*from);
        err = _dec._8->add_symbol (p_8,
                    reinterpret_cast<uint8_t*> (const_cast<void*> (to)), esi);
        *from = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*from);
        err = _dec._16->add_symbol (p_16,
                    reinterpret_cast<uint16_t*> (const_cast<void*> (to)), esi);
        *from = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*from);
        err = _dec._32->add_symbol (p_32,
                    reinterpret_cast<uint32_t*> (const_cast<void*> (to)), esi);
        *from = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*from);
        err = _dec._64->add_symbol (p_64,
                    reinterpret_cast<uint64_t*> (const_cast<void*> (to)), esi);
        *from = p_64;
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

bool Decoder_void::can_decode() const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->can_decode();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->can_decode();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->can_decode();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->can_decode();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

bool Decoder_void::ready() const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->ready();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->ready();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->ready();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->ready();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

void Decoder_void::stop()
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->stop();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->stop();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->stop();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->stop();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

void Decoder_void::clear_data()
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->clear_data();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->clear_data();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->clear_data();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->clear_data();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

uint16_t Decoder_void::needed_symbols() const
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->needed_symbols();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->needed_symbols();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->needed_symbols();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->needed_symbols();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

void Decoder_void::set_max_concurrency (const uint16_t max_threads)
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->set_max_concurrency (max_threads);
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->set_max_concurrency (max_threads);
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->set_max_concurrency (max_threads);
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->set_max_concurrency (max_threads);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

Decoder_Result Decoder_void::decode_once()
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->decode_once();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->decode_once();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->decode_once();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->decode_once();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return Decoder_Result::STOPPED;
}

struct Decoder_wait_res Decoder_void::poll()
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->poll();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->poll();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->poll();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->poll();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return {Error::INITIALIZATION, 0};
}

struct Decoder_wait_res Decoder_void::wait_sync()
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->wait_sync();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->wait_sync();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->wait_sync();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->wait_sync();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return {Error::INITIALIZATION, 0};
}

std::future<struct Decoder_wait_res> Decoder_void::wait()
{
    const cast_dec _dec (_decoder);
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return _dec._8->wait();
    case RaptorQ_type::RQ_DEC_16:
        return _dec._16->wait();
    case RaptorQ_type::RQ_DEC_32:
        return _dec._32->wait();
    case RaptorQ_type::RQ_DEC_64:
        return _dec._64->wait();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    std::promise<struct Decoder_wait_res> p;
    p.set_value({Error::INITIALIZATION, 0});
    return p.get_future();
}

Error Decoder_void::decode_symbol (void** start, const void* end,
                                                            const uint16_t esi)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    const cast_dec _dec (_decoder);
    Error ret = Error::INITIALIZATION;
    if (start == nullptr || end == nullptr)
        return ret;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*start);
        ret = _dec._8-> decode_symbol (p_8,
                    reinterpret_cast<uint8_t*> (const_cast<void*> (end)), esi);
        *start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*start);
        ret = _dec._16-> decode_symbol (p_16,
                    reinterpret_cast<uint16_t*> (const_cast<void*> (end)), esi);
        *start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*start);
        ret = _dec._32-> decode_symbol (p_32,
                    reinterpret_cast<uint32_t*> (const_cast<void*> (end)), esi);
        *start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*start);
        ret = _dec._64-> decode_symbol (p_64,
                    reinterpret_cast<uint64_t*> (const_cast<void*> (end)), esi);
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

struct Decoder_written Decoder_void::decode_bytes (void** start,
                                                        const void* end,
                                                        const size_t from_byte,
                                                        const size_t skip)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    struct Decoder_written ret {0, 0};
    if (start == nullptr || end == nullptr)
        return ret;
    const cast_dec _dec (_decoder);

    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (*start);
        ret = _dec._8->decode_bytes (p_8,
                        reinterpret_cast<uint8_t*> (const_cast<void*> (end)),
                                                            from_byte, skip);
        *start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (*start);
        ret = _dec._16->decode_bytes (p_16,
                         reinterpret_cast<uint16_t*> (const_cast<void*> (end)),
                                                             from_byte, skip);
        *start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (*start);
        ret = _dec._32->decode_bytes (p_32,
                         reinterpret_cast<uint32_t*> (const_cast<void*> (end)),
                                                             from_byte, skip);
        *start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (*start);
        ret = _dec._64->decode_bytes (p_64,
                         reinterpret_cast<uint64_t*> (const_cast<void*> (end)),
                                                             from_byte, skip);
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

} // namespace Impl
} // namespace RaptorQ__v1
