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
#include "RaptorQ/v1/wrapper/CPP_RAW_API_void.hpp"
#include <future>


// There are a lot of NON-C++11 things here.
// there is also a lot of void-casting.
// However,  I could not find a nicer way of hiding the implementation
// and make this impervious to things like linking against things which
// have different stl, or different C++ version.
// This is ugly. I know. no other way?


namespace RaptorQ__v1 {

namespace It {
    namespace Encoder {
        template <typename Rnd_It, typename Fwd_It = Rnd_It>
        class RAPTORQ_API Symbol_Iterator;
    } // namespace Encoder
    namespace Decoder {
        template <typename Rnd_It, typename Fwd_It = Rnd_It>
        class RAPTORQ_API Symbol_Iterator;
    } // namespace Decoder
} // namespace It

template <typename Rnd_It, typename Fwd_It = Rnd_It>
class RAPTORQ_API Encoder
{
public:
    Encoder (const Block_Size symbols, const size_t symbol_size);
    ~Encoder();
    explicit operator bool() const;

    uint16_t symbols() const;
    size_t symbol_size() const;
    uint32_t max_repair() const;

    // these are NOT the Encoder_void ones!
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> begin_source();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> end_source();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> begin_repair();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> end_repair (
                                                        const uint32_t repair);

    bool has_data() const;
    size_t set_data (const Rnd_It &from, const Rnd_It &to);
    void clear_data();
    void stop();

    bool precompute_sync();
    bool compute_sync();
    std::future<Error> precompute();
    std::future<Error> compute();

    size_t encode (Fwd_It &output, const Fwd_It end, const uint32_t id);

private:
    Impl::Encoder_void *_encoder;
};

template <typename Rnd_It, typename Fwd_It = Rnd_It>
class RAPTORQ_API Decoder
{
public:
    using Report = RaptorQ__v1::Impl::Dec_Report;

    Decoder (const Block_Size symbols, const size_t symbol_size,
                                                            const Report type);
    explicit operator bool() const;

    uint16_t symbols() const;
    size_t symbol_size() const;

    // these are NOT the Encoder_void ones!
    RaptorQ__v1::It::Decoder::Symbol_Iterator<Rnd_It> begin();
    RaptorQ__v1::It::Decoder::Symbol_Iterator<Rnd_It> end();

    Error add_symbol (Rnd_It &from, const Rnd_It to, const uint32_t esi);
    void end_of_input();

    bool can_decode() const;
    void stop();
    uint16_t needed_symbols() const;

    void set_max_concurrency (const uint16_t max_threads);
    using Decoder_Result = Impl::Decoder_Result;
    Decoder_Result decode_once();
    std::pair<Error, uint16_t> poll();
    std::pair<Error, uint16_t> wait_sync();
    std::future<std::pair<Error, uint16_t>> wait();

    Error decode_symbol (Fwd_It &start, const Fwd_It end, const uint16_t esi);
    // returns numer of bytes written, offset of data in last iterator
    std::pair<size_t, size_t> decode_bytes (Fwd_It &start, const Fwd_It end,
                                    const size_t from_byte, const size_t skip);
private:
    Impl::Decoder_void *_decoder;
};



///////////////////
//// Iterators
///////////////////

namespace It {
namespace Encoder {

template <typename Rnd_It, typename Fwd_It = Rnd_It>
class RAPTORQ_API Symbol
{
public:
    Symbol (Impl::Encoder_void *enc, const uint32_t esi)
        : _enc (enc), _esi (esi) {}

    uint64_t operator() (Rnd_It &start, const Rnd_It end)
    {
        if (_enc == nullptr)
            return 0;
        return _enc->encode (start, end, _esi);
    }
    uint32_t id() const
        { return _esi; }
private:
    Impl::Encoder_void *const _enc;
    const uint32_t _esi;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<Rnd_It>>
{
public:
    Symbol_Iterator (Impl::Encoder_void *enc, const uint32_t esi)
        : _enc (enc), _esi (esi) {}
    Symbol<Rnd_It> operator*()
        { return Symbol<Rnd_It> (_enc, _esi); }
    Symbol_Iterator& operator++()
    {
        ++_esi;
        return *this;
    }
    Symbol_Iterator operator++ (const int i) const
        { return Symbol_Iterator (_enc, _esi + static_cast<uint32_t>(i)); }
    bool operator== (const Symbol_Iterator &it) const
        { return it._esi == _esi; }
    bool operator!= (const Symbol_Iterator &it) const
        { return it._esi != _esi; }
private:
    Impl::Encoder_void *const _enc;
    uint32_t _esi;
};
} // namespace Encoder


namespace Decoder {
template <typename Rnd_It, typename Fwd_It = Rnd_It>
class RAPTORQ_API Symbol
{
public:
    Symbol (Impl::Decoder_void *dec, const uint16_t esi)
        : _dec (dec), _esi (esi) {}

