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


#pragma once

#include "RaptorQ/v1/common.hpp"
#include "RaptorQ/v1/wrapper/CPP_RFC_API_void.hpp"
#include <cmath>

///////////////////
//// Iterators
///////////////////
namespace RFC6330__v1 {

namespace It {
namespace Encoder {

template <typename Rnd_It, typename Fwd_It = Rnd_It>
class RAPTORQ_API Symbol
{
public:
    Symbol (Impl::Encoder_void *enc, const uint32_t id)
        : _enc (enc), _id (id) {}

    uint64_t operator() (Rnd_It &start, const Rnd_It end)
    {
        if (_enc == nullptr)
            return 0;
        return _enc->encode (start, end, _id);
    }
    uint32_t id() const
        { return _id; }

    uint8_t block() const
        { return static_cast<uint8_t> (_id >> 24); }

    uint32_t esi() const
    {
        uint32_t rev_mask = static_cast<uint32_t> (0xFF) << 24;
        return static_cast<uint8_t> (_id & ~rev_mask);
    }
private:
    Impl::Encoder_void *const _enc;
    const uint32_t _id;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<Rnd_It>>
{
public:
    Symbol_Iterator (Impl::Encoder_void *enc, const uint32_t id)
        : _enc (enc), _id (id) {}
    Symbol<Rnd_It> operator*()
        { return Symbol<Rnd_It> (_enc, _id); }
    Symbol_Iterator& operator++()
    {
        ++_id;
        return *this;
    }
    Symbol_Iterator operator++ (const int i) const
        { return Symbol_Iterator (_enc, _id + static_cast<uint32_t>(i)); }
    bool operator== (const Symbol_Iterator &it) const
        { return it._id == _id && it._enc == _enc; }
    bool operator!= (const Symbol_Iterator &it) const
        { return it._id != _id && it._enc == _enc; }
private:
    Impl::Encoder_void *const _enc;
    uint32_t _id;
};


template <typename Rnd_It, typename Fwd_It = Rnd_It>
class RAPTORQ_API Block
{
public:
    Block (Impl::Encoder_void *enc, const uint8_t block)
        : _enc (enc), _block (block) {}

    Symbol_Iterator<Rnd_It, Fwd_It> begin_source() const
    {
        if (_enc == nullptr)
            return Symbol_Iterator<Rnd_It, Fwd_It> (nullptr, 0);
        const uint32_t id = static_cast<uint32_t> (_block) << 24;
        return Symbol_Iterator<Rnd_It, Fwd_It> (_enc, id);
    }
    Symbol_Iterator<Rnd_It, Fwd_It> end_source() const
    {
        if (_enc == nullptr)
            return Symbol_Iterator<Rnd_It, Fwd_It> (nullptr, 0);
        const uint32_t id = static_cast<uint32_t> (_block) << 24;
        return Symbol_Iterator<Rnd_It, Fwd_It> (_enc,
                                        id + _enc->symbols (id));
    }
    Symbol_Iterator<Rnd_It, Fwd_It> begin_repair() const
        { return end_source(); }
    Symbol_Iterator<Rnd_It, Fwd_It> end_repair (uint32_t repairs) const
    {
        if (repairs > std::pow (2, 20))
            repairs = static_cast<uint32_t> (std::pow (2, 20));
        if (_enc == nullptr)
            return Symbol_Iterator<Rnd_It, Fwd_It> (nullptr, 0);
        const uint32_t id = static_cast<uint32_t> (_block) << 24;
        return Symbol_Iterator<Rnd_It, Fwd_It> (_enc,
                                            id + _enc->symbols (id) + repairs);
    }

    uint8_t id() const
        { return _block; }

    uint32_t max_repair() const
    {
        if (_enc == nullptr)
            return 0;
        return _enc->max_repair (_block);
    }

    uint16_t symbols() const
    {
        if (_enc == nullptr)
            return 0;
        return _enc->symbols (_block);
    }

