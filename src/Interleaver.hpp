/*
 * Copyright (c) 2015, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

#ifndef RAPTORQ_INTERLEAVER_HPP
#define RAPTORQ_INTERLEAVER_HPP

#include "common.hpp"
#include "multiplication.hpp"
#include "table2.hpp"
#include <cmath>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

// force promotion to double in division
namespace {
double RAPTORQ_LOCAL div_floor (const double a, const double b);
double RAPTORQ_LOCAL div_ceil (const double a, const double b);

double div_floor (const double a, const double b)
{
	return std::floor (a / b);
}
double div_ceil (const double a, const double b)
{
	return std::ceil (a / b);
}
}

namespace RaptorQ {
namespace Impl {

void test (void);

//
// Partition: see RFC6330: each object is partitioned in
//		N1 blocks of size S1, plus N2 blocks of size S2. This class tracks it
//
class RAPTORQ_LOCAL Partition
{
public:
	Partition() = default;
	// partition something into "num1" partitions of "size1" and "num2"
	// of "size2"
	// still better than the TL, TS, NL, NL in RFC6330...
	Partition (const uint64_t obj_size, const uint16_t partitions)
	{
		uint16_t size_1, size_2, blocks_1, blocks_2;

		size_1 = static_cast<uint16_t> (div_ceil (obj_size, partitions));
		size_2 = static_cast<uint16_t> (div_floor (obj_size, partitions));
		blocks_1 = static_cast<uint16_t> (obj_size - size_2 * partitions);
		blocks_2 = partitions - blocks_1;

		if (blocks_1 == 0)
			size_1 = 0;
		part1 = {blocks_1, size_1};
		part2 = {blocks_2, size_2};
	}

	uint16_t size (const uint8_t part_number) const
	{
		assert(part_number < 2 && "partition: only two partitions exists");
		if (part_number == 0)
			return std::get<1>(part1);
		return std::get<1>(part2);
	}
	uint16_t num (const uint8_t part_number) const
	{
		assert(part_number < 2 && "partition: only two partitions exists");
		if (part_number == 0)
			return std::get<0>(part1);
		return std::get<0>(part2);
	}
	uint16_t tot (const uint8_t part_number) const
	{
		assert(part_number < 2 && "partition: only two partitions exists");
		// num * size
		if (part_number == 0)
			return std::get<0>(part1) * std::get<1>(part1);
		return std::get<0>(part2) * std::get<1>(part2);
	}
private:
	// PAIR: amount, size
	std::pair<uint16_t, uint16_t> part1, part2;
};

template <typename T>
class RAPTORQ_LOCAL Symbol_Wrap
{
public:
	Symbol_Wrap (const uint8_t *raw, const uint16_t size) : _raw (raw),
																	_size (size)
	{}

	Symbol_Wrap<T>& operator= (const Symbol_Wrap<T> &a)
	{
		assert (_raw != nullptr && "Encoded_Symbol raw == nullptr");
		for (size_t i = 0; i < _size * sizeof(T); ++i)
			_raw[i] = a._raw[i];
		return *this;
	}
	Symbol_Wrap<T>& operator+= (const Symbol_Wrap<T> &a)
	{
		assert (_raw != nullptr && "Encoded_Symbol raw == nullptr");
		for (size_t i = 0; i < _size * sizeof(T); ++i)
			_raw[i] ^= a._raw[i];
		return *this;
	}
	Symbol_Wrap<T>& operator*= (const Symbol_Wrap<T> &a)
	{
		assert (_raw != nullptr && "Encoded_Symbol raw == nullptr");
		for (size_t i = 0; i < _size * sizeof(T); ++i) {
			if (_raw[i] == 0 || a._raw[i] == 0) {
				_raw[i] = 0;
			} else {
				_raw[i] = Impl::oct_exp[Impl::oct_log[_raw[i]] +
											Impl::oct_exp[a._raw[i]]];
			}
		}
		return *this;
	}
	Symbol_Wrap<T>& operator/= (const Symbol_Wrap<T> &a)
	{
		assert (_raw != nullptr && "Encoded_Symbol raw == nullptr");
		for (size_t i = 0; i < _size * sizeof(T); ++i) {
			if (_raw[i] != 0) {
				_raw[i] = Impl::oct_exp[Impl::oct_log[_raw[i]] -
										Impl::oct_exp[a._raw[i]] + 255];
			}
		}
		return *this;
	}
private:
	const uint8_t *_raw = nullptr;
	const uint16_t _size;
};

//
// Symbol:
//		Basic unit later on. This is a block of interneaved sub-symbols.
//		see RFC 6330 for details
//		Padding is included here
//
template <typename Rnd_It>
class RAPTORQ_LOCAL Symbol_it
{
public:
	Symbol_it ();
	Symbol_it (const Rnd_It data_from, const Rnd_It data_to, const size_t start,
										const size_t end, const size_t idx,
										const Partition sub_blocks,
										const uint16_t symbol_size,
										const uint16_t symbol_id,
										const uint16_t k)
			:_data_from (data_from), _data_to (data_to), _start (start),
								_end (end), _idx(idx), _sub_blocks (sub_blocks),
								_symbol_size (symbol_size),
								_symbol_id (symbol_id), _k(k)
	{}

	constexpr Symbol_it<Rnd_It> begin() const
	{
		return Symbol_it<Rnd_It> (_data_from, _data_to, _start, _end, 0,
									_sub_blocks, _symbol_size, _symbol_id, _k);
	}
	constexpr Symbol_it<Rnd_It> end() const
	{
		return Symbol_it<Rnd_It> (_data_from, _data_to, _start, _end,
									 _sub_blocks.tot (0) + _sub_blocks.tot (1),
									_sub_blocks, _symbol_size, _symbol_id, _k);
	}
	using T = typename std::iterator_traits<Rnd_It>::value_type;
	T operator[] (const size_t pos) const
	{
		size_t i;
		if (pos < _sub_blocks.tot (0)) {
			auto sub_blk_id = pos / _sub_blocks.size (0);
			i = _start +
					sub_blk_id * _k * _sub_blocks.size (0) +// right sub block
					_symbol_id * _sub_blocks.size (0) +	// get right subsymbol
					pos % _sub_blocks.size (0);			// get right alignment
		} else {
			auto pos_part2 = pos - _sub_blocks.tot (0);
			auto sub_blk_id = pos_part2 / _sub_blocks.size (1);
			i = _start + _sub_blocks.tot (0) * _k +	// skip previous partition
					sub_blk_id * _k * _sub_blocks.size (1) +// right sub block
					_symbol_id * _sub_blocks.size (1) +	// get right subsymbol
					pos_part2 % _sub_blocks.size (1);	// get right alignment
		}
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wshorten-64-to-32"
		auto data = _data_from + static_cast<int64_t>(i);
        #pragma clang diagnostic pop
		if (data >= _data_to) {
			// Padding. remember to cast it to the same time as the iterator
			// value
			return static_cast<
						typename std::iterator_traits<Rnd_It>::value_type> (0);
		}
		return *data;
	}
	T operator* () const
	{
		return (*this)[_idx];
	}
	Symbol_it<Rnd_It> operator++ (int i) const
	{
		if (_idx + i >=  _sub_blocks.tot (0) + _sub_blocks.tot (1))
			return end();
		return Symbol_it<Rnd_It> (_data_from, _data_to, _start, _end, _idx + i,
									_sub_blocks, _symbol_size, _symbol_id, _k);
	}
	Symbol_it<Rnd_It>& operator++()
	{
		if (_idx <  _sub_blocks.tot (0) + _sub_blocks.tot (1))
			++_idx;
		return *this;
	}
	bool operator== (const Symbol_it<Rnd_It> &s) const
	{
		return _idx == s._idx;
	}
	bool operator!= (const Symbol_it<Rnd_It> &s) const
	{
		return _idx != s._idx;
	}

private:
	const Rnd_It _data_from, _data_to;
	const size_t _start, _end;
	size_t _idx;
	const Partition _sub_blocks;
	const uint16_t _symbol_size, _symbol_id, _k;
};

//
// Source_Block:
//		First unit of partitioning for the object to be transferred.
//
template <typename Rnd_It>
class RAPTORQ_LOCAL Source_Block
{
public:
	Source_Block (const Rnd_It data_from, const Rnd_It data_to,
										const size_t start,
										const size_t end, const size_t idx,
										const Partition sub_blocks,
										const uint16_t symbol_size)
			:_data_from (data_from), _data_to (data_to), _start (start),
					_end (end), _idx(idx), _sub_blocks(sub_blocks),
					_symbol_size (symbol_size),
					_symbols (
						static_cast<uint16_t> ((end - start) / symbol_size))
	{}

	constexpr Source_Block<Rnd_It> begin() const
	{
		return Source_Block (_data_from, _data_to, _start, _end, 0, _sub_blocks,
																_symbol_size);
	}
	constexpr Source_Block<Rnd_It> end() const
	{
		return Source_Block<Rnd_It> (_data_from, _data_to, _start, _end, _end,
													_sub_blocks, _symbol_size);
	}
	const Symbol_it<Rnd_It> operator[] (const uint16_t symbol_id) const
	{
		if (symbol_id <  _symbols) {
			return Symbol_it<Rnd_It> (_data_from, _data_to, _start, _end, 0,
										_sub_blocks, _symbol_size, symbol_id,
																	_symbols);
		}
		// out of range.
		return Symbol_it<Rnd_It> (_data_from, _data_to, 0, 0, 0, _sub_blocks,
															_symbol_size, 0, 0);
	}
	const Symbol_it<Rnd_It> operator* () const
	{
		return (*this)[_idx];
	}
	const Source_Block<Rnd_It> operator++ (int i) const
	{
		if (_idx + i >= _symbols)
			return end();
		return Source_Block<Rnd_It> (_data_from, _data_to, _start, _end,
										_idx + i, _sub_blocks, _symbol_size);
	}
	const Source_Block<Rnd_It>& operator++ ()
	{
		if (_idx < _symbols)
			++_idx;
		return *this;
	}
private:
	const Rnd_It _data_from, _data_to;
	const size_t _start, _end;
	size_t _idx;
	const Partition _sub_blocks;
	const uint16_t _symbol_size, _symbols;
};


//
// Interleaver
//		Take an object file, and handle the source block, sub block, sub symbol
//		and symbol division and interleaving, and padding.
//
template <typename Rnd_It>
class RAPTORQ_LOCAL Interleaver
{
public:
	operator bool() const;	// true => all ok

	Interleaver (const Rnd_It data_from, const Rnd_It data_to,
											const uint16_t min_subsymbol_size,
											const size_t max_block_decodable,
											const uint16_t symbol_syze);

	Source_Block<Rnd_It>& begin() const;
	Source_Block<Rnd_It>& end() const;
	Interleaver<Rnd_It>& operator++();
	Source_Block<Rnd_It> operator*() const;
	Source_Block<Rnd_It> operator[] (uint8_t source_block_id) const;
	Partition get_partition() const;
	uint16_t source_symbols(const uint8_t SBN) const;
	uint8_t blocks () const;
	uint16_t sub_blocks () const;
	uint16_t symbol_size() const;
protected:
private:
	const Rnd_It _data_from, _data_to;
	const uint16_t _symbol_size;
	uint16_t _sub_blocks, _source_symbols, iterator_idx = 0;
	uint8_t _alignment, _source_blocks;

	// Please everyone take a moment to tank the RFC6330 guys for
	// giving such wonderfully self-explanatory names to *everything*.
	// Same names are kept to better track the rfc
	// (SIZE, SIZE, BLOCKNUM, BLOCKNUM) for:
	Partition _source_part, _sub_part;
};

///////////////////////////////////
//
// IMPLEMENTATION OF ABOVE TEMPLATE
//
///////////////////////////////////

template <typename Rnd_It>
Interleaver<Rnd_It>::Interleaver (const Rnd_It data_from,
											const Rnd_It data_to,
											const uint16_t min_subsymbol_size,
											const size_t max_block_decodable,
											const uint16_t symbol_size)
	:_data_from (data_from), _data_to (data_to), _symbol_size (symbol_size),
		_alignment (sizeof(typename std::iterator_traits<Rnd_It>::value_type))
{
	IS_RANDOM(Rnd_It, "RaptorQ::Impl::Interleaver");
	// all parameters are in octets
	assert(_symbol_size >= _alignment &&
					"RaptorQ: symbol_size must be >= alignment");
	assert((_symbol_size % _alignment) == 0 &&
					"RaptorQ: symbol_size must be multiple of alignment");
	assert(min_subsymbol_size >= _alignment &&
					"RaptorQ: minimum subsymbol must be at least aligment");
	assert(min_subsymbol_size <= _symbol_size &&
					"RaptorQ: minimum subsymbol must be at most symbol_size");
	assert((min_subsymbol_size % _alignment) == 0 &&
				"RaptorQ: minimum subsymbol must be multiple of alignment");
	// derive number of source blocks and sub blocks. seed RFC 6330, pg 8
	std::vector<uint16_t> sizes;
	size_t iter_size =sizeof(typename std::iterator_traits<Rnd_It>::value_type);
	const double input_size = static_cast<double>(_data_to - _data_from) *
																	iter_size;
	const double Kt = div_ceil (input_size, symbol_size);
	const size_t N_max = static_cast<size_t> (div_floor (_symbol_size,
														min_subsymbol_size));

	// symbol_size must be a multiple of our alignment
	if (_symbol_size % _alignment != 0 || min_subsymbol_size < _alignment ||
									(min_subsymbol_size % _alignment) != 0 ||
											min_subsymbol_size > symbol_size) {
		// nonsense configurations. refuse to work.
		_alignment = 0;
		return;
	}

	// rfc 6330, pg 8
	size_t tmp;
	sizes.reserve (N_max);
	// find our KL(n), for each n
	for (tmp = 1; tmp <= N_max; ++tmp) {
		auto upper_bound = max_block_decodable / (_alignment *
									div_ceil (_symbol_size, _alignment * tmp));
		size_t idx;
		for (idx = 0; idx < RaptorQ::Impl::K_padded.size(); ++idx) {
			if (RaptorQ::Impl::K_padded[idx] > upper_bound)
				break;
		}
		// NOTE: tmp starts from 1, but "sizes" stores from 0.
		sizes.push_back (RaptorQ::Impl::K_padded[idx == 0 ? 0 : --idx]);
	}
	const uint64_t _test_blocks = static_cast<uint64_t> (
											div_ceil (Kt, sizes[N_max - 1]));
	if (_test_blocks > std::numeric_limits<uint8_t>::max()) {
		_alignment = 0;
		return;
	}
	_source_blocks = static_cast<uint8_t> (_test_blocks);
	tmp = static_cast<size_t> (div_ceil (Kt, _source_blocks));
	for (size_t i = 0; i < sizes.size(); ++i) {
		// rfc: ceil (Kt / Z) <= KL(n)
		if (tmp <= sizes[i]) {
			_sub_blocks = static_cast<uint16_t> (i + 1); // +1: see above note
			break;
		}
	}
	assert(div_ceil (div_ceil (input_size, _symbol_size),
													_source_blocks) <= K_max &&
						"RaptorQ: RFC: ceil(ceil(F/T)/Z must be <= K'_max");
	if (_source_blocks == 0 || _sub_blocks == 0 ||
					symbol_size < _alignment || symbol_size % _alignment != 0 ||
							div_ceil (div_ceil ( input_size, _symbol_size),
													_source_blocks) > K_max) {
		_alignment = 0;
		return;
	}
	// blocks and size for source block partitioning
	_source_part = Partition (static_cast<uint64_t> (Kt), _source_blocks);

	_source_symbols = _source_part.size(0) + _source_part.size(1);

	// blocks and size for sub-block partitioning
	_sub_part = Partition (_symbol_size / _alignment, _sub_blocks);
}

template <typename Rnd_It>
Interleaver<Rnd_It>::operator bool() const
{
	// true => all ok
	return _alignment != 0;
}

template <typename Rnd_It>
Source_Block<Rnd_It> Interleaver<Rnd_It>::operator[] (
												uint8_t source_block_id) const
{
	// now we start working with multiples of T.
	// identify the start and end of the requested block.
	uint16_t al_symbol_size = _symbol_size /
					sizeof(typename std::iterator_traits<Rnd_It>::value_type);

	if (source_block_id < _source_part.num(0)) {
		size_t sb_start = source_block_id * _source_part.size(0) *
																al_symbol_size;
		size_t sb_end = (source_block_id + 1) * _source_part.size(0) *
																al_symbol_size;

		return Source_Block<Rnd_It> (_data_from, _data_to, sb_start, sb_end, 0,
													_sub_part, al_symbol_size);
	} else if (source_block_id - _source_part.num(0) < _source_part.num(1)) {
		// start == all the previous partition
		size_t sb_start = _source_part.tot(0) * al_symbol_size +
									// plus some blocks of the new partition
									(source_block_id - _source_part.num(0)) *
										_source_part.size(1) * al_symbol_size;
		size_t sb_end =  sb_start + _source_part.size(1) * al_symbol_size;

		return Source_Block<Rnd_It> (_data_from, _data_to, sb_start, sb_end, 0,
													_sub_part, al_symbol_size);
	} else  {
		assert(false && "RaptorQ: source_block_id out of range");
		return Source_Block<Rnd_It> (_data_from, _data_to, 0, 0, 0, _sub_part,
																al_symbol_size);
	}
}

template <typename Rnd_It>
uint16_t Interleaver<Rnd_It>::symbol_size() const
{
	// return the number of alignments, to make things easier
	return _symbol_size / sizeof(
							typename std::iterator_traits<Rnd_It>::value_type);
}

template <typename Rnd_It>
Partition Interleaver<Rnd_It>::get_partition() const
{
	return _source_part;
}


template <typename Rnd_It>
uint16_t Interleaver<Rnd_It>::source_symbols (const uint8_t SBN) const
{
	if (SBN < _source_part.num (0))
		return _source_part.size (0);
	if (SBN - _source_part.num (0) < _source_part.num (1))
		return _source_part.size (1);
	return 0;
}

template <typename Rnd_It>
uint8_t Interleaver<Rnd_It>::blocks () const
{
	return static_cast<uint8_t> (_source_part.num (0) + _source_part.num (1));
}

template <typename Rnd_It>
uint16_t Interleaver<Rnd_It>::sub_blocks () const
{
	return _sub_part.num (0) + _sub_part.num (1);
}

template <typename Rnd_It>
Source_Block<Rnd_It>& Interleaver<Rnd_It>::begin() const
{
	return this[0];
}

template <typename Rnd_It>
Source_Block<Rnd_It>& Interleaver<Rnd_It>::end() const
{
	return this[_source_blocks + 1];
}

template <typename Rnd_It>
Interleaver<Rnd_It>& Interleaver<Rnd_It>::operator++()
{
	++iterator_idx;
	return *this;
}

template <typename Rnd_It>
Source_Block<Rnd_It> Interleaver<Rnd_It>::operator*() const
{
	return this[iterator_idx];
}

}	// namespace Impl
}	// namespace RaptorQ

#endif
