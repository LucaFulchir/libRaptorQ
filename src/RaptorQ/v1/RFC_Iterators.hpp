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

#pragma once

#include "RaptorQ/v1/common.hpp"
#include "RaptorQ/v1/util/endianess.hpp"
#include <cmath>
#include <iterator>

// NOTE: this file is included from RFC.hpp and CPP_RFC_API.hpp

// Yes, another template parameter would keep us away from macros.
// But I prefer to kep the parameters the same as for the classes.
// So we keep the macros. (NOTE: this header requires C++98 compatibility)
#ifdef RQ_HEADER_ONLY
    #include "RaptorQ/v1/RFC.hpp"
    namespace RFC6330__v1 {
        namespace Impl {
            template<typename Rnd_It, typename Fwd_It>
            class Encoder;
            template<typename In_It, typename Fwd_It>
            class Decoder;
        }
    }
    #define RQ_ENC_T Impl::Encoder<Rnd_It, Fwd_It>
    #define RQ_DEC_T Impl::Decoder<In_It, Fwd_It>
#else
    #include "RaptorQ/v1/wrapper/CPP_RFC_API_void.hpp"
    #define RQ_ENC_T Impl::Encoder_void
    #define RQ_DEC_T Impl::Decoder_void
#endif

///////////////////
//// Iterators
///////////////////
namespace RFC6330__v1 {

namespace It {
namespace Encoder {

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol
{
public:
    Symbol (RQ_ENC_T *const enc, const uint32_t id)
        : _enc (enc), _id (id) {}
    Symbol() = delete;
    Symbol (const Symbol&) = default;
    Symbol& operator= (const Symbol&) = default;
    Symbol (Symbol&&) = default;
    Symbol& operator= (Symbol&&) = default;
    ~Symbol() = default;

    uint64_t operator() (Fwd_It &start, const Fwd_It end)
    {
        if (_enc == nullptr)
            return 0;
        #ifdef RQ_HEADER_ONLY
        return _enc->encode (start, end,
                            RaptorQ__v1::Impl::Endian::h_to_b<uint32_t> (_id));
        #else
        void **_from = reinterpret_cast<void**> (&start);
        void *_to = reinterpret_cast<void*> (end);
        uint64_t res = _enc->encode (_from, _to,
                            RaptorQ__v1::Impl::Endian::h_to_b<uint32_t> (_id));
        Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
        start = *tmp;
        return res;
        #endif
    }
    uint32_t id() const
        { return RaptorQ__v1::Impl::Endian::h_to_b<uint32_t> (_id); }

    uint8_t block() const
        { return static_cast<uint8_t> (_id >> 24); }

    uint32_t esi() const
    {
        constexpr uint32_t rev_mask = ~(static_cast<uint32_t> (0xFF) << 24);
        return _id & rev_mask;
    }
private:
    RQ_ENC_T *const _enc;
    // BIG NOTE: HOST endianness, remember to report in big-endian!
    const uint32_t _id;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<Rnd_It, Fwd_It>>
{
public:
    Symbol_Iterator (RQ_ENC_T *const enc, const uint32_t id)
        : _enc (enc), _id (id) {}
    Symbol_Iterator() = delete;
    Symbol_Iterator (const Symbol_Iterator&) = default;
    Symbol_Iterator& operator= (const Symbol_Iterator&) = default;
    Symbol_Iterator (Symbol_Iterator&&) = default;
    Symbol_Iterator& operator= (Symbol_Iterator&&) = default;
    ~Symbol_Iterator() = default;

    Symbol<Rnd_It, Fwd_It> operator*()
        { return Symbol<Rnd_It, Fwd_It> (_enc, _id); }
    Symbol_Iterator<Rnd_It, Fwd_It>& operator++()
    {
        ++_id;
        return *this;
    }
    Symbol_Iterator<Rnd_It, Fwd_It> operator++ (const int i) const
    {
        return Symbol_Iterator<Rnd_It, Fwd_It> (_enc, _id +
                                                    static_cast<uint32_t> (i));
    }
    bool operator== (const Symbol_Iterator<Rnd_It, Fwd_It> &it) const
        { return it._id == _id && it._enc == _enc; }
    bool operator!= (const Symbol_Iterator<Rnd_It, Fwd_It> &it) const
        { return it._id != _id && it._enc == _enc; }
private:
    RQ_ENC_T *const _enc;
    // BIG NOTE: HOST endianness, remember to report in big-endian!
    uint32_t _id;
};


template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Block
{
public:
    Block (RQ_ENC_T *const enc, const uint8_t block)
        : _enc (enc), _block (block) {}
    Block() = delete;
    Block (const Block&) = default;
    Block& operator= (const Block&) = default;
    Block (Block&&) = default;
    Block& operator= (Block&&) = default;
    ~Block() = default;

    Symbol_Iterator<Rnd_It, Fwd_It> begin_source() const
    {
        if (_enc == nullptr)
            return Symbol_Iterator<Rnd_It, Fwd_It> (nullptr, 0);
        const uint32_t host_endian_id = static_cast<uint32_t> (_block) << 24;
        return Symbol_Iterator<Rnd_It, Fwd_It> (_enc, host_endian_id);
    }
    Symbol_Iterator<Rnd_It, Fwd_It> end_source() const
    {
        if (_enc == nullptr)
            return Symbol_Iterator<Rnd_It, Fwd_It> (nullptr, 0);
        const uint32_t host_endian_id = (static_cast<uint32_t> (_block) << 24) +
                                            _enc->symbols (_block);
        return Symbol_Iterator<Rnd_It, Fwd_It> (_enc, host_endian_id);
    }
    Symbol_Iterator<Rnd_It, Fwd_It> begin_repair() const
        { return end_source(); }
    Symbol_Iterator<Rnd_It, Fwd_It> end_repair (uint32_t repairs) const
    {
        if (repairs > std::pow (2, 20))
            repairs = static_cast<uint32_t> (std::pow (2, 20));
        if (_enc == nullptr)
            return Symbol_Iterator<Rnd_It, Fwd_It> (nullptr, 0);
        const uint32_t host_endian_id = (static_cast<uint32_t> (_block) << 24) +
                                            _enc->symbols (_block) + repairs;
        return Symbol_Iterator<Rnd_It, Fwd_It> (_enc, host_endian_id);
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

    Block_Size extended_symbols() const
    {
        if (_enc == nullptr)
            return static_cast<Block_Size> (0);
        return _enc->extended_symbols (_block);
    }

    uint32_t block_size() const
    {
        if (_enc == nullptr)
            return 0;
        return _enc->block_size (_block);
    }
private:
    RQ_ENC_T *const _enc;
    const uint8_t _block;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Block_Iterator :
        public std::iterator<std::input_iterator_tag, Block<Rnd_It, Fwd_It>>
{
public:
    Block_Iterator (RQ_ENC_T *const enc, const uint8_t block)
        : _enc (enc), _block (block) {}
    Block_Iterator() = delete;
    Block_Iterator (const Block_Iterator&) = default;
    Block_Iterator& operator= (const Block_Iterator&) = default;
    Block_Iterator (Block_Iterator&&) = default;
    Block_Iterator& operator= (Block_Iterator&&) = default;
    ~Block_Iterator() = default;

    Block<Rnd_It, Fwd_It> operator*()
        { return Block<Rnd_It, Fwd_It> (_enc, _block); }
    Block_Iterator<Rnd_It, Fwd_It>& operator++()
    {
        ++_block;
        return *this;
    }
    Block_Iterator<Rnd_It, Fwd_It> operator++ (const int i) const
    {
        return Block_Iterator<Rnd_It, Fwd_It> (_enc, _block +
                                                    static_cast<uint8_t> (i));
    }
    bool operator== (const Block_Iterator<Rnd_It, Fwd_It> &it) const
        { return it._block == _block && it._enc == _enc; }
    bool operator!= (const Block_Iterator<Rnd_It, Fwd_It> &it) const
        { return it._block != _block && it._enc == _enc; }
private:
    RQ_ENC_T *const _enc;
    uint8_t _block;
};

} // namespace Encoder

/////////////////
//// Decoder
/////////////////

namespace Decoder {

template <typename In_It, typename Fwd_It>
class RAPTORQ_API Symbol
{
public:
    Symbol (RQ_DEC_T *const dec, const uint32_t id)
        : _dec (dec), _id (id) {}
    Symbol() = delete;
    Symbol (const Symbol&) = default;
    Symbol& operator= (const Symbol&) = default;
    Symbol (Symbol&&) = default;
    Symbol& operator= (Symbol&&) = default;
    ~Symbol() = default;

    uint64_t operator() (In_It &start, const In_It end)
    {
        if (_dec == nullptr || esi() >= _dec->symbols (block()))
            return 0;
        #ifdef RQ_HEADER_ONLY
        return _dec->decode_symbol (start, end, esi(), block());
        #else
        void **_from = reinterpret_cast<void**> (&start);
        void *_to = reinterpret_cast<void*> (end);
        uint64_t res = _dec->decode_symbol (_from, _to, esi(), block());
        Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
        start = *tmp;
        return res;
        #endif
    }
    uint32_t id() const
        { return RaptorQ__v1::Impl::Endian::h_to_b<uint32_t> (_id); }

    uint8_t block() const
        { return static_cast<uint8_t> (_id >> 24); }

    uint32_t esi() const
    {
        constexpr uint32_t rev_mask = ~(static_cast<uint32_t> (0xFF) << 24);
        return _id & rev_mask;
    }
private:
    RQ_DEC_T *const _dec;
    // BIG NOTE: HOST endianness, remember to report in big-endian!
    const uint32_t _id;
};

template <typename In_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<In_It, Fwd_It>>
{
public:
    Symbol_Iterator (RQ_DEC_T *const dec, const uint32_t id)
        : _dec (dec), _id (id) {}
    Symbol_Iterator() = delete;
    Symbol_Iterator (const Symbol_Iterator&) = default;
    Symbol_Iterator& operator= (const Symbol_Iterator&) = default;
    Symbol_Iterator (Symbol_Iterator&&) = default;
    Symbol_Iterator& operator= (Symbol_Iterator&&) = default;
    ~Symbol_Iterator() = default;

    Symbol<In_It, Fwd_It> operator*()
        { return Symbol<In_It, Fwd_It> (_dec, _id); }
    Symbol_Iterator<In_It, Fwd_It>& operator++()
    {
        ++_id;
        return *this;
    }
    Symbol_Iterator<In_It, Fwd_It> operator++ (const int i) const
    {
        return Symbol_Iterator<In_It, Fwd_It> (_dec, _id +
                                                    static_cast<uint32_t> (i));
    }
    bool operator== (const Symbol_Iterator<In_It, Fwd_It> &it) const
        { return it._id == _id && it._dec == _dec; }
    bool operator!= (const Symbol_Iterator<In_It, Fwd_It> &it) const
        { return it._id != _id && it._dec == _dec; }
private:
    RQ_DEC_T *const _dec;
    // BIG NOTE: HOST endianness, remember to report in big-endian!
    uint32_t _id;
};


template <typename In_It, typename Fwd_It>
class RAPTORQ_API Block
{
public:
    Block (RQ_DEC_T *const dec, const uint8_t block)
        : _dec (dec), _block (block) {}
    Block() = delete;
    Block (const Block&) = default;
    Block& operator= (const Block&) = default;
    Block (Block&&) = default;
    Block& operator= (Block&&) = default;
    ~Block() = default;

    Symbol_Iterator<In_It, Fwd_It> begin() const
    {
        if (_dec == nullptr)
            return Symbol_Iterator<In_It, Fwd_It> (nullptr, 0);
        const uint32_t host_endian_id = static_cast<uint32_t> (_block) << 24;
        return Symbol_Iterator<In_It, Fwd_It> (_dec, host_endian_id);
    }
    Symbol_Iterator<In_It, Fwd_It> end() const
    {
        if (_dec == nullptr)
            return Symbol_Iterator<In_It, Fwd_It> (nullptr, 0);
        const uint32_t host_endian_id = ((static_cast<uint32_t> (_block) << 24)
                                                    + _dec->symbols(_block));
        return Symbol_Iterator<In_It, Fwd_It> (_dec, host_endian_id);
    }
    uint8_t id() const
        { return _block; }

    uint16_t symbols() const
    {
        if (_dec == nullptr)
            return 0;
        return _dec->symbols (_block);
    }

    Block_Size extended_symbols() const
    {
        if (_dec == nullptr)
            return static_cast<Block_Size> (0);
        return _dec->extended_symbols (_block);
    }


    uint32_t block_size() const
    {
        if (_dec == nullptr)
            return 0;
        return _dec->block_size (_block);
    }

private:
    RQ_DEC_T *const _dec;
    const uint8_t _block;
};

template <typename In_It, typename Fwd_It>
class RAPTORQ_API Block_Iterator :
        public std::iterator<std::input_iterator_tag, Block<In_It, Fwd_It>>
{
public:
    Block_Iterator (RQ_DEC_T *const dec, const uint8_t block)
        : _dec (dec), _block (block) {}
    Block_Iterator() = delete;
    Block_Iterator (const Block_Iterator&) = default;
    Block_Iterator& operator= (const Block_Iterator&) = default;
    Block_Iterator (Block_Iterator&&) = default;
    Block_Iterator& operator= (Block_Iterator&&) = default;
    ~Block_Iterator() = default;

    Block<In_It, Fwd_It> operator*()
        { return Block<In_It, Fwd_It> (_dec, _block); }
    Block_Iterator<In_It, Fwd_It>& operator++()
    {
        ++_block;
        return *this;
    }
    Block_Iterator<In_It, Fwd_It> operator++ (const int i) const
    {
        return Block_Iterator<In_It, Fwd_It> (_dec, _block +
                                                    static_cast<uint8_t> (i));
    }
    bool operator== (const Block_Iterator<In_It, Fwd_It> &it) const
        { return it._block == _block && it._dec == _dec; }
    bool operator!= (const Block_Iterator<In_It, Fwd_It> &it) const
        { return it._block != _block && it._dec == _dec; }
private:
    RQ_DEC_T *const _dec;
    uint8_t _block;
};

} // namespace Decoder


} // namespace It
} // namespace Rfc6330__v1