    uint32_t block_size() const
    {
        if (_enc == nullptr)
            return 0;
        return _enc->block_size (_block);
    }
private:
    Impl::Encoder_void *const _enc;
    const uint8_t _block;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Block_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<Rnd_It>>
{
public:
    Block_Iterator (Impl::Encoder_void *enc, const uint8_t block)
        : _enc (enc), _block (block) {}
    Symbol<Rnd_It> operator*()
        { return Symbol<Rnd_It> (_enc, _block); }
    Block_Iterator& operator++()
    {
        ++_block;
        return *this;
    }
    Block_Iterator operator++ (const int i) const
        { return Block_Iterator (_enc, _block + static_cast<uint8_t>(i)); }
    bool operator== (const Block_Iterator &it) const
        { return it._block == _block && it._enc == _enc; }
    bool operator!= (const Block_Iterator &it) const
        { return it._block != _block && it._enc == _enc; }
private:
    Impl::Encoder_void *const _enc;
    uint8_t _block;
};

} // namespace Encoder

/////////////////
//// Decoder
/////////////////

namespace Decoder {

template <typename Rnd_It, typename Fwd_It = Rnd_It>
class RAPTORQ_API Symbol
{
public:
    Symbol (Impl::Decoder_void *dec, const uint32_t id)
        : _dec (dec), _id (id) {}

    uint64_t operator() (Rnd_It &start, const Rnd_It end)
    {
        if (_id == nullptr)
            return 0;
        // FIXME: we can't decode wach symbol separately? :(
        //return _dec->decode (start, end, _id);
    }
    uint32_t id() const
        { return _id; }

    uint8_t block() const
        { return static_cast<uint8_t> (_id >> 24); }

    uint32_t esi() const
    {
        uint32_t rev_mask = static_cast<uint32_t> (0xFF) << 24;
        return static_cast<uint8_t> (_id & ~rev_mask);
    }
private:
    Impl::Decoder_void *const _dec;
    const uint32_t _id;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<Rnd_It>>
{
public:
    Symbol_Iterator (Impl::Decoder_void *dec, const uint32_t id)
        : _dec (dec), _id (id) {}
    Symbol<Rnd_It> operator*()
        { return Symbol<Rnd_It> (_dec, _id); }
    Symbol_Iterator& operator++()
    {
        ++_id;
        return *this;
    }
    Symbol_Iterator operator++ (const int i) const
        { return Symbol_Iterator (_dec, _id + static_cast<uint32_t>(i)); }
    bool operator== (const Symbol_Iterator &it) const
        { return it._id == _id && it._dec == _dec; }
    bool operator!= (const Symbol_Iterator &it) const
        { return it._id != _id && it._dec == _dec; }
private:
    Impl::Decoder_void *const _dec;
    uint32_t _id;
};


template <typename Rnd_It, typename Fwd_It = Rnd_It>
class RAPTORQ_API Block
{
public:
    Block (Impl::Decoder_void *dec, const uint8_t block)
        : _dec (dec), _block (block) {}

    Symbol_Iterator<Rnd_It, Fwd_It> begin() const
    {
        if (_dec == nullptr)
            return Symbol_Iterator<Rnd_It, Fwd_It> (nullptr, 0);
        const uint32_t id = static_cast<uint32_t> (_block) << 24;
        return Symbol_Iterator<Rnd_It, Fwd_It> (_dec, id);
    }
    Symbol_Iterator<Rnd_It, Fwd_It> end() const
    {
        if (_dec == nullptr)
            return Symbol_Iterator<Rnd_It, Fwd_It> (nullptr, 0);
        const uint32_t id = static_cast<uint32_t> (_block) << 24;
        return Symbol_Iterator<Rnd_It, Fwd_It> (_dec, id + _dec->symbols (id));
    }
    uint8_t id() const
        { return _block; }

    uint16_t symbols() const
    {
        if (_dec == nullptr)
            return 0;
        return _dec->symbols (_block);
    }

    uint32_t block_size() const
    {
        if (_dec == nullptr)
            return 0;
        return _dec->block_size (_block);
    }

private:
    Impl::Decoder_void *const _dec;
    const uint8_t _block;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Block_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<Rnd_It>>
{
public:
    Block_Iterator (Impl::Decoder_void *dec, const uint8_t block)
        : _dec (dec), _block (block) {}
    Symbol<Rnd_It> operator*()
        { return Symbol<Rnd_It> (_dec, _block); }
    Block_Iterator& operator++()
    {
        ++_block;
        return *this;
    }
    Block_Iterator operator++ (const int i) const
        { return Block_Iterator (_dec, _block + static_cast<uint8_t>(i)); }
    bool operator== (const Block_Iterator &it) const
        { return it._block == _block && it._dec == _dec; }
    bool operator!= (const Block_Iterator &it) const
        { return it._block != _block && it._dec == _dec; }
private:
    Impl::Decoder_void *const _dec;
    uint8_t _block;
};

} // namespace Decoder


} // namespace It
} // namespace Rfc6330__v1

