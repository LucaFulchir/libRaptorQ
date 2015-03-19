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

#ifndef RAPTORQ_HPP
#define RAPTORQ_HPP

/////////////////////
//
//	These templates are just a wrapper around the
//	functionalities offered by the RaptorQ::Impl namespace
//	So if you want to see what the algorithm looks like,
//	you are in the wrong place
//
/////////////////////

#include "Interleaver.hpp"
#include "De_Interleaver.hpp"
#include "Encoder.hpp"
#include "Decoder.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace RaptorQ {

template <typename T, typename OutputIterator>
class RAPTORQ_API Encoder;

/////////////
///
/// Just a couple of iterators, for blocks and symbols
///
/////////////

template <class OutputIterator, typename T>
class RAPTORQ_API Symbol_Iterator :
				public std::iterator<std::input_iterator_tag, std::vector<T>>
{
public:
	Symbol_Iterator (const Encoder<T, OutputIterator> *enc, const uint32_t esi,
														const uint32_t symbols,
														const uint8_t sbn)
		:_enc(enc), _esi(esi), _end (symbols), _sbn(sbn)
	{}
	uint64_t operator() (std::vector<T> &output)
	{
		// true if all good
		return _enc->encode (output.begin(), output.end(), _sbn, _esi);
	}
	uint64_t operator() (const OutputIterator start, const OutputIterator end)
	{
		// true if all good
		return _enc->encode (start, end, _sbn, _esi);
	}
	uint32_t id()
	{
		union {
			uint32_t raw;
			struct {
				uint8_t sbn;
				uint32_t esi:24;
			};
		} combined;

		combined.sbn = _sbn;
		combined.esi = _esi;

		return combined.raw;
	}
	std::vector<T> operator*()
	{
		std::vector<T> ret;
		ret.reserve (_enc->symbol_size);
		_enc->encode (ret.begin(), ret.end(), _sbn, _esi);
		return ret;
	}
	Symbol_Iterator<OutputIterator, T> operator++(const int i)
	{
		if (_esi + i <= _end) {
			return Symbol_Iterator<OutputIterator, T> (_enc, _esi + i,_sbn);
		} else {
			return Symbol_Iterator<OutputIterator, T> (_enc, _end, _sbn);
		}
	}
	Symbol_Iterator<OutputIterator, T>& operator++()
	{
		if (_esi < _end)
			++_esi;
		return *this;
	}
	bool operator!= (const Symbol_Iterator<OutputIterator, T> &s_it)
	{
		return !(this == s_it);
	}
	bool operator== (const Symbol_Iterator<OutputIterator, T> &s_it)
	{
		return _enc == s_it._enc && _esi == s_it._esi;
	}
private:
	const Encoder<T, OutputIterator> *_enc;
	uint32_t _esi;
	const uint32_t _end;
	const uint8_t _sbn;
};

template <typename OutputIterator, typename T>
class RAPTORQ_API Block_Iterator
{
public:
	Block_Iterator (const Encoder<T, OutputIterator> *enc, const uint8_t start,
															const uint8_t end)
		:_enc(enc), _sbn(start), _end(end)
	{}
	Symbol_Iterator<OutputIterator, T> begin () const
	{
		return Symbol_Iterator<OutputIterator, T> (_enc, 0, _enc->symbols(),
																		_sbn);
	}
	Symbol_Iterator<OutputIterator, T> end () const;
	Block_Iterator<OutputIterator, T> operator++(int i) const
	{
		if (i + _sbn < _end) {
			return Block_Iterator<OutputIterator, T> (_enc, _sbn + i, _end);
		} else {
			return Block_Iterator<OutputIterator, T> (_enc, _end, _end);
		}
	}
	Block_Iterator<OutputIterator, T>& operator++();
	bool operator!= (const Block_Iterator<OutputIterator, T> &e_it) const
	{
		return !(this == e_it);
	}
	bool operator== (const Block_Iterator<OutputIterator, T> &e_it) const
	{
		return _enc == e_it._enc && _sbn == e_it._sbn;
	}
private:
	const Encoder<T, OutputIterator> *_enc;
	uint8_t _sbn;
	const uint8_t _end;
};





