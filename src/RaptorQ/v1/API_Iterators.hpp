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
#include "RaptorQ/v1/Interleaver.hpp"
#include <cmath>

namespace RFC6330__v1 {
namespace Impl {

template <typename Rnd_It, typename Fwd_It>
class Encoder;

}	// namespace Impl

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol
{
public:
	Symbol (Impl::Encoder<Rnd_It, Fwd_It> *enc, const uint32_t esi,
															const uint8_t sbn)
		: _enc (enc), _esi (esi), _sbn (sbn) {}

	uint64_t operator() (Fwd_It &start, const Fwd_It end)
	{
        if (_enc == nullptr)
            return 0;
		return _enc->encode (start, end, _esi, _sbn);
	}
	uint32_t id() const
	{
		uint32_t ret = _sbn;
		ret <<= 24;
		return ret + _esi;
	}
private:
	Impl::Encoder<Rnd_It, Fwd_It> *_enc;
	const uint32_t _esi;
	const uint8_t _sbn;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
		public std::iterator<std::input_iterator_tag, Symbol<Rnd_It, Fwd_It>>
{
public:
	Symbol_Iterator (Impl::Encoder<Rnd_It, Fwd_It> *enc, const uint32_t esi,
															const uint8_t sbn)
		: _enc (enc), _esi (esi), _sbn (sbn) {}
	Symbol<Rnd_It, Fwd_It> operator*()
	{
		return Symbol<Rnd_It, Fwd_It> (_enc, _esi, _sbn);
	}
	Symbol_Iterator<Rnd_It, Fwd_It>& operator++()
	{
		++_esi;
		return *this;
	}
	Symbol_Iterator operator++ (const int i) const
	{
		Symbol_Iterator<Rnd_It, Fwd_It> ret (_esi + i, _sbn);
		return ret;
	}
	bool operator== (const Symbol_Iterator<Rnd_It, Fwd_It> &it) const
	{
		return it._esi == _esi && it._sbn == _sbn;
	}
	bool operator!= (const Symbol_Iterator<Rnd_It, Fwd_It> &it) const
	{
		return it._esi != _esi || it._sbn != _sbn;
	}
private:
	Impl::Encoder<Rnd_It, Fwd_It> *_enc;
	uint32_t _esi;
	const uint8_t _sbn;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Block
{
public:
	Block (Impl::Encoder<Rnd_It, Fwd_It> *enc, const uint16_t symbols,
														const uint8_t sbn)
		: _enc (enc), _symbols (symbols), _sbn (sbn) {}

	Symbol_Iterator<Rnd_It, Fwd_It> begin_source() const
	{
		return Symbol_Iterator<Rnd_It, Fwd_It> (_enc, 0, _sbn);
	}
	Symbol_Iterator<Rnd_It, Fwd_It> end_source() const
	{
		return Symbol_Iterator<Rnd_It, Fwd_It> (_enc, _symbols, _sbn);
	}
	Symbol_Iterator<Rnd_It, Fwd_It> begin_repair() const
	{
		return Symbol_Iterator<Rnd_It, Fwd_It> (_enc, _symbols, _sbn);
	}
	Symbol_Iterator<Rnd_It, Fwd_It> end_repair (const uint32_t max_repair)
																		const
	{
		uint32_t max_r = max_repair;
		uint32_t max_sym = 1;
		max_sym <<= 20;	// max_sym = 2^20
		if (max_repair > std::pow (2, 20) - _symbols)
			max_r = max_sym - _symbols;
		return Symbol_Iterator<Rnd_It, Fwd_It> (_enc, _symbols + max_r,_sbn);
	}
	uint32_t max_repair() const
	{
        if (_enc == nullptr)
            return 0;
		return _enc->max_repair (_sbn);
	}
	uint16_t symbols () const
	{
        if (_enc == nullptr)
            return 0;
		return _enc->symbols (_sbn);
	}
	uint32_t block_size () const
	{
        if (_enc == nullptr)
            return 0;
		return _enc->block_size (_sbn);
	}

private:
	Impl::Encoder<Rnd_It, Fwd_It> * _enc;
	const uint16_t _symbols;
	const uint8_t _sbn;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Block_Iterator :
		public std::iterator<std::input_iterator_tag, Block<Rnd_It, Fwd_It>>
{
public:
	Block_Iterator (Impl::Encoder<Rnd_It, Fwd_It> *enc,
										const Impl::Partition part, uint8_t sbn)
		:_enc (enc), _part (part), _sbn (sbn) {}
	Block<Rnd_It, Fwd_It> operator*()
	{
		if (_sbn < _part.num (0))
			return Block<Rnd_It, Fwd_It> (_enc, _part.size (0), _sbn);
		return Block<Rnd_It, Fwd_It> (_enc, _part.size (1), _sbn);
	}
	Block_Iterator& operator++()
	{
		++_sbn;
		return *this;
	}
	Block_Iterator operator++ (const int i) const
	{
		Block_Iterator ret = *this;
		ret._sbn += i;
		return ret;
	}
	bool operator== (const Block_Iterator &it) const
	{
		return it._sbn == _sbn;
	}
	bool operator!= (const Block_Iterator &it) const
	{
		return it._sbn != _sbn;
	}
private:
	Impl::Encoder<Rnd_It, Fwd_It> *_enc;
	const Impl::Partition _part;
	uint8_t _sbn;
};

}	// namespace RFC6330__v1
