/*
 * Copyright (c) 2015-2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#include "RaptorQ/v1/RaptorQ_Iterators.hpp"
#include <vector>
#if __cplusplus >= 201103L || _MSC_VER > 1900
    #include <future>
#endif


// There are a lot of NON-C++11 things here.
// there is also a lot of void-casting.
// However,  I could not find a nicer way of hiding the implementation
// and make this impervious to things like linking against things which
// have different stl, or different C++ version.
// This is ugly. I know. no other way?

#if __cplusplus >= 201103L || _MSC_VER > 1900
#define RQ_EXPLICIT explicit
#else
#define RQ_EXPLICIT
#endif

namespace RaptorQ__v1 {

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Encoder
{
public:
    Encoder (const Block_Size symbols, const size_t symbol_size);
    ~Encoder();
    Encoder() = delete;
    Encoder (const Encoder&) = delete;
    Encoder& operator= (const Encoder&) = delete;
    Encoder (Encoder &&) = default;
    Encoder& operator= (Encoder &&) = default;
    RQ_EXPLICIT operator bool() const;

    uint16_t symbols() const;
    size_t symbol_size() const;
    uint32_t max_repair() const;

    // these are NOT the Encoder_void ones!
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> begin_source();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> end_source();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> begin_repair();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> end_repair (
                                                        const uint32_t repair);

    bool has_data() const;
    size_t set_data (const Rnd_It &from, const Rnd_It &to);
    void clear_data();
    bool ready() const;
    void stop();

    bool precompute_sync();
    bool compute_sync();
    #if __cplusplus >= 201103L || _MSC_VER > 1900
    std::shared_future<Error> precompute();
    std::shared_future<Error> compute();
    #endif

    size_t encode (Fwd_It &output, const Fwd_It end, const uint32_t id);

private:
    Impl::Encoder_void _encoder;
};

template <typename In_It, typename Fwd_It>
class RAPTORQ_API Decoder
{
public:
    using Report = Dec_Report;

    ~Decoder();
    Decoder (const Block_Size symbols, const size_t symbol_size,
                                                            const Report type);
    Decoder (const Decoder&) = delete;
    Decoder& operator= (const Decoder&) = delete;
    Decoder (Decoder &&) = default;
    Decoder& operator= (Decoder &&) = default;
    RQ_EXPLICIT operator bool() const;

    uint16_t symbols() const;
    size_t symbol_size() const;

    // these are NOT the Encoder_void ones!
    RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> begin();
    RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> end();

    Error add_symbol (In_It &from, const In_It to, const uint32_t esi);
    std::vector<bool> end_of_input (const Fill_With_Zeros fill);

    bool can_decode() const;
    bool ready() const;
    void stop();
    void clear_data();
    uint16_t needed_symbols() const;

    void set_max_concurrency (const uint16_t max_threads);
    Decoder_Result decode_once();

    Decoder_wait_res poll();
    Decoder_wait_res wait_sync();
    #if __cplusplus >= 201103L || _MSC_VER > 1900
    std::future<Decoder_wait_res> wait();
    #endif

    Error decode_symbol (Fwd_It &start, const Fwd_It end, const uint16_t esi);
    // returns numer of bytes written, offset of data in last iterator
    Decoder_written decode_bytes (Fwd_It &start, const Fwd_It end,
                                    const size_t from_byte, const size_t skip);
private:
    Impl::Decoder_void _decoder;
};


///////////////////
//// Encoder
///////////////////


template <>
inline Encoder<uint8_t*, uint8_t*>::Encoder (const Block_Size symbols,
                                                    const size_t symbol_size)
    : _encoder (RaptorQ_type::RQ_ENC_8, symbols, symbol_size)
    {}

template <>
inline Encoder<uint16_t*, uint16_t*>::Encoder (const Block_Size symbols,
                                                    const size_t symbol_size)
    : _encoder (RaptorQ_type::RQ_ENC_16, symbols, symbol_size)
    {}

template <>
inline Encoder<uint32_t*, uint32_t*>::Encoder (const Block_Size symbols,
                                                    const size_t symbol_size)
    : _encoder (RaptorQ_type::RQ_ENC_32, symbols, symbol_size)
    {}

template <>
inline Encoder<uint64_t, uint64_t*>::Encoder (const Block_Size symbols,
                                                    const size_t symbol_size)
    : _encoder (RaptorQ_type::RQ_ENC_64, symbols, symbol_size)
    {}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::~Encoder()
    {}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::operator bool() const
    { return static_cast<bool> (_encoder); }

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbols() const
    { return _encoder.symbols(); }

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::symbol_size() const
    { return _encoder.symbol_size(); }

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::max_repair() const
    { return _encoder.max_repair(); }

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
                                        Encoder<Rnd_It, Fwd_It>::begin_source()
{
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (
                                                                &_encoder, 0);
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
                                        Encoder<Rnd_It, Fwd_It>::end_source ()
{
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (&_encoder,
                                                                    symbols());
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
                                        Encoder<Rnd_It, Fwd_It>::begin_repair()
{
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (&_encoder,
                                                                    symbols());
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
                    Encoder<Rnd_It, Fwd_It>::end_repair (const uint32_t repair)
{
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (&_encoder,
                                                            symbols() + repair);
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::has_data() const
    { return _encoder.has_data(); }

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::set_data (const Rnd_It &from, const Rnd_It &to)
{
    const void *_from = reinterpret_cast<const void*> (from);
    const void *_to = reinterpret_cast<const void*> (to);
    return _encoder.set_data (_from, _to);
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::clear_data()
    { _encoder.clear_data(); }

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::ready() const
    { _encoder.ready(); }

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::stop()
    { _encoder.stop(); }

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::precompute_sync()
    { return _encoder.precompute_sync(); }

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::compute_sync()
    { return _encoder.compute_sync(); }

#if __cplusplus >= 201103L
template <typename Rnd_It, typename Fwd_It>
std::shared_future<Error> Encoder<Rnd_It, Fwd_It>::precompute()
    { return _encoder.precompute(); }

template <typename Rnd_It, typename Fwd_It>
std::shared_future<Error> Encoder<Rnd_It, Fwd_It>::compute()
    { return _encoder.compute(); }
#endif

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
                                                            const uint32_t id)
{
    void **_from = reinterpret_cast<void**> (&output);
    void *_to = reinterpret_cast<void*> (end);
    auto ret = _encoder.encode (_from, _to, id);
    Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
    output = *tmp;
    return ret;
}

///////////////////
//// Decoder
///////////////////


template <>
inline Decoder<uint8_t*, uint8_t*>::Decoder (const Block_Size symbols,
                                                    const size_t symbol_size,
                                                    const Dec_Report type)
    : _decoder (RaptorQ_type::RQ_DEC_8, symbols, symbol_size, type)
    {}

template <>
inline Decoder<uint16_t*, uint16_t*>::Decoder (const Block_Size symbols,
                                                     const size_t symbol_size,
                                                     const Dec_Report type)
    :_decoder (RaptorQ_type::RQ_DEC_16, symbols, symbol_size, type)
    {}

template <>
inline Decoder<uint32_t*, uint32_t*>::Decoder (const Block_Size symbols,
                                                     const size_t symbol_size,
                                                     const Dec_Report type)
    :_decoder (RaptorQ_type::RQ_DEC_32, symbols, symbol_size, type)
    {}

template <>
inline Decoder<uint64_t*, uint64_t*>::Decoder (const Block_Size symbols,
                                                     const size_t symbol_size,
                                                     const Dec_Report type)
    :_decoder (RaptorQ_type::RQ_DEC_64, symbols, symbol_size, type)
    {}

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::~Decoder()
    {}


template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::operator bool() const
    { return static_cast<bool> (_decoder); }

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbols() const
    { return _decoder.symbols(); }

template <typename In_It, typename Fwd_It>
size_t Decoder<In_It, Fwd_It>::symbol_size() const
    { return _decoder.symbol_size(); }

template <typename In_It, typename Fwd_It>
RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It>
                                                Decoder<In_It, Fwd_It>::begin()
{
    return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (&_decoder,
                                                                            0);
}

template <typename In_It, typename Fwd_It>
RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It>
                                                Decoder<In_It, Fwd_It>::end()
{
    return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (
                                                        &_decoder, symbols());
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::needed_symbols() const
    { return _decoder.needed_symbols(); }

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::add_symbol (In_It &from, const In_It to,
                                                            const uint32_t esi)
{
    void **_from = reinterpret_cast<void**> (&from);
    void *_to = reinterpret_cast<void*> (to);
    auto ret = _decoder.add_symbol (_from, _to, esi);
    In_It *tmp = reinterpret_cast<In_It*> (_from);
    from = *tmp;
    return ret;
}

template <typename In_It, typename Fwd_It>
std::vector<bool> Decoder<In_It, Fwd_It>::end_of_input (
                                                    const Fill_With_Zeros fill)
    { return _decoder.end_of_input (fill); }

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::set_max_concurrency (const uint16_t max_threads)
    { return _decoder.set_max_concurrency (max_threads); }

template <typename In_It, typename Fwd_It>
Decoder_Result Decoder<In_It, Fwd_It>::decode_once()
    { return _decoder.decode_once(); }

template <typename In_It, typename Fwd_It>
Decoder_wait_res Decoder<In_It, Fwd_It>::poll()
    { return _decoder.poll(); }

template <typename In_It, typename Fwd_It>
Decoder_wait_res Decoder<In_It, Fwd_It>::wait_sync()
    { return _decoder.wait_sync(); }

#if __cplusplus >= 201103L || _MSC_VER > 1900
template <typename In_It, typename Fwd_It>
std::future<struct Decoder_wait_res> Decoder<In_It, Fwd_It>::wait()
    { return _decoder.wait(); }
#endif

template <typename In_It, typename Fwd_It>
bool Decoder<In_It, Fwd_It>::can_decode() const
    { return _decoder.can_decode(); }

template <typename In_It, typename Fwd_It>
bool Decoder<In_It, Fwd_It>::ready() const
    { return _decoder.ready(); }

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::stop()
    { _decoder.stop(); }

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::clear_data()
    { _decoder.clear_data(); }

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::decode_symbol (Fwd_It &start, const Fwd_It end,
                                                            const uint16_t esi)
{
    void **_from = reinterpret_cast<void**> (&start);
    void *_to = reinterpret_cast<void*> (end);
    auto ret = _decoder.decode_symbol (_from, _to, esi);
    Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
    start = *tmp;
    return ret;
}

template <typename In_It, typename Fwd_It>
Decoder_written Decoder<In_It, Fwd_It>::decode_bytes (Fwd_It &start,
                                                        const Fwd_It end,
                                                        const size_t from_byte,
                                                        const size_t skip)
{
    void **_from = reinterpret_cast<void**> (&start);
    void *_to = reinterpret_cast<void*> (end);
    auto ret = _decoder.decode_bytes (_from, _to, from_byte, skip);
    Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
    start = *tmp;
    return ret;
}

}   // namespace RaptorQ__v1