// rfc 6330, pg 6
union OTI_Common_Data {
	uint64_t raw;
	struct {
		uint64_t size:40;
		uint8_t reserved:8;
		uint16_t symbol_size:16;
	};
};

union OTI_Scheme_Specific_Data {
	uint32_t raw;
	struct {
		uint8_t source_blocks;
		uint16_t sub_blocks;
		uint8_t	alignment;
	};
};





static const uint64_t max_data = 946270874880;

template <typename T, typename OutputIterator>
class RAPTORQ_API Encoder
{
public:
	const uint16_t _symbol_size;

	Encoder (std::shared_ptr<std::vector<T>> data,
											const uint16_t min_subsymbol_size,
											const uint16_t symbol_size,
											const size_t max_memory)
		: _data (data), _symbol_size (symbol_size),
						_min_subsymbol (min_subsymbol_size), _mem (max_memory)
	{
		static_assert(std::is_unsigned<T>::value,
					"RaptorQ::Encoder: can only be used with unsigned types");
		// max size: between 2^39 and 2^40
		if (data == nullptr || data->size() *sizeof(T) > max_data)
			return;
		interleave = std::unique_ptr<Impl::Interleaver<T>> (data.get(),
														_min_subsymbol, _mem,
																_symbol_size);
	}

	Block_Iterator<OutputIterator, T> begin () const
	{
		return Block_Iterator<OutputIterator, T> (this, 0,interleave->blocks());
	}
	Block_Iterator<OutputIterator, T> end () const
	{
		return Block_Iterator<OutputIterator, T> (this, interleave->blocks(),
														interleave->blocks());
	}
	bool operator()() const { return interleave != nullptr; }
	OTI_Common_Data OTI_Common() const;
	OTI_Scheme_Specific_Data OTI_Scheme_Specific() const;

	void precompute_all (const uint8_t threads);
	size_t precompute_max_memory ();
	uint32_t encode (OutputIterator &start, const OutputIterator end,
													uint32_t esi, uint8_t sbn);
	// id: 8-bit sbn + 24 bit esi
	uint32_t encode (OutputIterator &start, const OutputIterator end,
																uint32_t &id);
private:
	std::shared_ptr<std::vector<T>> _data;
	std::unique_ptr<Impl::Interleaver<T>> interleave = nullptr;
	std::map<uint8_t, std::shared_ptr<
									std::pair<std::mutex, Impl::Encoder<T>>
									>
						> encoders;
	const size_t _mem;
	std::mutex _mtx;
	const uint16_t _min_subsymbol;

	static void precompute (Encoder<T, OutputIterator> &obj, uint8_t *sbn);
};


template <typename T, typename InputIterator>
class RAPTORQ_API Decoder
{
public:
	// using shared pointers to avoid locking too much or
	// worrying about deleting used stuff.
	using Dec_ptr = std::shared_ptr<RaptorQ::Impl::Decoder<T>>;

	Decoder (OTI_Common_Data common, OTI_Scheme_Specific_Data scheme)
		:_symbol_size (common.symbol_size), _sub_blocks (scheme.sub_blocks),
												_blocks (scheme.source_blocks)
	{
		assert (scheme.alignment <= sizeof(T) &&
							"RaptorQ::Decoder: sizeof(T) must be <= alignment");
		if (common.size > max_data)
			return;

		const uint64_t total_symbols = static_cast<uint64_t> (ceil (
								static_cast<double> (common.size * sizeof(T)) /
										static_cast<double> (_symbol_size)));

		part = Impl::Partition (total_symbols, scheme.source_blocks);
	}

	Decoder (uint16_t symbol_size,uint16_t sub_blocks, uint8_t blocks)
		:_symbol_size (symbol_size), _sub_blocks (sub_blocks), _blocks (blocks)
	{}

