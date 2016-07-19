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
#include "RaptorQ/v1/RaptorQ_Iterators.hpp"
#include "RaptorQ/v1/Encoder.hpp"
#include "RaptorQ/v1/Decoder.hpp"
#include <algorithm>
#include <future>
#include <mutex>


namespace RaptorQ__v1 {
namespace Impl {


template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_LOCAL Encoder
{
public:
	~Encoder();
    // used for precomputation
    Encoder (const uint16_t symbols, const uint16_t symbol_size);
    // with data at the beginning. Less work.
    Encoder (const Rnd_It data_from, const Rnd_It data_to,
													const uint16_t symbol_size);
	uint16_t symbols() const;
	uint16_t symbol_size() const;
	uint32_t max_repair() const;

    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> begin_source();
	RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> end_source();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> begin_repair();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> end_repair
														(const uint32_t repair);

	uint64_t add_data (Rnd_It from, const Rnd_It to);
	bool compute_sync();

    std::future<Error> compute();
    uint64_t encode (Fwd_It &output, const Fwd_It end, const uint32_t &id);

private:
	enum class Data_State : uint8_t {
		NEED_DATA = 1,	// first constructor used. no interleaver until FULL
		FULL = 2,
		INIT = 3	// second constructor used: we already have the interleaver
	};

    std::unique_ptr<RFC6330__v1::Impl::Interleaver<Rnd_It>> interleaver;
	Raw_Encoder<Rnd_It, Fwd_It> encoder;
	DenseMtx precomputed;
	std::vector<typename std::iterator_traits<Rnd_It>::value_type> data;
	std::mutex data_mtx;
	const uint16_t _symbols, _symbol_size;
	RaptorQ__v1::Work_State work;
    Data_State state;


	static uint16_t real_symbol_size (const uint16_t symbol_size);
	static uint16_t calc_symbols (const Rnd_It data_from, const Rnd_It data_to,
													const uint16_t symbol_size);
};

template <typename In_It, typename Fwd_It>
class RAPTORQ_LOCAL Decoder
{
public:

	enum class RAPTORQ_LOCAL Report : uint8_t {
		PARTIAL_FROM_BEGINNING = 1,
		PARTIAL_ANY = 2,
		COMPLETE = 3
	};

	~Decoder();
    Decoder (const uint64_t bytes, const uint16_t symbol_size,
															const Report type);

	uint16_t symbols() const;
	uint16_t symbol_size() const;

	RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> begin ();
    RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> end ();

	Error add_symbol (In_It from, const In_It to, const uint32_t esi);
	using Decoder_Result = typename Raw_Decoder<In_It>::Decoder_Result;

	bool can_decode() const;
	Decoder_Result decode();
	void stop();

	std::pair<Error, uint16_t> poll();
	std::future<std::pair<Error, uint16_t>> wait (bool async);

	// return number of symbols.
	// simbol_size % sizeof(FWD) == 0 else assert!
	// returns number of iterators written
	uint64_t decode_symbol (Fwd_It &start, const Fwd_It end,const uint16_t esi);
	std::pair<uint64_t, size_t> decode_bytes (Fwd_It &start, const Fwd_It end,
									const size_t from_byte, const size_t skip);
private:
	const uint64_t _bytes;
	const uint16_t _symbols, _symbol_size;
	int32_t last_reported;
	const Report _type;
	RaptorQ__v1::Work_State work;
	Raw_Decoder<In_It> dec;
	// 2* symbols. actually tracks available and reported symbols.
	std::vector<bool> symbols_tracker;
	std::mutex _mtx;
	std::condition_variable _cond;
	std::vector<std::thread> waiting;

