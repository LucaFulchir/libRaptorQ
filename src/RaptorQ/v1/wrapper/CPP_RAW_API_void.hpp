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

#include "RaptorQ/v1/block_sizes.hpp"
#include "RaptorQ/v1/common.hpp"
#include "RaptorQ/v1/RaptorQ.hpp"
#include "RaptorQ/v1/wrapper/C_common.h"
#include <future>
#include <utility>


// There are a lot of NON-C++11 things here.
// there is also a lot of void-casting.
// However,  I could not find a nicer way of hiding the implementation
// and make this impervious to things like linking against things which
// have different stl, or different C++ version.
// This is ugly. I know. no other way?

// For added safety, please use the header-only library


namespace RaptorQ__v1 {
namespace Impl {

namespace It {
    namespace Encoder {
        class RAPTORQ_API Symbol_Iterator_void;
    } // namespace Encoder
    namespace Decoder {
        class RAPTORQ_API Symbol_Iterator_void;
    } // namespace Decoder
} // namespace It

class RAPTORQ_API Encoder_void final
{
public:
    Encoder_void (const RaptorQ_type type, const Block_Size symbols,
                                                    const size_t symbol_size);
    ~Encoder_void();
    explicit operator bool() const;

    uint16_t symbols() const;
    size_t symbol_size() const;
    uint32_t max_repair() const;

    // Iterators are exposed for mad people only.
    // please do not use this interface directly.
    // include CPP_RAW_API.hpp
    It::Encoder::Symbol_Iterator_void begin_source();
    It::Encoder::Symbol_Iterator_void end_source();
    It::Encoder::Symbol_Iterator_void begin_repair();
    It::Encoder::Symbol_Iterator_void end_repair (const uint32_t repair);

    bool has_data() const;
    size_t set_data (const void* from, const void* to);
    void clear_data();
    void stop();

    bool precompute_sync();
    bool compute_sync();
    std::future<Error> precompute();
    std::future<Error> compute();

    size_t encode (void* &output, const void* end, const uint32_t id);

private:
    RaptorQ_type _type;
    void *_encoder;
};

class RAPTORQ_API Decoder_void
{
public:
    using Report = Dec_Report;

    Decoder_void (const RaptorQ_type type, const Block_Size symbols,
                            const size_t symbol_size, const Report computation);
    ~Decoder_void();
    explicit operator bool() const;

    uint16_t symbols() const;
    size_t symbol_size() const;

    // Iterators are exposed for mad people only.
    // please do not use this interface directly.
    // include CPP_RAW_API.hpp
    It::Decoder::Symbol_Iterator_void begin();
    It::Decoder::Symbol_Iterator_void end();

    Error add_symbol (void* &from, const void* to, const uint32_t esi);
    void end_of_input();

    bool can_decode() const;
    void stop();
    uint16_t needed_symbols() const;

    void set_max_concurrency (const uint16_t max_threads);
    using Decoder_Result = Decoder_Result;
    Decoder_Result decode_once();
    std::pair<Error, uint16_t> poll();
    std::pair<Error, uint16_t> wait_sync();
    std::future<std::pair<Error, uint16_t>> wait();

    Error decode_symbol (void* &start, const void* end, const uint16_t esi);
    // returns number of bytes written, offset of data in last iterator
    struct decode_pair {
        size_t written;
        size_t offset;
    };
    decode_pair decode_bytes (void* &start, const void* end,
                                    const size_t from_byte, const size_t skip);
private:
    RaptorQ_type _type;
    void *_decoder;
};



// Iterators:


namespace It {
namespace Encoder {

class RAPTORQ_API Symbol_void
{
public:
    Symbol_void (Impl::Encoder_void *enc, const uint32_t esi)
        : _enc (enc), _esi (esi) {}

    uint64_t operator() (void* &start, const void* end)
    {
        if (_enc == nullptr)
            return 0;
        return _enc->encode (start, end, _esi);
    }
    uint32_t id() const
        { return _esi; }
private:
    Encoder_void *const _enc;
    const uint32_t _esi;
};

class RAPTORQ_API Symbol_Iterator_void :
        public std::iterator<std::input_iterator_tag, Symbol_void>
{
public:
    Symbol_Iterator_void (Encoder_void *enc, const uint32_t esi)
        : _enc (enc), _esi (esi) {}
    Symbol_void operator*()
        { return Symbol_void (_enc, _esi); }
    Symbol_Iterator_void& operator++()
    {
        ++_esi;
        return *this;
    }
    Symbol_Iterator_void operator++ (const int i) const
        { return Symbol_Iterator_void (_enc, _esi + static_cast<uint32_t>(i)); }
    bool operator== (const Symbol_Iterator_void &it) const
        { return it._esi == _esi; }
    bool operator!= (const Symbol_Iterator_void &it) const
        { return it._esi != _esi; }
private:
    Encoder_void *const _enc;
    uint32_t _esi;
};
} // namespace Encoder


namespace Decoder {

class RAPTORQ_API Symbol_void
{
public:
    Symbol_void (Decoder_void *dec, const uint16_t esi)
        : _dec (dec), _esi (esi) {}

    Error operator() (void* &start, const void* end)
    {
        if (_dec == nullptr)
            return Error::INITIALIZATION;
        return _dec->decode_symbol (start, end, _esi);
    }
    uint32_t id() const
        { return _esi; }
private:
    Decoder_void *const _dec;
    const uint16_t _esi;
};

class RAPTORQ_API Symbol_Iterator_void :
        public std::iterator<std::input_iterator_tag, Symbol_void>
{
public:
    Symbol_Iterator_void (Decoder_void *dec, const uint16_t esi)
        : _dec (dec), _esi (esi) {}
    Symbol_void operator*()
        { return Symbol_void (_dec, _esi); }
    Symbol_Iterator_void& operator++()
    {
        ++_esi;
        return *this;
    }
    Symbol_Iterator_void operator++ (const int i) const
        { return Symbol_Iterator_void (_dec, _esi + static_cast<uint16_t> (i));}
    bool operator== (const Symbol_Iterator_void &it) const
        { return it._esi == _esi; }
    bool operator!= (const Symbol_Iterator_void &it) const
        { return it._esi != _esi; }
private:
    Decoder_void *_dec;
    uint16_t _esi;
};


} // namespace Decoder
} // namespace It




} // namespace Impl
} // namespace RaptorQ__v1
