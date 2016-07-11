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


namespace RaptorQ__v1 {

namespace Impl {
template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_LOCAL Encoder;

template <typename In_It, typename Fwd_It>
class RAPTORQ_LOCAL Decoder;
} // namespace Impl



namespace Encoder {

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol
{
public:
    Symbol (Impl::Encoder<Rnd_It, Fwd_It> *enc, const uint32_t esi)
        : _enc (enc), _esi (esi) {}

    uint64_t operator() (Fwd_It &start, const Fwd_It end)
    {
        if (_enc == nullptr)
            return 0;
        return _enc->encode (start, end, _esi);
    }
    uint32_t id() const
    {
        return _esi;
    }
private:
    Impl::Encoder<Rnd_It, Fwd_It> *_enc;
    const uint32_t _esi;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<Rnd_It, Fwd_It>>
{
public:
    Symbol_Iterator (Impl::Encoder<Rnd_It, Fwd_It> *enc, const uint32_t esi)
        : _enc (enc), _esi (esi) {}
    Symbol<Rnd_It, Fwd_It> operator*()
    {
        return Symbol<Rnd_It, Fwd_It> (_enc, _esi);
    }
    Symbol_Iterator<Rnd_It, Fwd_It>& operator++()
    {
        ++_esi;
        return *this;
    }
    Symbol_Iterator operator++ (const int i) const
    {
        Symbol_Iterator<Rnd_It, Fwd_It> ret (_esi + i);
        return ret;
    }
    bool operator== (const Symbol_Iterator<Rnd_It, Fwd_It> &it) const
    {
        return it._esi == _esi;
    }
    bool operator!= (const Symbol_Iterator<Rnd_It, Fwd_It> &it) const
    {
        return it._esi != _esi;
    }
private:
    Impl::Encoder<Rnd_It, Fwd_It> *_enc;
    uint32_t _esi;
};
} // namespace Encoder




namespace Decoder {
template <typename In_It, typename Fwd_It>
class RAPTORQ_API Symbol
{
public:
    Symbol (Impl::Decoder<In_It, Fwd_It> *dec, const uint32_t esi)
        : _dec (dec), _esi (esi) {}

    uint64_t operator() (Fwd_It &start, const Fwd_It end)
    {
        if (_dec == nullptr)
            return 0;
        return _dec->decode_symbol (start, end, _esi);
    }
    uint32_t id() const
    {
        return _esi;
    }
private:
    Impl::Decoder<In_It, Fwd_It> *_dec;
    const uint32_t _esi;
};

template <typename In_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
        public std::iterator<std::input_iterator_tag, Symbol<In_It, Fwd_It>>
{
public:
    Symbol_Iterator (Impl::Decoder<In_It, Fwd_It> *dec, const uint32_t esi)
        : _dec (dec), _esi (esi) {}
    Symbol<In_It, Fwd_It> operator*()
    {
        return Symbol<In_It, Fwd_It> (_dec, _esi);
    }
    Symbol_Iterator<In_It, Fwd_It>& operator++()
    {
        ++_esi;
        return *this;
    }
    Symbol_Iterator operator++ (const int i) const
    {
        Symbol_Iterator<In_It, Fwd_It> ret (_esi + i);
        return ret;
    }
    bool operator== (const Symbol_Iterator<In_It, Fwd_It> &it) const
    {
        return it._esi == _esi;
    }
    bool operator!= (const Symbol_Iterator<In_It, Fwd_It> &it) const
    {
        return it._esi != _esi;
    }
private:
    Impl::Decoder<In_It, Fwd_It> *_dec;
    uint32_t _esi;
};
} // namespace Decoder

} // namespace RaptorQ

