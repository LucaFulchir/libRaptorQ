/*
 * Copyright (c) 2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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


// check that we have the right type, else keep error.
static RaptorQ_type RAPTORQ_LOCAL init_t (const RaptorQ_type type,
                                                        const bool is_encoder)
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
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder);
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder);
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder);
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder);
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
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                                    symbols();
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                                    symbols();
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                                    symbols();
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                                    symbols();
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
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                                symbol_size();
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                                symbol_size();
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                                symbol_size();
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                                symbol_size();
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
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                                max_repair();
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                                max_repair();
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                                max_repair();
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                                max_repair();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

It::Encoder::Symbol_Iterator_void Encoder_void::begin_source()
    { return It::Encoder::Symbol_Iterator_void (this, 0); }

It::Encoder::Symbol_Iterator_void Encoder_void::end_source()
    { return It::Encoder::Symbol_Iterator_void (this, symbols()); }

It::Encoder::Symbol_Iterator_void Encoder_void::begin_repair()
    { return It::Encoder::Symbol_Iterator_void (this, symbols()); }

It::Encoder::Symbol_Iterator_void Encoder_void::end_repair (
                                                        const uint32_t repair)
{
    return It::Encoder::Symbol_Iterator_void (this, symbols() + repair);
}

bool Encoder_void::has_data() const
{
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                                has_data();
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                                has_data();
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                                has_data();
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                                has_data();
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
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                    set_data(
                        reinterpret_cast<uint8_t*> (const_cast<void*> (from)),
                        reinterpret_cast<uint8_t*> (const_cast<void*> (to)));
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                    set_data(
                        reinterpret_cast<uint16_t*> (const_cast<void*> (from)),
                        reinterpret_cast<uint16_t*> (const_cast<void*> (to)));
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                    set_data(
                        reinterpret_cast<uint32_t*> (const_cast<void*> (from)),
                        reinterpret_cast<uint32_t*> (const_cast<void*> (to)));
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                    set_data(
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
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                                clear_data();
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                                clear_data();
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                                clear_data();
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                                clear_data();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

void Encoder_void::stop()
{
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                                        stop();
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                                        stop();
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                                        stop();
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                                        stop();
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
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                            precompute_sync();
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                            precompute_sync();
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                            precompute_sync();
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                            precompute_sync();
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
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                                compute_sync();
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                                compute_sync();
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                                compute_sync();
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                                compute_sync();
    case RaptorQ_type::RQ_DEC_8:
    case RaptorQ_type::RQ_DEC_16:
    case RaptorQ_type::RQ_DEC_32:
    case RaptorQ_type::RQ_DEC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return false;
}

std::future<Error> Encoder_void::precompute()
{
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                                precompute();
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                                precompute();
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                                precompute();
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                                precompute();
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

std::future<Error> Encoder_void::compute()
{
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        return reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                                    compute();
    case RaptorQ_type::RQ_ENC_16:
        return reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                                    compute();
    case RaptorQ_type::RQ_ENC_32:
        return reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                                    compute();
    case RaptorQ_type::RQ_ENC_64:
        return reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                                    compute();
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

size_t Encoder_void::encode (void* &output, const void* end, const uint32_t id)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    size_t ret = 0;
    switch (_type) {
    case RaptorQ_type::RQ_ENC_8:
        p_8 = reinterpret_cast<uint8_t*> (output);
        ret = reinterpret_cast<Encoder<uint8_t*, uint8_t*>*> (_encoder)->
                                                                encode(p_8,
                    reinterpret_cast<uint8_t*> (const_cast<void*> (end)), id);
        output = p_8;
        break;
    case RaptorQ_type::RQ_ENC_16:
        p_16 = reinterpret_cast<uint16_t*> (output);
        ret = reinterpret_cast<Encoder<uint16_t*, uint16_t*>*> (_encoder)->
                                                                encode(p_16,
                    reinterpret_cast<uint16_t*> (const_cast<void*> (end)), id);
        output = p_16;
        break;
    case RaptorQ_type::RQ_ENC_32:
        p_32 = reinterpret_cast<uint32_t*> (output);
        ret = reinterpret_cast<Encoder<uint32_t*, uint32_t*>*> (_encoder)->
                                                                encode(p_32,
                    reinterpret_cast<uint32_t*> (const_cast<void*> (end)), id);
        output = p_32;
        break;
    case RaptorQ_type::RQ_ENC_64:
        p_64 = reinterpret_cast<uint64_t*> (output);
        ret = reinterpret_cast<Encoder<uint64_t*, uint64_t*>*> (_encoder)->
                                                                encode(p_64,
                    reinterpret_cast<uint64_t*> (const_cast<void*> (end)), id);
        output = p_64;

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

Decoder_void::Decoder_void (const RaptorQ_type type, const Block_Size symbols,
                                                    const size_t symbol_size,
                                                    const Report computation)
    : _type (init_t (type, true))
{

    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        _decoder = new Decoder<uint8_t*, uint8_t*> (symbols, symbol_size,
                                                                computation);
        return;
    case RaptorQ_type::RQ_DEC_16:
        _decoder = new Decoder<uint16_t*, uint16_t*> (symbols, symbol_size,
                                                                computation);
        return;
    case RaptorQ_type::RQ_DEC_32:
        _decoder = new Decoder<uint32_t*, uint32_t*> (symbols, symbol_size,
                                                                computation);
        return;
    case RaptorQ_type::RQ_DEC_64:
        _decoder = new Decoder<uint64_t*, uint64_t*> (symbols, symbol_size,
                                                                computation);
        return;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    _decoder = nullptr;
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
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder);
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder);
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder);
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder);
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
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                                    symbols();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                                    symbols();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                                    symbols();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                                    symbols();
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
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                                symbol_size();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                                symbol_size();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                                symbol_size();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                                symbol_size();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return 0;
}

It::Decoder::Symbol_Iterator_void Decoder_void::begin()
    { return It::Decoder::Symbol_Iterator_void (this, 0); }

It::Decoder::Symbol_Iterator_void Decoder_void::end()
    { return It::Decoder::Symbol_Iterator_void (this, symbols()); }

Error Decoder_void::add_symbol (void* &from, const void* to, const uint32_t esi)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    Error err = Error::INITIALIZATION;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (from);
        err = reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                    add_symbol (p_8,
                        reinterpret_cast<uint8_t*> (const_cast<void*> (to)),
                                                                        esi);
        from = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (from);
        err = reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                    add_symbol (p_16,
                        reinterpret_cast<uint16_t*> (const_cast<void*> (to)),
                                                                        esi);
        from = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (from);
        err = reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                    add_symbol (p_32,
                        reinterpret_cast<uint32_t*> (const_cast<void*> (to)),
                                                                        esi);
        from = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (from);
        err = reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                    add_symbol (p_64,
                        reinterpret_cast<uint64_t*> (const_cast<void*> (to)),
                                                                        esi);
        from = p_64;
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

void Decoder_void::end_of_input()
{
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                                end_of_input();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                                end_of_input();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                                end_of_input();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                                end_of_input();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

bool Decoder_void::can_decode() const
{
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                                can_decode();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                                can_decode();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                                can_decode();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                                can_decode();
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
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                                        stop();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                                        stop();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                                        stop();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                                        stop();
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
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                            needed_symbols();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                            needed_symbols();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                            needed_symbols();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                            needed_symbols();
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
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                            set_max_concurrency (max_threads);
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                            set_max_concurrency (max_threads);
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                            set_max_concurrency (max_threads);
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                            set_max_concurrency (max_threads);
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
}

Decoder_void::Decoder_Result Decoder_void::decode_once()
{
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                                decode_once();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                                decode_once();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                                decode_once();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                                decode_once();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return Decoder_void::Decoder_Result::STOPPED;
}

std::pair<Error, uint16_t> Decoder_void::poll()
{
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                                        poll();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                                        poll();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                                        poll();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                                        poll();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return {Error::INITIALIZATION, 0};
}

std::pair<Error, uint16_t> Decoder_void::wait_sync()
{
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                                    wait_sync();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                                    wait_sync();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                                    wait_sync();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                                    wait_sync();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    return {Error::INITIALIZATION, 0};
}

std::future<std::pair<Error, uint16_t>> Decoder_void::wait()
{
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        return reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                                        wait();
    case RaptorQ_type::RQ_DEC_16:
        return reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                                        wait();
    case RaptorQ_type::RQ_DEC_32:
        return reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                                        wait();
    case RaptorQ_type::RQ_DEC_64:
        return reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                                        wait();
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    std::promise<std::pair<Error, uint16_t>> p;
    p.set_value({Error::INITIALIZATION, 0});
    return p.get_future();
}

Error Decoder_void::decode_symbol (void* &start, const void* end,
                                                            const uint16_t esi)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    Error ret = Error::INITIALIZATION;
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (start);
        ret = reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                            decode_symbol (p_8,
                    reinterpret_cast<uint8_t*> (const_cast<void*> (end)), esi);
        start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (start);
        ret = reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                            decode_symbol (p_16,
                    reinterpret_cast<uint16_t*> (const_cast<void*> (end)), esi);
        start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (start);
        ret = reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                            decode_symbol (p_32,
                    reinterpret_cast<uint32_t*> (const_cast<void*> (end)), esi);
        start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (start);
        ret = reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                            decode_symbol (p_64,
                    reinterpret_cast<uint64_t*> (const_cast<void*> (end)), esi);
        start = p_64;
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

Decoder_void::decode_pair Decoder_void::decode_bytes (void* &start,
                                                        const void* end,
                                                        const size_t from_byte,
                                                        const size_t skip)
{
    uint8_t *p_8;
    uint16_t *p_16;
    uint32_t *p_32;
    uint64_t *p_64;
    std::pair<size_t, size_t> ret {0, 0};
    switch (_type) {
    case RaptorQ_type::RQ_DEC_8:
        p_8 = reinterpret_cast<uint8_t*> (start);
        ret = reinterpret_cast<Decoder<uint8_t*, uint8_t*>*> (_decoder)->
                                                            decode_bytes(p_8,
                        reinterpret_cast<uint8_t*> (const_cast<void*> (end)),
                                                            from_byte, skip);
        start = p_8;
        break;
    case RaptorQ_type::RQ_DEC_16:
        p_16 = reinterpret_cast<uint16_t*> (start);
        ret = reinterpret_cast<Decoder<uint16_t*, uint16_t*>*> (_decoder)->
                                                            decode_bytes (p_16,
                         reinterpret_cast<uint16_t*> (const_cast<void*> (end)),
                                                             from_byte, skip);
        start = p_16;
        break;
    case RaptorQ_type::RQ_DEC_32:
        p_32 = reinterpret_cast<uint32_t*> (start);
        ret = reinterpret_cast<Decoder<uint32_t*, uint32_t*>*> (_decoder)->
                                                            decode_bytes (p_32,
                         reinterpret_cast<uint32_t*> (const_cast<void*> (end)),
                                                             from_byte, skip);
        start = p_32;
        break;
    case RaptorQ_type::RQ_DEC_64:
        p_64 = reinterpret_cast<uint64_t*> (start);
        ret = reinterpret_cast<Decoder<uint64_t*, uint64_t*>*> (_decoder)->
                                                            decode_bytes (p_64,
                         reinterpret_cast<uint64_t*> (const_cast<void*> (end)),
                                                             from_byte, skip);
        start = p_64;
        break;
    case RaptorQ_type::RQ_ENC_8:
    case RaptorQ_type::RQ_ENC_16:
    case RaptorQ_type::RQ_ENC_32:
    case RaptorQ_type::RQ_ENC_64:
    case RaptorQ_type::RQ_NONE:
        break;
    }
    decode_pair p {ret.first, ret.second};
    return p;
}

} // namespace Impl
} // namespace RaptorQ__v1