	static uint16_t get_symbols (const uint64_t bytes,
													const uint16_t symbol_size);
	static void waiting_thread (Decoder<In_It, Fwd_It> *obj,
									std::promise<std::pair<Error, uint16_t>> p);
};


///////////////////
//// Encoder
///////////////////

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::~Encoder()
{
	encoder.stop();
}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::Encoder (const uint16_t symbols,
													const uint16_t symbol_size)
	: interleaver (nullptr), encoder (symbols), _symbols (symbols),
													_symbol_size (symbol_size)
{
	IS_RANDOM(Rnd_It, "RaptorQ__v1::Encoder");
	IS_FORWARD(Fwd_It, "RaptorQ__v1::Encoder");
    state = Data_State::NEED_DATA;
	work = RaptorQ__v1::Work_State::KEEP_WORKING;
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::real_symbol_size (const uint16_t symbol_size)
{
	using T = typename std::iterator_traits<Rnd_It>::value_type;
	return symbol_size * sizeof(T);
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::calc_symbols (const Rnd_It data_from,
													const Rnd_It data_to,
													const uint16_t symbol_size)
{
	using T = typename std::iterator_traits<Rnd_It>::value_type;

	uint64_t size = static_cast<uint64_t> (data_to - data_from) * sizeof(T);
	uint16_t symbols = static_cast<uint16_t> (size / symbol_size);
	if ((size % symbol_size) != 0)
		++symbols;
	return symbols;
}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::Encoder (const Rnd_It data_from, const Rnd_It data_to,
                                            const uint16_t symbol_size)
    : interleaver (new RFC6330__v1::Impl::Interleaver<Rnd_It> (data_from,
									data_to, real_symbol_size(symbol_size),
									SIZE_MAX, real_symbol_size(symbol_size))),
	  encoder (interleaver.get(), 0),
		_symbols (calc_symbols (data_from, data_to,
												real_symbol_size(symbol_size))),
		_symbol_size (real_symbol_size(symbol_size))
{
	IS_RANDOM(Rnd_It, "RaptorQ__v1::Encoder");
	IS_FORWARD(Fwd_It, "RaptorQ__v1::Encoder");
    state = Data_State::INIT;
	work = RaptorQ__v1::Work_State::KEEP_WORKING;
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbols() const
{
	return _symbols;
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbol_size() const
{
	return _symbol_size;
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::max_repair() const
{
	return static_cast<uint32_t> (std::pow(2, 20)) - _symbols;
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
										Encoder<Rnd_It, Fwd_It>::begin_source()
{
	return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (this, 0);
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
										Encoder<Rnd_It, Fwd_It>::end_source()
{
	return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (this,
																	_symbols);
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
										Encoder<Rnd_It, Fwd_It>::begin_repair()
{
	return end_source();
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
					Encoder<Rnd_It, Fwd_It>::end_repair (const uint32_t repair)
{
	return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (nullptr,
															_symbols + repair);
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Encoder<Rnd_It, Fwd_It>::add_data (Rnd_It from, const Rnd_It to)
{
	uint64_t written = 0;
	using T = typename std::iterator_traits<Rnd_It>::value_type;

	if (state != Data_State::NEED_DATA)
		return written;
	std::unique_lock<std::mutex> lock (data_mtx);
	RQ_UNUSED (lock);
	while (from != to) {
		if ((data.size() * sizeof (T) >= _symbols * _symbol_size)) {
			state = Data_State::FULL;
			break;
		}
		data.push_back (from);
		++from;
		++written;
	}
	return written;
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::compute_sync()
{
	if (state == Data_State::INIT) {
		return encoder.generate_symbols (&work);
	} else {
		if (precomputed.rows() != 0)
			return true;
		precomputed = encoder.get_precomputed (&work);
		return precomputed.rows() != 0;
	}
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
															const uint32_t &id)
{
	switch (state) {
	case Data_State::INIT:
		if (!encoder.ready())
			return 0;
		return encoder.Enc (id, output, end);
	case Data_State::NEED_DATA:
		break;
	case Data_State::FULL:
		if (!encoder.ready()) {
			if (precomputed.rows() == 0) {
				return 0;
			} else {
				interleaver = std::unique_ptr<
							RFC6330__v1::Impl::Interleaver<Rnd_It>> (
									new RFC6330__v1::Impl::Interleaver<Rnd_It> (
													data.begin(), data.end(),
													_symbol_size, SIZE_MAX,
																_symbol_size));
				encoder.generate_symbols (precomputed, interleaver.get());
				precomputed = DenseMtx();	// free mem
			}
		}
		return encoder.Enc (id, output, end);
	}
	return 0;
}


///////////////////
//// Decoder
///////////////////

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::get_symbols (const uint64_t bytes,
													const uint16_t symbol_size)
{
	uint16_t symbols = static_cast<uint16_t> (bytes / symbol_size);
	if (bytes % symbol_size != 0)
		++symbols;
	return symbols;
}

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::~Decoder ()
{
	work = RaptorQ__v1::Work_State::ABORT_COMPUTATION;
	_cond.notify_all();
	// wait threads to exit
	do {
		std::unique_lock<std::mutex> lock (_mtx);
		if (waiting.size() == 0)
			break;
		_cond.wait (lock);
		lock.unlock();
	} while (waiting.size() != 0);
}

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::Decoder (const uint64_t bytes,
								const uint16_t symbol_size, const Report type)
	:_bytes (bytes), _symbols (get_symbols (bytes, symbol_size)),
									_symbol_size (symbol_size), _type (type),
													dec (_symbols, symbol_size)
{
	IS_INPUT(In_It, "RaptorQ__v1::Decoder");
	IS_FORWARD(Fwd_It, "RaptorQ__v1::Decoder");
	//dec = Raw_Decoder<In_It> (_symbols, symbol_size);
	symbols_tracker = std::vector<bool> (2 * _symbols, false);
	last_reported = -1;
	work = RaptorQ__v1::Work_State::KEEP_WORKING;
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbols() const
{
	return _symbols;
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbol_size() const
{
	return _symbol_size;
}

template <typename In_It, typename Fwd_It>
RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It>
												Decoder<In_It, Fwd_It>::begin()
{
	return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (this, 0);
}

template <typename In_It, typename Fwd_It>
RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It>
												Decoder<In_It, Fwd_It>::end()
{
	return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (nullptr,
																	_symbols);
}


template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::add_symbol (In_It from, const In_It to,
															const uint32_t esi)
{
	auto ret = dec.add_symbol (from, to, esi);
	if (ret == Error::NONE && esi < _symbols) {
		symbols_tracker [2 * esi] = true;
		std::unique_lock<std::mutex> lock (_mtx);
		RQ_UNUSED (lock);
		_cond.notify_all();
	}
	return ret;
}


template <typename In_It, typename Fwd_It>
std::pair<Error, uint16_t> Decoder<In_It, Fwd_It>::poll ()
{
	std::unique_lock<std::mutex> lock (_mtx, std::defer_lock);
	int32_t id;
	uint32_t to_report;
	switch (_type) {
	case Report::PARTIAL_FROM_BEGINNING:
		lock.lock();
		id = std::max (0, last_reported);
		to_report = 0;
		if (id < 0)
			id = 0;
		for (; id < symbols_tracker.size(); id += 2) {
			if (symbols_tracker[id] == true) {
				++id;
				if (symbols_tracker[id] == false) {
					symbols_tracker[id] = true;
					++to_report;
				}
			} else {
				break;
			}
		}
		if (to_report > 0 || dec->ready()) {
			last_reported += to_report;
			return {Error::NONE, last_reported};
		}
		if (!dec.can_decode())
			return {Error::NEED_DATA, 0};
		return {Error::WORKING, 0};
	case Report::PARTIAL_ANY:
		for (id = 0; id < symbols_tracker.size(); id += 2) {
			if (symbols_tracker[id] == true) {
				++id;
				lock.lock();
				if (symbols_tracker[id] == false) {
					symbols_tracker[id] = true;
					return {Error::NONE, id / 2};
				}
			}
		}
		if (dec->ready())
			return {Error::NONE, 0};
		if (dec.can_decode())
			return {Error::NEED_DATA, 0};
		return {Error::WORKING, 0};
	case Report::COMPLETE:
		for (id = last_reported; id < symbols_tracker.size(); id += 2) {
			if (symbols_tracker[id] == false) {
				if (dec.can_decode())
					return {Error::WORKING, 0};
				return {Error::NEED_DATA, 0};
			}
		}
		last_reported = symbols_tracker.size();
		return {Error::NONE, 0};
	}
	return {Error::WORKING, 0};
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::waiting_thread (Decoder<In_It, Fwd_It> *obj,
									std::promise<std::pair<Error, uint16_t>> p)
{
	while (obj->work == RaptorQ__v1::Work_State::KEEP_WORKING) {
		std::unique_lock<std::mutex> lock (obj->_mtx);
		auto res = obj->poll();
		if (obj->poll.first == Error::NONE) {
			p.set_value (res);
			break;
		}
		obj->_cond.wait (lock);
		res = obj->poll();
		lock.unlock();
		if (obj->poll.first == Error::NONE) {
			p.set_value (res);
			break;
		}
	}
	if (obj->work != RaptorQ__v1::Work_State::KEEP_WORKING)
		p.set_value ({Error::EXITING, 0});

	std::unique_lock<std::mutex> lock (obj->_mtx);
	RQ_UNUSED (lock);
	for (auto th = obj->waiting.begin(); th != obj->waiting.end(); ++th) {
		if (std::this_thread::get_id() == th.id()) {
			th.detach();
			obj->waiting.erase (th);
			break;
		}
	}
	obj->_cond.notify_all(); // notify exit to destructor
}

template <typename In_It, typename Fwd_It>
std::future<std::pair<Error, uint16_t>> Decoder<In_It, Fwd_It>::wait (
															const bool async)
{
	std::promise<std::pair<Error, uint16_t>> p;
	if (async) {
		waiting_thread (this, std::move(p));
	} else {
		waiting.emplace_back (waiting_thread, this, std::move(p));
	}
	return p.get_future();
}

template <typename In_It, typename Fwd_It>
bool Decoder<In_It, Fwd_It>::can_decode() const
{
	return dec.can_decode();
}

template <typename In_It, typename Fwd_It>
typename Decoder<In_It, Fwd_It>::Decoder_Result Decoder<In_It, Fwd_It>::decode()
{
	auto res = dec.decode (&work);
	if (res == Decoder_Result::DECODED) {
		std::unique_lock<std::mutex> lock (_mtx);
		RQ_UNUSED (lock);
		if (_type != Report::COMPLETE) {
			uint32_t id = static_cast<uint32_t> (std::max (0, last_reported));
			for (; id < symbols_tracker.size(); id += 2)
				symbols_tracker[id] = true;
		}
		_cond.notify_all();
	}
	return res;
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::stop()
{
	work = RaptorQ__v1::Work_State::ABORT_COMPUTATION;
	std::unique_lock<std::mutex> lock (_mtx);
	_cond.notify_all();
}

template <typename In_It, typename Fwd_It>
std::pair<uint64_t, size_t> Decoder<In_It, Fwd_It>::decode_bytes (Fwd_It &start,
													const Fwd_It end,
													const uint64_t from_byte,
													const size_t skip)
{
	using T = typename std::iterator_traits<Fwd_It>::value_type;
	if (!dec.ready() || skip >= sizeof(T))	// !dec.ready()
		return {0, 0};

	auto decoded = dec.get_symbols();

	uint16_t esi = static_cast<uint16_t> (from_byte /
									static_cast<uint64_t> (decoded->cols()));
	uint16_t byte = static_cast<uint16_t> (from_byte %
									static_cast<uint64_t> (decoded->cols()));

	size_t offset_al = skip;
	T element = static_cast<T> (0);
	if (skip != 0) {
		uint8_t *p = reinterpret_cast<uint8_t *> (&*start);
		for (size_t keep = 0; keep < skip; ++keep) {
			element += static_cast<T> (*(p++)) << keep * 8;
		}
	}
	uint64_t written = 0;
	while (start != end && esi < decoded->rows() &&
									_bytes > from_byte + written + offset_al) {
		element += static_cast<T> (static_cast<uint8_t> ((*decoded)(esi, byte)))
															<< offset_al * 8;
		++offset_al;
		if (offset_al == sizeof(T)) {
			*start = element;
			++start;
			written += offset_al;
			offset_al = 0;
			element = static_cast<T> (0);
		}
		++byte;
		if (byte == decoded->cols()) {
			byte = 0;
			++esi;
		}
	}
	if (start != end && offset_al != 0) {
		// we have more stuff in "element", but not enough to fill
		// the iterator.
		*start = element;
		written += offset_al;
	}
	return {written, offset_al};
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::decode_symbol (Fwd_It &start, const Fwd_It end,
															const uint16_t esi)
{
	assert ((_symbol_size %
			sizeof(typename std::iterator_traits<Fwd_It>::value_type)) == 0);
	if (!dec.ready())
		return 0;
	size_t esi_byte = esi * dec->cols();
	auto pair = decode_bytes (start, end, esi_byte, 0);
	assert (pair.second == 0 );
	return pair.first / _symbol_size;
}

}   // namespace Impl
}   // namespace RaptorQ__v1
