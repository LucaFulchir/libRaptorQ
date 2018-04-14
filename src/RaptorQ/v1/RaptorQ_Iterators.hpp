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

#include "RaptorQ/v1/common.hpp"
#include <iterator>

// NOTE: this file is included from RaptorQ.hpp and CPP_RAW_API.hpp

// Yes, another template parameter would keep us away from macros.
// But I prefer to kep the parameters the same as for the classes.
// So we keep the macros. (NOTE: this header requires C++98 compatibility)
#ifdef RQ_HEADER_ONLY
    #include "RaptorQ/v1/RaptorQ.hpp"
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
    #include "RaptorQ/v1/wrapper/CPP_RAW_API_void.hpp"
    #define RQ_ENC_T Impl::Encoder_void
    #define RQ_DEC_T Impl::Decoder_void
#endif

namespace RaptorQ__v1 {

namespace Impl {
template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_LOCAL Encoder;

template <typename In_It, typename Fwd_It>
class RAPTORQ_LOCAL Decoder;
} // namespace Impl


namespace It {
namespace Encoder {


template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol
{
public:
    Symbol (RQ_ENC_T *enc, const uint32_t esi)
        : _enc (enc), _esi (esi) {}
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
        return _enc->encode (start, end, _esi);
        #else
        void **_from = reinterpret_cast<void**> (&start);
        void *_to = reinterpret_cast<void*> (end);
        uint64_t res = _enc->encode (_from, _to, _esi);
        Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
        start = *tmp;
        return res;
        #endif
    }
    uint32_t id() const
        { return _esi; }
private:
    RQ_ENC_T *const _enc;
    const uint32_t _esi;
};


template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<Rnd_It, Fwd_It>>
{
public:
    Symbol_Iterator (RQ_ENC_T *enc, const uint32_t esi)
        : _enc (enc), _esi (esi) {}
    Symbol_Iterator() = delete;
    Symbol_Iterator (const Symbol_Iterator&) = default;
    Symbol_Iterator& operator= (const Symbol_Iterator&) = default;
    Symbol_Iterator (Symbol_Iterator&&) = default;
    Symbol_Iterator& operator= (Symbol_Iterator&&) = default;
    ~Symbol_Iterator() = default;

    Symbol<Rnd_It, Fwd_It> operator*()
        { return Symbol<Rnd_It, Fwd_It> (_enc, _esi); }
    Symbol_Iterator<Rnd_It, Fwd_It>& operator++()
    {
        ++_esi;
        return *this;
    }
    Symbol_Iterator operator++ (const int i) const
    {
        return Symbol_Iterator<Rnd_It, Fwd_It>(_enc,
                                            _esi + static_cast<uint32_t> (i));
    }
    bool operator== (const Symbol_Iterator<Rnd_It, Fwd_It> &it) const
        { return it._esi == _esi; }
    bool operator!= (const Symbol_Iterator<Rnd_It, Fwd_It> &it) const
        { return it._esi != _esi; }
private:
    RQ_ENC_T *const _enc;
    uint32_t _esi;
};
} // namespace Encoder




namespace Decoder {
template <typename In_It, typename Fwd_It>
class RAPTORQ_API Symbol
{
public:
    Symbol (RQ_DEC_T *dec, const uint16_t esi)
        : _dec (dec), _esi (esi) {}
    Symbol() = delete;
    Symbol (const Symbol&) = default;
    Symbol& operator= (const Symbol&) = default;
    Symbol (Symbol&&) = default;
    Symbol& operator= (Symbol&&) = default;
    ~Symbol() = default;

    Error operator() (Fwd_It &start, const Fwd_It end)
    {
        if (_dec == nullptr)
            return Error::INITIALIZATION;

        #ifdef RQ_HEADER_ONLY
        return _dec->encode (start, end, _esi);
        #else
        void **_from = reinterpret_cast<void**> (&start);
        void *_to = reinterpret_cast<void*> (end);
        Error err = _dec->decode_symbol (_from, _to, _esi);
        Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
        start = *tmp;
        return err;
        #endif
    }
    uint16_t id() const
        { return _esi; }
private:
    RQ_DEC_T *const _dec;
    const uint16_t _esi;
};

template <typename In_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<In_It, Fwd_It>>
{
public:
    Symbol_Iterator (RQ_DEC_T *dec, const uint16_t esi)
        : _dec (dec), _esi (esi) {}
    Symbol_Iterator() = delete;
    Symbol_Iterator (const Symbol_Iterator&) = default;
    Symbol_Iterator& operator= (const Symbol_Iterator&) = default;
    Symbol_Iterator (Symbol_Iterator&&) = default;
    Symbol_Iterator& operator= (Symbol_Iterator&&) = default;
    ~Symbol_Iterator() = default;

    Symbol<In_It, Fwd_It> operator*()
        { return Symbol<In_It, Fwd_It> (_dec, _esi); }
    Symbol_Iterator<In_It, Fwd_It>& operator++()
    {
        ++_esi;
        return *this;
    }
    Symbol_Iterator operator++ (const int i) const
    {
        return Symbol_Iterator<In_It, Fwd_It>(_dec,
                                            _esi + static_cast<uint16_t> (i));
    }
    bool operator== (const Symbol_Iterator<In_It, Fwd_It> &it) const
        { return it._esi == _esi; }
    bool operator!= (const Symbol_Iterator<In_It, Fwd_It> &it) const
        { return it._esi != _esi; }
private:
    RQ_DEC_T *const _dec;
    uint16_t _esi;
};

} // namespace Decoder
} // namespace It
} // namespace RaptorQ

