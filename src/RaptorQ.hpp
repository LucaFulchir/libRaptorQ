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
#include <map>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

namespace RaptorQ {

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Encoder;

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol
{
public:
	Symbol (Encoder<Rnd_It, Fwd_It> *enc, const uint32_t esi, const uint8_t sbn)
		: _enc (enc), _esi (esi), _sbn (sbn) {}

	uint64_t operator() (Fwd_It &start, const Fwd_It end)
	{
		return _enc->encode (start, end, _esi, _sbn);
	}
	uint32_t id() const
	{
		uint32_t ret = _sbn;
		ret <<= 24;
		return ret + _esi;
	}
private:
	Encoder<Rnd_It, Fwd_It> *_enc;
	const uint32_t _esi;
	const uint8_t _sbn;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Symbol_Iterator :
		public std::iterator<std::input_iterator_tag, Symbol<Rnd_It, Fwd_It>>
{
public:
	Symbol_Iterator (Encoder<Rnd_It, Fwd_It> *enc, const uint32_t esi,
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
	Encoder<Rnd_It, Fwd_It> *_enc;
	uint32_t _esi;
	const uint8_t _sbn;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Block
{
public:
	Block (Encoder<Rnd_It, Fwd_It> *enc, const uint16_t symbols,
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
		return _enc->max_repair (_sbn);
	}
	uint16_t symbols () const
	{
		return _enc->symbols (_sbn);
	}
	uint32_t block_size () const
	{
		return _enc->block_size (_sbn);
	}

private:
	Encoder<Rnd_It, Fwd_It> * _enc;
	const uint16_t _symbols;
	const uint8_t _sbn;
};

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Block_Iterator :
		public std::iterator<std::input_iterator_tag, Block<Rnd_It, Fwd_It>>
{
public:
	Block_Iterator (Encoder<Rnd_It, Fwd_It> *enc, const Impl::Partition part,
																uint8_t sbn)
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
	Encoder<Rnd_It, Fwd_It> *_enc;
	const Impl::Partition _part;
	uint8_t _sbn;
};


static const uint64_t max_data = 946270874880;	// ~881 GB

typedef uint64_t OTI_Common_Data;
typedef uint32_t OTI_Scheme_Specific_Data;

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Encoder
{
public:

	~Encoder();
	Encoder (const Rnd_It data_from, const Rnd_It data_to,
											const uint16_t min_subsymbol_size,
											const uint16_t symbol_size,
											const size_t max_memory)
		: _mem (max_memory), _data_from (data_from), _data_to (data_to),
											_symbol_size (symbol_size),
											_min_subsymbol (min_subsymbol_size)
	{
		IS_RANDOM(Rnd_It, "RaptorQ::Encoder");
		IS_FORWARD(Fwd_It, "RaptorQ::Encoder");
		auto _alignment = sizeof(typename
									std::iterator_traits<Rnd_It>::value_type);
		UNUSED(_alignment);	// used only for asserts
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
		assert((_symbol_size % min_subsymbol_size == 0) &&
					"RaptorQ: symbol size must be multiple of subsymbol size");
		// max size: ~881 GB
		if (static_cast<uint64_t> (data_to - data_from) *
					sizeof(typename std::iterator_traits<Rnd_It>::value_type)
																> max_data) {
			return;
		}
		interleave = std::unique_ptr<Impl::Interleaver<Rnd_It>> (
								new Impl::Interleaver<Rnd_It> (_data_from,
														_data_to,
														_min_subsymbol, _mem,
														_symbol_size));
		if (!(*interleave))
			interleave = nullptr;
	}

	Block_Iterator<Rnd_It, Fwd_It> begin ()
	{
		return Block_Iterator<Rnd_It, Fwd_It> (this,
												interleave->get_partition(), 0);
	}
	const Block_Iterator<Rnd_It, Fwd_It> end ()
	{
		auto part = interleave->get_partition();
		return Block_Iterator<Rnd_It, Fwd_It> (this, part,
							static_cast<uint8_t> (part.num(0) + part.num(1)));
	}

	operator bool() const { return interleave != nullptr; }
	OTI_Common_Data OTI_Common() const;
	OTI_Scheme_Specific_Data OTI_Scheme_Specific() const;

	void precompute (const uint8_t threads, const bool background);
	size_t precompute_max_memory ();
	uint64_t encode (Fwd_It &output, const Fwd_It end, const uint32_t esi,
															const uint8_t sbn);
	// id: 8-bit sbn + 24 bit esi
	uint64_t encode (Fwd_It &output, const Fwd_It end, const uint32_t &id);
	void free (const uint8_t sbn);
	uint8_t blocks() const;
	uint32_t block_size (const uint8_t sbn) const;
	uint16_t symbol_size() const;
	uint16_t symbols (const uint8_t sbn) const;
	uint32_t max_repair (const uint8_t sbn) const;
private:
	class RAPTORQ_LOCAL Locked_Encoder
	{
	public:
		Locked_Encoder (const Impl::Interleaver<Rnd_It> &symbols,
															const uint8_t SBN)
			:_enc (symbols, SBN)
		{}
		std::mutex _mtx;
		Impl::Encoder<Rnd_It, Fwd_It> _enc;
	};
	std::vector<std::thread> background_work;
	std::unique_ptr<Impl::Interleaver<Rnd_It>> interleave = nullptr;
	std::map<uint8_t, std::shared_ptr<Locked_Encoder>> encoders;
	std::mutex _mtx;
	const size_t _mem;
	const Rnd_It _data_from, _data_to;
	const uint16_t _symbol_size;
	const uint16_t _min_subsymbol;

	static void precompute_block_all (Encoder<Rnd_It, Fwd_It> *obj,
														const uint8_t threads);
	static void precompute_thread (Encoder<Rnd_It, Fwd_It> *obj, uint8_t *sbn,
													const uint8_t single_sbn);
};


template <typename In_It, typename Fwd_It>
class RAPTORQ_API Decoder
{
public:
	// using shared pointers to avoid locking too much or
	// worrying about deleting used stuff.
	using Dec_ptr = std::shared_ptr<RaptorQ::Impl::Decoder<In_It>>;

	// rfc 6330, pg 6
	// easy explanation for OTI_* comes next.
	// we do NOT use bitfields as compilators are not actually forced to put
	// them in any particular order. meaning tey're useless.
	//
	//union OTI_Common_Data {
	//	uint64_t raw;
	//	struct {
	//		uint64_t size:40;
	//		uint8_t reserved:8;
	//		uint16_t symbol_size:16;
	//	};
	//};

	//union OTI_Scheme_Specific_Data {
	//	uint32_t raw;
	//	struct {
	//		uint8_t source_blocks;
	//		uint16_t sub_blocks;
	//		uint8_t	alignment;
	//	};
	//};
	Decoder (const OTI_Common_Data common,const OTI_Scheme_Specific_Data scheme)
	{
		IS_INPUT(In_It, "RaptorQ::Decoder");
		IS_FORWARD(Fwd_It, "RaptorQ::Decoder");

		// see the above commented bitfields for quick reference
		_symbol_size = static_cast<uint16_t> (common);
		uint16_t tot_sub_blocks = static_cast<uint16_t> (scheme >> 8);
		_alignment = static_cast<uint8_t> (scheme);
		_sub_blocks = Impl::Partition (_symbol_size /
												static_cast<uint8_t> (scheme),
																tot_sub_blocks);
		_blocks = static_cast<uint8_t> (scheme >> 24);
		_size = common >> 24;
		//	(common >> 24) == total file size
		if (_size > max_data)
			return;

		const uint64_t total_symbols = static_cast<uint64_t> (ceil (
								_size / static_cast<double> (_symbol_size)));

		part = Impl::Partition (total_symbols, static_cast<uint8_t> (_blocks));
	}

	Decoder (const uint64_t size, const uint16_t symbol_size,
													const uint16_t sub_blocks,
													const uint8_t blocks,
													const uint8_t alignment)
		:_size (size), _symbol_size (symbol_size), _blocks (blocks),
														_alignment(alignment)
	{
		if (_size > max_data)
			return;

		const uint64_t total_symbols = static_cast<uint64_t> (ceil (
								_size / static_cast<double> (_symbol_size)));
		_sub_blocks = Impl::Partition (_symbol_size / _alignment, sub_blocks);

		part = Impl::Partition (total_symbols, static_cast<uint8_t> (_blocks));
	}

	uint64_t decode (Fwd_It &start, const Fwd_It end);
	uint64_t decode (Fwd_It &start, const Fwd_It end, const uint8_t sbn);
	// id: 8-bit sbn + 24 bit esi
	bool add_symbol (In_It &start, const In_It end, const uint32_t id);
	bool add_symbol (In_It &start, const In_It end, const uint32_t esi,
															const uint8_t sbn);
	void free (const uint8_t sbn);
	uint64_t bytes() const;
	uint8_t blocks() const;
	uint32_t block_size (const uint8_t sbn) const;
	uint16_t symbol_size() const;
	uint16_t symbols (const uint8_t sbn) const;
private:
	uint64_t _size;
	Impl::Partition part, _sub_blocks;
	uint16_t _symbol_size;
	uint8_t _blocks, _alignment;
	std::map<uint8_t, Dec_ptr> decoders;
	std::mutex _mtx;
};





/////////////////
//
// Encoder
//
/////////////////
template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::~Encoder ()
{
	for (auto &thread: background_work)
		thread.join();
}

template <typename Rnd_It, typename Fwd_It>
OTI_Common_Data Encoder<Rnd_It, Fwd_It>::OTI_Common() const
{
	if (interleave == nullptr)
		return 0;
	OTI_Common_Data ret;
	// first 40 bits: data length.
	ret = (static_cast<uint64_t> (_data_to - _data_from) *
			sizeof(typename std::iterator_traits<Rnd_It>::value_type)) << 24;
	// 8 bits: reserved
	// last 16 bits: symbol size
	ret += _symbol_size;

	return ret;
}

template <typename Rnd_It, typename Fwd_It>
OTI_Scheme_Specific_Data Encoder<Rnd_It, Fwd_It>::OTI_Scheme_Specific() const
{
	if (interleave == nullptr)
		return 0;
	OTI_Scheme_Specific_Data ret;
	// 8 bit: source blocks
	ret = static_cast<uint32_t> (interleave->blocks()) << 24;
	// 16 bit: sub-blocks number (N)
	ret += static_cast<uint32_t> (interleave->sub_blocks()) << 8;
	// 8 bit: alignment
	ret += sizeof(typename std::iterator_traits<Rnd_It>::value_type);

	return ret;
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::precompute_max_memory ()
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

	// Rough memory estimate: Matrix A, matrix X (=> *2) and matrix D.
	return matrix_cols * matrix_cols * 2 + _symbol_size * matrix_cols;
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::precompute_thread (Encoder<Rnd_It, Fwd_It> *obj,
													uint8_t *sbn,
													const uint8_t single_sbn)
{
	if (obj->interleave == nullptr)
		return;
	// if "sbn" pointer is NOT nullptr, than we are a thread from
	// from a precompute_block_all. This means that we need to update
	// the value of sbn as soon as we get our work.
	//
	// if sbn == nullptr, then we have been called to work on a single
	// sbn, and not from "precompute_block_all".
	// This means we work on "single_sbn", and do not touch "sbn"

	uint8_t *sbn_ptr = sbn;
	if (sbn_ptr == nullptr)
		sbn_ptr = const_cast<uint8_t*> (&single_sbn);
	// call this from a thread, precomput all block symbols
	while (*sbn_ptr < obj->interleave->blocks()) {
		obj->_mtx.lock();
		if (*sbn_ptr >= obj->interleave->blocks()) {
			obj->_mtx.unlock();
			return;
		}
		auto it = obj->encoders.find (*sbn_ptr);
		if (it == obj->encoders.end()) {
			bool success;
			std::tie (it, success) = obj->encoders.insert ({*sbn_ptr,
					std::make_shared<Locked_Encoder> (*obj->interleave,*sbn_ptr)
														});
		}
		auto enc_ptr = it->second;
		bool locked = enc_ptr->_mtx.try_lock();
		if (sbn != nullptr)
			++(*sbn);
		obj->_mtx.unlock();
		if (locked) {	// if not locked, someone else is already waiting
						// on this. so don't do the same work twice.
			enc_ptr->_enc.generate_symbols();
			enc_ptr->_mtx.unlock();
		}
		if (sbn == nullptr)
			return;
	}
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::precompute (const uint8_t threads,
														const bool background)
{
	if (interleave == nullptr)
		return;
	if (background) {
		background_work.emplace_back (precompute_block_all, this, threads);
	} else {
		return precompute_block_all (this, threads);
	}
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::precompute_block_all (
												Encoder<Rnd_It, Fwd_It> *obj,
												const uint8_t threads)
{
	// precompute all intermediate symbols, do it with more threads.
	if (obj->interleave == nullptr)
		return;
	std::vector<std::thread> t;
	int32_t spawned = threads - 1;
	if (spawned <= -1)
		spawned = static_cast<int32_t> (std::thread::hardware_concurrency());

	if (spawned > 0)
		t.reserve (static_cast<size_t> (spawned));
	uint8_t sbn = 0;

	// spawn n-1 threads
	for (int8_t id = 0; id < spawned; ++id)
		t.emplace_back (precompute_thread, obj, &sbn, 0);

	// do the work ourselves
	precompute_thread (obj, &sbn, 0);

	// join other threads
	for (uint8_t id = 0; id < spawned; ++id)
		t[id].join();
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
															const uint32_t &id)
{
	const uint32_t mask_8 = static_cast<uint32_t> (std::pow (2, 8)) - 1;
	const uint32_t mask = ~(mask_8 << 24);

	return encode (output, end, id & mask, static_cast<uint8_t> (id & mask_8));
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
															const uint32_t esi,
															const uint8_t sbn)
{
	if (interleave == nullptr)
		return 0;
	if (sbn >= interleave->blocks())
		return 0;
	_mtx.lock();
	auto it = encoders.find (sbn);
	if (it == encoders.end()) {
		bool success;
		std::tie (it, success) = encoders.emplace (sbn,
						std::make_shared<Locked_Encoder> (*interleave, sbn));
		background_work.emplace_back (precompute_thread, this, nullptr, sbn);
	}
	auto enc_ptr = it->second;
	_mtx.unlock();
	if (esi >= interleave->source_symbols (sbn)) {
		// make sure we generated the intermediate symbols
		enc_ptr->_mtx.lock();
		enc_ptr->_enc.generate_symbols();
		enc_ptr->_mtx.unlock();
	}

	return enc_ptr->_enc.Enc (esi, output, end);
}


template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::free (const uint8_t sbn)
{
	_mtx.lock();
	auto it = encoders.find (sbn);
	if (it != encoders.end())
		encoders.erase (it);
	_mtx.unlock();
}

template <typename Rnd_It, typename Fwd_It>
uint8_t Encoder<Rnd_It, Fwd_It>::blocks() const
{
	if (interleave == nullptr)
		return 0;
	return interleave->blocks();
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::block_size (const uint8_t sbn) const
{
	if (interleave == nullptr)
		return 0;
	return interleave->source_symbols (sbn) * interleave->symbol_size();
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbol_size() const
{
	if (interleave == nullptr)
		return 0;
	return interleave->symbol_size();
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbols (const uint8_t sbn) const
{
	if (interleave == nullptr)
		return 0;
	return interleave->source_symbols (sbn);
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::max_repair (const uint8_t sbn) const
{
	if (interleave == nullptr)
		return 0;
	return static_cast<uint32_t> (std::pow (2, 20)) -
											interleave->source_symbols (sbn);
}
/////////////////
//
// Decoder
//
/////////////////

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::free (const uint8_t sbn)
{
	_mtx.lock();
	auto it = decoders.find(sbn);
	if (it != decoders.end())
		decoders.erase(it);
	_mtx.unlock();
}

template <typename In_It, typename Fwd_It>
bool Decoder<In_It, Fwd_It>::add_symbol (In_It &start, const In_It end,
															const uint32_t id)
{
	uint32_t esi = (id << 8 ) >> 8;
	uint8_t sbn = id >> 24;

	return add_symbol (start, end, esi, sbn);
}

template <typename In_It, typename Fwd_It>
bool Decoder<In_It, Fwd_It>::add_symbol (In_It &start, const In_It end,
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
		decoders.insert ({sbn, std::make_shared<Impl::Decoder<In_It>> (
													symbols, _symbol_size)});
		it = decoders.find (sbn);
	}
	auto dec = it->second;
	_mtx.unlock();

	return dec->add_symbol (start, end, esi);
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::decode (Fwd_It &start, const Fwd_It end)
{
	for (uint8_t sbn = 0; sbn < _blocks; ++sbn) {
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
	}
	// everything has been decoded
	// now try to decode all blocks. Some tricks are needed since the output
	// alignment and the block size might not be the same.
	uint64_t written = 0;
	uint8_t skip = 0;
	for (uint8_t sbn = 0; sbn < _blocks; ++sbn) {
		_mtx.lock();
		auto it = decoders.find (sbn);
		if (it == decoders.end())
			return written;
		auto dec = it->second;
		_mtx.unlock();
		Impl::De_Interleaver<Fwd_It> de_interleaving (dec->get_symbols(),
													_sub_blocks, _alignment);
		auto tmp_start = start;
		uint64_t tmp_written = de_interleaving (tmp_start, end, skip);
		if (tmp_written == 0)
			return written;
		written += tmp_written;
		// did we end up in the middle of a Fwd_It now?
		skip = block_size (sbn) %
					sizeof(typename std::iterator_traits<Fwd_It>::value_type);
		// if we ended decoding in the middle of a Fwd_It, do not advance
		// start too much, or we will end up having additional zeros.
		if (skip == 0) {
			start = tmp_start;
		} else {
			// tmp_written > 0 due to earlier return
			// moreover, RaptorQ handles at most 881GB per rfc, so
			// casting uint64 to int64 is safe
			// we can not do "--start" since it's a forward iterator
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wshorten-64-to-32"
			start += static_cast<int64_t> (tmp_written - 1);
            #pragma clang diagnostic pop
		}
	}
	return written;
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::decode (Fwd_It &start, const Fwd_It end,
															const uint8_t sbn)
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

	Impl::De_Interleaver<Fwd_It> de_interleaving (dec->get_symbols(),
													_sub_blocks, _alignment);
	return de_interleaving (start, end);
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::bytes() const
{
	return _size;
}

template <typename In_It, typename Fwd_It>
uint8_t Decoder<In_It, Fwd_It>::blocks() const
{
	return static_cast<uint8_t> (part.num (0) + part.num (1));
}

template <typename In_It, typename Fwd_It>
uint32_t Decoder<In_It, Fwd_It>::block_size (const uint8_t sbn) const
{
	if (sbn < part.num (0)) {
		return part.size (0) * _symbol_size;
	} else if (sbn - part.num (0) < part.num (1)) {
		return part.size (1) * _symbol_size;
	}
	return 0;
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbol_size() const
{
	return _symbol_size;
}
template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbols (const uint8_t sbn) const
{
	if (sbn < part.num (0)) {
		return part.size (0);
	} else if (sbn - part.num (0) < part.num (1)) {
		return part.size (1);
	}
	return 0;
}

}	//RaptorQ

#endif