    Error operator() (Rnd_It &start, const Rnd_It end)
    {
        if (_dec == nullptr)
            return Error::INITIALIZATION;
        return _dec->decode_symbol (start, end, _esi);
    }
    uint32_t id() const
        { return _esi; }
private:
    Impl::Decoder_void *const _dec;
    const uint16_t _esi;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<Rnd_It>>
{
public:
    Symbol_Iterator (Impl::Decoder_void *dec, const uint16_t esi)
        : _dec (dec), _esi (esi) {}
    Symbol<Rnd_It> operator*()
        { return Symbol<Rnd_It> (_dec, _esi); }
    Symbol_Iterator& operator++()
    {
        ++_esi;
        return *this;
    }
    Symbol_Iterator operator++ (const int i) const
        { return Symbol_Iterator (_dec, _esi + static_cast<uint16_t> (i));}
    bool operator== (const Symbol_Iterator &it) const
        { return it._esi == _esi; }
    bool operator!= (const Symbol_Iterator &it) const
        { return it._esi != _esi; }
private:
    Impl::Decoder_void *_dec;
    uint16_t _esi;
};

} // namespace Decoder
} // namespace It



///////////////////
//// Encoder
///////////////////


template <>
Encoder<uint8_t*>::Encoder (const Block_Size symbols, const size_t symbol_size)
{
    _encoder =  new Impl::Encoder_void (RaptorQ_type::RQ_ENC_8,
                                                        symbols, symbol_size);
}

template <>
Encoder<uint16_t*>::Encoder (const Block_Size symbols, const size_t symbol_size)
{
    _encoder =  new Impl::Encoder_void (RaptorQ_type::RQ_ENC_16,
                                                        symbols, symbol_size);
}

template <>
Encoder<uint32_t*>::Encoder (const Block_Size symbols, const size_t symbol_size)
{
    _encoder =  new Impl::Encoder_void (RaptorQ_type::RQ_ENC_32,
                                                        symbols, symbol_size);
}

template <>
Encoder<uint64_t*>::Encoder (const Block_Size symbols, const size_t symbol_size)
{
    _encoder =  new Impl::Encoder_void (RaptorQ_type::RQ_ENC_64,
                                                        symbols, symbol_size);
}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::Encoder (const Block_Size symbols,
                                                    const size_t symbol_size)
{
    RQ_UNUSED (symbols);
    RQ_UNUSED (symbol_size);
    static_assert (false,
            "RaptorQ: sorry, only uint8_t*, uint16_t*, uint32_t*, uint64_t*, "
                "supported for the linked library. For more please use the "
                                                        "header-only version");
}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::~Encoder()
    { delete _encoder; }

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::operator bool() const
{
    if (_encoder == nullptr)
        return 0;
    return static_cast<bool> (*_encoder);
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbols() const
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->symbols();
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::symbol_size() const
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->symbol_size();
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::max_repair() const
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->max_repair();
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It>
                                        Encoder<Rnd_It, Fwd_It>::begin_source()
{
    if (_encoder == nullptr)
        return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> (nullptr, 0);
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> (this, 0);
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It>
                                        Encoder<Rnd_It, Fwd_It>::end_source ()
{
    if (_encoder == nullptr)
        return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> (nullptr, 0);
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> (this, symbols());
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It>
                                        Encoder<Rnd_It, Fwd_It>::begin_repair()
{
    if (_encoder == nullptr)
        return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> (nullptr, 0);
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> (this, symbols());
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It>
                    Encoder<Rnd_It, Fwd_It>::end_repair (const uint32_t repair)
{
    if (_encoder == nullptr)
        return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> (nullptr, 0);
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It> (this,
                                                            symbols() + repair);
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::has_data() const
{
    if (_encoder == nullptr)
        return false;
    return _encoder->has_data();
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::set_data (const Rnd_It &from, const Rnd_It &to)
{
    if (_encoder == nullptr)
        return 0;
    // trying to avoid stl incompatibilities can lead to headhaches...
    using T = typename std::iterator_traits<Rnd_It>::value_type;
    const void *_from = reinterpret_cast<void*> (reinterpret_cast<T*> (&*from));
    const void *_to = reinterpret_cast<void*> (
                                reinterpret_cast<T*> (&*from) + (to - from));
    return _encoder->set_data (_from, _to);
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::clear_data()
{
    if (_encoder == nullptr)
        return;
    _encoder->clear_data();
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::stop()
{
    if (_encoder == nullptr)
        return;
    _encoder->stop();
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::precompute_sync()
{
    if (_encoder == nullptr)
        return false;
    return _encoder->precompute_sync();
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::compute_sync()
{
    if (_encoder == nullptr)
        return false;
    return _encoder->compute_sync();
}

template <typename Rnd_It, typename Fwd_It>
std::future<Error> Encoder<Rnd_It, Fwd_It>::precompute()
{
    if (_encoder == nullptr) {
        std::promise<Error> p;
        p.set_value (Error::INITIALIZATION);
        return p.get_future();
    }
    return _encoder->precompute();
}

template <typename Rnd_It, typename Fwd_It>
std::future<Error> Encoder<Rnd_It, Fwd_It>::compute()
{
    if (_encoder == nullptr) {
        std::promise<Error> p;
        p.set_value (Error::INITIALIZATION);
        return p.get_future();
    }
    return _encoder->compute();
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
                                                            const uint32_t id)
{
    if (_encoder == nullptr)
        return 0;
    // trying to avoid stl incompatibilities can lead to headhaches...
    using T = typename std::iterator_traits<Fwd_It>::value_type;
    void *_from = reinterpret_cast<void*> (reinterpret_cast<T*> (&*output));
    void *_to = reinterpret_cast<void*> (
                            reinterpret_cast<T*> (&*output) + (end - output));
    auto ret = _encoder->encode (_from, _to, id);
    output += reinterpret_cast<T*>(_to) - reinterpret_cast<T*>(_from);
    return ret;
}

///////////////////
//// Decoder
///////////////////


template <>
Decoder<uint8_t*>::Decoder (const Block_Size symbols, const size_t symbol_size,
                                        const Decoder<uint8_t*>::Report type)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_8,
                                                    symbols, symbol_size, type);
}

template <>
Decoder<uint16_t*>::Decoder (const Block_Size symbols, const size_t symbol_size,
                                        const Decoder<uint16_t*>::Report type)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_16,
                                                    symbols, symbol_size, type);
}

template <>
Decoder<uint32_t*>::Decoder (const Block_Size symbols, const size_t symbol_size,
                                        const Decoder<uint32_t*>::Report type)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_32,
                                                    symbols, symbol_size, type);
}

template <>
Decoder<uint64_t*>::Decoder (const Block_Size symbols, const size_t symbol_size,
                                        const Decoder<uint64_t*>::Report type)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_64,
                                                    symbols, symbol_size, type);
}

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::Decoder (const Block_Size symbols,
                                const size_t symbol_size, const Report type)
{
    RQ_UNUSED (symbols);
    RQ_UNUSED (symbol_size);
    RQ_UNUSED (type);
    static_assert (false,
                  "RaptorQ decoder: sorry, only uint8_t*, uint16_t*, uint32t*, "
                  "uint64_t* are suppoted here. For other tyeps please use "
                  "the header-only version.");
}

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::operator bool() const
{
    if (_decoder == nullptr)
        return 0;
    return static_cast<bool> (*_decoder);
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbols() const
{
    if (_decoder == nullptr)
        return 0;
    return _decoder->symbols();
}

template <typename In_It, typename Fwd_It>
size_t Decoder<In_It, Fwd_It>::symbol_size() const
{
    if (_decoder == nullptr)
        return 0;
    return _decoder->symbol_size();
}

template <typename In_It, typename Fwd_It>
RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It>
                                                Decoder<In_It, Fwd_It>::begin()
{
    if (_decoder == nullptr)
        return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (
                                                                    nullptr, 0);
    return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (this, 0);
}

template <typename In_It, typename Fwd_It>
RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It>
                                                Decoder<In_It, Fwd_It>::end()
{
    if (_decoder == nullptr)
        return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (
                                                                    nullptr, 0);
    return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (
                                                            this, symbols());
}


template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::needed_symbols() const
{
    if (_decoder == nullptr)
        return 0;
    return _decoder->needed_symbols();
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::add_symbol (In_It &from, const In_It to,
                                                            const uint32_t esi)
{
    if (_decoder == nullptr)
        return Error::INITIALIZATION;
    // trying to avoid stl incompatibilities can lead to headhaches...
    using T = typename std::iterator_traits<In_It>::value_type;
    void *_from = reinterpret_cast<void*> (reinterpret_cast<T*> (&*from));
    void *_to = reinterpret_cast<void*> (
                                reinterpret_cast<T*> (&*from) + (to - from));
    auto ret = _decoder->add_symbol (_from, _to, esi);
    from += reinterpret_cast<T*>(_to) - reinterpret_cast<T*>(_from);
    return ret;
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::end_of_input()
{
    if (_decoder != nullptr)
        return _decoder->end_of_input();
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::set_max_concurrency (const uint16_t max_threads)
{
    if (_decoder != nullptr)
        return _decoder->set_max_concurrency (max_threads);
}

template <typename In_It, typename Fwd_It>
typename Decoder<In_It, Fwd_It>::Decoder_Result
                                        Decoder<In_It, Fwd_It>::decode_once()
{
    if (_decoder == nullptr)
        return Decoder<In_It, Fwd_It>::Decoder_Result::NEED_DATA;
    return _decoder->decode_once();
}

template <typename In_It, typename Fwd_It>
std::pair<Error, uint16_t> Decoder<In_It, Fwd_It>::poll()
{
    if (_decoder == nullptr)
        return {Error::INITIALIZATION, 0};
    return _decoder->poll();
}

template <typename In_It, typename Fwd_It>
std::pair<Error, uint16_t> Decoder<In_It, Fwd_It>::wait_sync()
{
    if (_decoder == nullptr) {
        return {Error::INITIALIZATION, 0};
    }
    return _decoder->wait_sync();
}

template <typename In_It, typename Fwd_It>
std::future<std::pair<Error, uint16_t>> Decoder<In_It, Fwd_It>::wait()
{
    if (_decoder == nullptr) {
        std::promise<std::pair<Error, uint16_t>> p;
        p.set_value ({Error::INITIALIZATION, 0});
        return p.get_future();
    }
    return _decoder->wait();
}

template <typename In_It, typename Fwd_It>
bool Decoder<In_It, Fwd_It>::can_decode() const
{
    if (_decoder == nullptr)
        return false;
    return _decoder->can_decode();
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::stop()
{
    if (_decoder != nullptr)
        _decoder->stop();
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::decode_symbol (Fwd_It &start, const Fwd_It end,
                                                            const uint16_t esi)
{
    if (_decoder == nullptr || end < start)
        return Error::INITIALIZATION;
    // trying to avoid stl incompatibilities can lead to headhaches...
    using T = typename std::iterator_traits<Fwd_It>::value_type;
    void *_from = reinterpret_cast<void*> (reinterpret_cast<T*> (&*start));
    void *_to = reinterpret_cast<void*> (
                                reinterpret_cast<T*> (&*start) + (end - start));
    auto ret = _decoder->decode_symbol (_from, _to, esi);
    start += reinterpret_cast<T*>(_to) - reinterpret_cast<T*>(_from);
    return ret;
}

template <typename In_It, typename Fwd_It>
std::pair<size_t, size_t> Decoder<In_It, Fwd_It>::decode_bytes (Fwd_It &start,
                                                    const Fwd_It end,
                                                    const size_t from_byte,
                                                    const size_t skip)
{
    if (_decoder == nullptr)
        return {0, skip};
    return _decoder->decode_bytes (start, end, from_byte, skip);
}

}   // namespace RaptorQ__v1