	uint32_t decode (InputIterator &start, const InputIterator end);
	uint32_t decode (InputIterator &start, const InputIterator end,
													const uint8_t sbn);
	// id: 8-bit sbn + 24 bit esi
	bool add_symbol (const std::vector<T> &symbol, const uint32_t id);
	bool add_symbol (const std::vector<T> &symbol, const uint32_t esi,
															const uint8_t sbn);
	void free (const uint8_t sbn);
private:
	Impl::Partition part;
	const uint16_t _symbol_size, _sub_blocks;
	const uint8_t _blocks;
	std::map<uint8_t, Dec_ptr> decoders;
	std::mutex _mtx;
};






/////////////////
//
// Encoder
//
/////////////////

template <typename T, typename OutputIterator>
OTI_Common_Data Encoder<T, OutputIterator>::OTI_Common() const
{
	OTI_Common_Data ret;
	// first 40 bits: data length.
	ret.size = _data.size();
	// 8 bits: reserved
	ret.reserved = 0;
	// last 16 bits: symbol size
	ret.symbol_size = _symbol_size;

	return ret;
}

template <typename T, typename OutputIterator>
OTI_Scheme_Specific_Data Encoder<T, OutputIterator>::OTI_Scheme_Specific() const
{
	OTI_Scheme_Specific_Data ret;
	// 8 bit: source blocks
	ret.source_blocks = interleave->blocks();
	// 16 bit: sub-blocks number (N)
	ret.sub_blocks = interleave->sub_blocks();
	// 8 bit: alignment
	ret.alignment = sizeof(T);

	return ret;
}

template <typename T, typename OutputIterator>
size_t Encoder<T, OutputIterator>::precompute_max_memory ()
{
	// give a good estimate on the amount of memory neede for the precomputation
	// of one block;
	// this will help you understand how many concurrent precomputations
	// you want to do :)

	if (interleave == nullptr)
		return 0;

	uint16_t symbols = interleave->source_symbols(0);

	uint16_t K_idx;
	for (K_idx = 0; K_idx < Impl::K_padded.size(); ++K_idx) {
		if (symbols < Impl::K_padded[K_idx])
			break;
	}
	if (K_idx == Impl::K_padded.size())
		return 0;

	auto S_H = Impl::S_H_W[K_idx];
	uint16_t matrix_cols = Impl::K_padded[K_idx] + std::get<0> (S_H) +
															std::get<1> (S_H);

	// Rough estimate: Matrix A, matrix X (=> *2), matrix D and symbols.
	return matrix_cols * matrix_cols * 2 + _symbol_size * matrix_cols * 2;
}


template <typename T, typename OutputIterator>
void Encoder<T, OutputIterator>::precompute (Encoder<T, OutputIterator> &obj,
																uint8_t *sbn)
{
	// call this from a thread, precomput all block symbols
	while (*sbn < obj.interleave->blocks()) {
		obj._mtx.lock();
		auto it = obj.encoders.find (*sbn);
		if (it == obj.encoders.end()) {
			bool success;
			std::tie (it, success) = obj.encoders.insert ({*sbn,
									Impl::Encoder<T> (obj.interleave, *sbn)});
		}
		auto enc_ptr = it->second;
		bool locked = enc_ptr->first.try_lock();
		++(*sbn);
		obj._mtx.unlock();
		if (locked) {	// if not locked, someone else is already waiting
						// on this. so don't do the same work twice.
			enc_ptr->second.generate_symbols();
			enc_ptr->first.unlock();
		}
	}
}

template <typename T, typename OutputIterator>
void Encoder<T, OutputIterator>::precompute_all (const uint8_t threads)
{
	// precompute all intermediate symbols, do it with more threads.
	if (interleave == nullptr)
		return;
	std::vector<std::thread> t;
	uint8_t spawned = threads - 1;
	if (spawned == 0)
		spawned = std::thread::hardware_concurrency();

	if (spawned > 0)
		t.reserve (spawned);
	uint8_t sbn = 0;

	// spawn n-1 threads
	for (uint8_t id = 0; id < spawned; ++id)
		t.push_back (precompute, &sbn);

	// do the work by ourselves
	precompute (&sbn);

	// join other threads
	for (uint8_t id = 0; id < spawned; ++id)
		t[id].join();
}

template <typename T, typename OutputIterator>
uint32_t Encoder<T, OutputIterator>::encode (OutputIterator &start,
										const OutputIterator end, uint32_t &id)
{
	union {
		uint32_t raw;
		struct {
			uint8_t sbn;
			uint32_t esi:24;
		};
	} extracted;

	extracted.raw = id;

	return encode (start, end, extracted.esi, extracted.sbn);
}

template <typename T, typename OutputIterator>
uint32_t Encoder<T, OutputIterator>::encode (OutputIterator &start,
							const OutputIterator end, uint32_t esi, uint8_t sbn)
{
	_mtx.lock();
	auto it = encoders.find (sbn);
	if (it == encoders.end()) {
		bool success;
		std::tie (it, success) = encoders.insert ({sbn,
										Impl::Encoder<T> (interleave, sbn)});
	}
	auto enc_ptr = it->second;
	_mtx.unlock();
	enc_ptr->first.lock();
	enc_ptr->second.generate_symbols();
	enc_ptr->first.unlock();


}



/////////////////
//
// Decoder
//
/////////////////

template <typename T, typename InputIterator>
void Decoder<T, InputIterator>::free (const uint8_t sbn)
{
	_mtx.lock();
	auto it = decoders.find(sbn);
	if (it != decoders.end())
		decoders.erase(it);
	_mtx.unlock();
}

template <typename T, typename InputIterator>
bool Decoder<T, InputIterator>::add_symbol (const std::vector<T> &symbol,
															const uint32_t id)
{
	union extract {
		uint32_t raw;
		struct {
			uint8_t sbn;
			uint32_t esi:24;
		};
	} extracted;

	extracted.raw = id;

	return add_symbol (symbol, extracted.esi, extracted.sbn);
}

template <typename T, typename InputIterator>
bool Decoder<T, InputIterator>::add_symbol (const std::vector<T> &symbol,
															const uint32_t esi,
															const uint8_t sbn)
{
	if (sbn >= _blocks)
		return false;
	_mtx.lock();
	auto it = decoders.find (sbn);
	if (it == decoders.end()) {
		const uint16_t symbols = sbn < part.num (0) ?
													part.size(0) : part.size(1);
		decoders.insert ({sbn, std::make_shared<Impl::Decoder<T>> (
													symbols, _symbol_size)});
		it = decoders.find (sbn);
	}
	auto dec = it->second;
	_mtx.unlock();

	return dec->add_symbol (esi, symbol);
}

template <typename T, typename InputIterator>
uint32_t Decoder<T, InputIterator>::decode (InputIterator &start,
														const InputIterator end)
{
	// TODO: incomplete decoding

	bool missing = false;
	for (uint8_t sbn = 0; sbn < _blocks; ++sbn) {
		_mtx.lock();
		auto it = decoders.find (sbn);
		if (it == decoders.end()) {
			missing = true;
			continue;
		}
		auto dec = it->second;
		_mtx.unlock();

		if (!dec->decode())
			return 0;
	}
	if (missing)
		return 0;
	uint32_t written = 0;
	for (uint8_t sbn = 0; sbn < _blocks; ++sbn) {
		_mtx.lock();
		auto it = decoders.find (sbn);
		if (it == decoders.end())
			return written;
		auto dec = it->second;
		_mtx.unlock();
		Impl::De_Interleaver<T, InputIterator> de_interleaving (
											dec->get_symbols(), _sub_blocks);
		written += de_interleaving (start, end);
	}
	return written;
}

template <typename T, typename InputIterator>
uint32_t Decoder<T, InputIterator>::decode (InputIterator &start,
									const InputIterator end, const uint8_t sbn)
{
	if (sbn >= _blocks)
		return 0;

	_mtx.lock();
	auto it = decoders.find (sbn);
	if (it == decoders.end()) {
		_mtx.unlock();
		return 0;
	}
	auto dec = it->second;
	_mtx.unlock();

	if (!dec->decode())
		return 0;

	Impl::De_Interleaver<T, InputIterator> de_interleaving (dec->get_symbols(),
																_sub_blocks);
	return de_interleaving (start, end);
}


}	//RaptorQ

#endif

