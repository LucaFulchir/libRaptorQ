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

/////////////////////
//
//	These templates are just a wrapper around the
//	functionalities offered by the RaptorQ__v1::Impl namespace
//	So if you want to see what the algorithm looks like,
//	you are in the wrong place
//
/////////////////////

#include "RaptorQ/v1/Interleaver.hpp"
#include "RaptorQ/v1/De_Interleaver.hpp"
#include "RaptorQ/v1/Decoder.hpp"
#include "RaptorQ/v1/Encoder.hpp"
#include "RaptorQ/v1/API_Iterators.hpp"
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"
#include "RaptorQ/v1/Thread_Pool.hpp"
#include <algorithm>
#include <cassert>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

namespace RFC6330__v1 {

typedef uint64_t RQ_OTI_Common_Data;
typedef uint32_t RQ_OTI_Scheme_Specific_Data;

namespace Impl {

// maximum times a single block can be decoded at the same time.
// the decoder can be launched multiple times with different combinations
// of repair symbols. This can be useful as the decoding is actually
// probabilistic, and dropping a set of repair symbols *MIGHT* make things
// decodable again.
// keep this low. 1, 2, 3 should be ok.
static uint16_t max_block_decoder_concurrency = 1;
static const uint64_t max_data = 946270874880;	// ~881 GB


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
											_min_subsymbol (min_subsymbol_size),
											interleave (_data_from,
														_data_to,
														_min_subsymbol,
														_mem,
														_symbol_size)
	{
		IS_RANDOM(Rnd_It, "RFC6330__v1::Encoder");
		IS_FORWARD(Fwd_It, "RFC6330__v1::Encoder");
		auto _alignment = sizeof(typename
									std::iterator_traits<Rnd_It>::value_type);
		RQ_UNUSED(_alignment);	// used only for asserts
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

		pool_lock = std::make_shared<std::pair<std::mutex,
												std::condition_variable>> ();
		pool_last_reported = -1;
		use_pool = true;
		exiting = false;
	}

	Block_Iterator<Rnd_It, Fwd_It> begin ()
	{
		return Block_Iterator<Rnd_It, Fwd_It> (this,
												interleave.get_partition(), 0);
	}
	const Block_Iterator<Rnd_It, Fwd_It> end ()
	{
		auto part = interleave.get_partition();
		return Block_Iterator<Rnd_It, Fwd_It> (this, part,
							static_cast<uint8_t> (part.num(0) + part.num(1)));
	}

	operator bool() const { return interleave; }
	RQ_OTI_Common_Data OTI_Common() const;
	RQ_OTI_Scheme_Specific_Data OTI_Scheme_Specific() const;

	// TODO: introduce memory limits on threading ?
	std::future<std::pair<Error, uint8_t>> compute (const Compute flags);

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

	static void wait_threads (Encoder<Rnd_It, Fwd_It> *obj, const Compute flags,
									std::promise<std::pair<Error, uint8_t>> p);

	class Block_Work : public Impl::Pool_Work {
	public:
		std::weak_ptr<RaptorQ__v1::Impl::Raw_Encoder<Rnd_It, Fwd_It>> work;
		std::weak_ptr<std::pair<std::mutex, std::condition_variable>> notify;

		Work_Exit_Status do_work (RaptorQ__v1::Work_State *state) override;
		~Block_Work() override {}
	};

	// TODO: tagged pointer
	class Enc {
	public:
		Enc (const Impl::Interleaver<Rnd_It> *interleaver, const uint8_t sbn)
		{
			enc = std::make_shared<
							RaptorQ__v1::Impl::Raw_Encoder<Rnd_It, Fwd_It>> (
															interleaver, sbn);
			reported = false;
		}
		std::shared_ptr<RaptorQ__v1::Impl::Raw_Encoder<Rnd_It, Fwd_It>> enc;
		bool reported;
	};

	std::pair<Error, uint8_t> get_report (const Compute flags);
	std::shared_ptr<std::pair<std::mutex, std::condition_variable>> pool_lock;
	std::deque<std::thread> pool_wait;

	std::map<uint8_t, Enc> encoders;
	std::mutex _mtx;

	const size_t _mem;
	const Rnd_It _data_from, _data_to;
	const uint16_t _symbol_size;
	const uint16_t _min_subsymbol;
	const Impl::Interleaver<Rnd_It> interleave;
	bool use_pool, exiting;
	int16_t pool_last_reported;

};

template <typename In_It, typename Fwd_It>
class RAPTORQ_API Decoder
{
public:

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
	~Decoder();
	Decoder (const RQ_OTI_Common_Data common,
                            const RQ_OTI_Scheme_Specific_Data scheme)
	{
		IS_INPUT(In_It, "RaptorQ__v1::Decoder");
		IS_FORWARD(Fwd_It, "RaptorQ__v1::Decoder");

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
		pool_lock = std::make_shared<std::pair<std::mutex,
												std::condition_variable>> ();
		pool_last_reported = -1;
		use_pool = true;
		exiting = false;
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
		pool_last_reported = -1;
		use_pool = true;
		exiting = false;
	}

	std::future<std::pair<Error, uint8_t>> compute (const Compute flags);

	// result in BYTES
	uint64_t decode_bytes (Fwd_It &start, const Fwd_It end, const uint8_t skip);
	uint64_t decode_block_bytes (Fwd_It &start, const Fwd_It end,
															const uint8_t skip,
															const uint8_t sbn);
	// result in ITERATORS
	// last *might* be half written depending on data alignments
	std::pair<size_t, uint8_t> decode_aligned (Fwd_It &start, const Fwd_It end,
															const uint8_t skip);
	std::pair<size_t, uint8_t> decode_block_aligned (Fwd_It &start,
															const Fwd_It end,
															const uint8_t skip,
															const uint8_t sbn);
	// id: 8-bit sbn + 24 bit esi
	Error add_symbol (In_It &start, const In_It end, const uint32_t id);
	Error add_symbol (In_It &start, const In_It end, const uint32_t esi,
															const uint8_t sbn);
	void free (const uint8_t sbn);
	uint64_t bytes() const;
	uint8_t blocks() const;
	uint32_t block_size (const uint8_t sbn) const;
	uint16_t symbol_size() const;
	uint16_t symbols (const uint8_t sbn) const;
private:
	// using shared pointers to avoid locking too much or
	// worrying about deleting used stuff.
	class Block_Work : public Impl::Pool_Work {
	public:
		std::weak_ptr<RaptorQ__v1::Impl::Raw_Decoder<In_It>> work;
		std::weak_ptr<std::pair<std::mutex, std::condition_variable>> notify;

		Work_Exit_Status do_work (RaptorQ__v1::Work_State *state) override;
		~Block_Work() override {}
	};
	// TODO: tagged pointer
	class Dec {
	public:
		Dec (const uint16_t symbols, const uint16_t symbol_size)
		{
			dec = std::make_shared<RaptorQ__v1::Impl::Raw_Decoder<In_It>> (symbols,
																symbol_size);
			reported = false;
		}
		std::shared_ptr<RaptorQ__v1::Impl::Raw_Decoder<In_It>> dec;
		bool reported;
	};

	static void wait_threads (Decoder<In_It, Fwd_It> *obj, const Compute flags,
									std::promise<std::pair<Error, uint8_t>> p);
	std::pair<Error, uint8_t> get_report (const Compute flags);
	std::shared_ptr<std::pair<std::mutex, std::condition_variable>> pool_lock;
	std::deque<std::thread> pool_wait;

	uint64_t _size;
	Impl::Partition part, _sub_blocks;
	std::map<uint8_t, Dec> decoders;
	std::mutex _mtx;
	uint16_t _symbol_size;
	int16_t pool_last_reported;
	uint8_t _blocks, _alignment;
	bool use_pool, exiting;

	std::vector<bool> decoded_sbn;

};


/////////////////
//
// Encoder
//
/////////////////


template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::~Encoder()
{
	exiting = true;	// stop notifying thread
	pool_lock->second.notify_all();
	for (auto &it : encoders) {	// stop existing computations
		auto ptr = it.second.enc;
		if (ptr != nullptr)
			ptr->stop();
	}
	do {
		std::unique_lock<std::mutex> lock (_mtx);
		if (pool_wait.size() != 0)
			pool_lock->second.wait (lock);
	} while (pool_wait.size() != 0);
}

template <typename Rnd_It, typename Fwd_It>
RQ_OTI_Common_Data Encoder<Rnd_It, Fwd_It>::OTI_Common() const
{
	if (!interleave)
		return 0;
	RQ_OTI_Common_Data ret;
	// first 40 bits: data length.
	ret = (static_cast<uint64_t> (_data_to - _data_from) *
			sizeof(typename std::iterator_traits<Rnd_It>::value_type)) << 24;
	// 8 bits: reserved
	// last 16 bits: symbol size
	ret += _symbol_size;

	return ret;
}

template <typename Rnd_It, typename Fwd_It>
RQ_OTI_Scheme_Specific_Data Encoder<Rnd_It, Fwd_It>::OTI_Scheme_Specific() const
{
	if (!interleave)
		return 0;
	RQ_OTI_Scheme_Specific_Data ret;
	// 8 bit: source blocks
	ret = static_cast<uint32_t> (interleave.blocks()) << 24;
	// 16 bit: sub-blocks number (N)
	ret += static_cast<uint32_t> (interleave.sub_blocks()) << 8;
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

	if (!interleave)
		return 0;

	uint16_t symbols = interleave.source_symbols (0);

	uint16_t K_idx;
	for (K_idx = 0; K_idx < RaptorQ__v1::Impl::K_padded.size(); ++K_idx) {
		if (symbols < RaptorQ__v1::Impl::K_padded[K_idx])
			break;
	}
	if (K_idx == RaptorQ__v1::Impl::K_padded.size())
		return 0;

	auto S_H = RaptorQ__v1::Impl::S_H_W[K_idx];
	uint16_t matrix_cols = RaptorQ__v1::Impl::K_padded[K_idx] +
															std::get<0> (S_H) +
															std::get<1> (S_H);

	// Rough memory estimate: Matrix A, matrix X (=> *2) and matrix D.
	return matrix_cols * matrix_cols * 2 + _symbol_size * matrix_cols;
}

template <typename Rnd_It, typename Fwd_It>
Work_Exit_Status Encoder<Rnd_It, Fwd_It>::Block_Work::do_work (
												RaptorQ__v1::Work_State *state)
{
	auto locked_enc = work.lock();
	auto locked_notify = notify.lock();
	if (locked_enc != nullptr && locked_notify != nullptr) {
		// encoding always works. It's one of the few constants of the universe.
		if (!locked_enc->generate_symbols (state))
			return Work_Exit_Status::STOPPED;	// only explanation.
		std::lock_guard<std::mutex> mtx (locked_notify->first);
		RQ_UNUSED(mtx);
		locked_notify->second.notify_one();
	}
	return Work_Exit_Status::DONE;
}

template <typename Rnd_It, typename Fwd_It>
std::future<std::pair<Error, uint8_t>> Encoder<Rnd_It, Fwd_It>::compute (
															const Compute flags)
{
	using ret_t = std::pair<Error, uint8_t>;
	std::promise<ret_t> p;

	bool error = !interleave;
	// need some flags
	if (flags == Compute::NONE)
		error = true;

	// flag incompatibilities
	if (Compute::NONE != (flags & Compute::PARTIAL_FROM_BEGINNING) &&
					(Compute::NONE != (flags & (Compute::PARTIAL_ANY |
												Compute::COMPLETE |
												Compute::NO_POOL)))) {
			error = true;
	} else if (Compute::NONE != (flags & Compute::PARTIAL_ANY) &&
				(Compute::NONE != (flags & (Compute::PARTIAL_FROM_BEGINNING |
											Compute::COMPLETE |
											Compute::NO_POOL)))) {
			error = true;
	} else if (Compute::NONE != (flags & Compute::COMPLETE) &&
					Compute::NONE != (flags &(Compute::PARTIAL_FROM_BEGINNING |
												Compute::PARTIAL_ANY |
												Compute::NO_POOL))) {
			error = true;
	}

	if (Compute::NONE != (flags & Compute::NO_POOL)) {
		std::unique_lock<std::mutex> lock (_mtx);
		RQ_UNUSED(lock);
		if (encoders.size() != 0) {
			// You can only say you won't use the pool *before* you start
			// decoding something!
			error = true;
		} else {
			use_pool = false;
			p.set_value ({Error::NONE, 0});
			return p.get_future();
		}
	}

	if (error) {
		p.set_value ({Error::WRONG_INPUT, 0});
		return p.get_future();
	}

	// flags are fine, add work to pool
	std::unique_lock<std::mutex> lock (_mtx);
	for (uint8_t block = 0; block < blocks(); ++block) {
		auto enc = encoders.find (block);
		if (enc == encoders.end()) {
			bool success;
			std::tie (enc, success) = encoders.emplace (
									std::piecewise_construct,
									std::forward_as_tuple (block),
									std::forward_as_tuple (&interleave, block));
			assert (success == true);
			std::unique_ptr<Block_Work> work = std::unique_ptr<Block_Work>(
															new Block_Work());
			work->work = enc->second.enc;
			work->notify = pool_lock;
			Impl::Thread_Pool::get().add_work (std::move(work));
		}
	}
	lock.unlock();

	// spawn thread waiting for other thread exit.
	// this way we can set_value to the future when needed.
	auto future = p.get_future();
	if (Compute::NONE != (flags & Compute::NO_BACKGROUND)) {
		wait_threads (this, flags, std::move(p));
	} else {
		std::unique_lock<std::mutex> pool_wait_lock (_mtx);
		RQ_UNUSED(pool_wait_lock);
		pool_wait.emplace_back(wait_threads, this, flags, std::move(p));
	}
	return future;
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::wait_threads (Encoder<Rnd_It, Fwd_It> *obj,
									const Compute flags,
									std::promise<std::pair<Error, uint8_t>> p)
{
	do {
		if (obj->exiting) {	// make sure we can exit
			p.set_value ({Error::NONE, 0});
			break;
		}
		// pool is global (static), so wait only for our stuff.
		std::unique_lock<std::mutex> lock (obj->pool_lock->first);
		if (obj->exiting) { // make sure we can exit
			p.set_value ({Error::NONE, 0});
			break;
		}
		auto status = obj->get_report (flags);
		if (status.first != Error::WORKING) {
			p.set_value (status);
			break;
		}

		obj->pool_lock->second.wait (lock); // conditional wait
		if (obj->exiting) {	// make sure we can exit
			p.set_value ({Error::NONE, 0});
			break;
		}
		status = obj->get_report (flags);
		lock.unlock();	// unlock
		if (status.first != Error::WORKING) {
			p.set_value (status);
			break;
		}
	} while (true);

	// delete ourselves from the waiting thread vector.
	std::unique_lock<std::mutex> lock (obj->_mtx);
	RQ_UNUSED (lock);
	for (auto it = obj->pool_wait.begin(); it != obj->pool_wait.end(); ++it) {
		if (it->get_id() == std::this_thread::get_id()) {
			it->detach();
			obj->pool_wait.erase (it);
			break;
		}
	}
	obj->pool_lock->second.notify_all();
}

template <typename Rnd_It, typename Fwd_It>
std::pair<Error, uint8_t> Encoder<Rnd_It, Fwd_It>::get_report (
														const Compute flags)
{
	if (Compute::NONE != (flags & Compute::COMPLETE) ||
				Compute::NONE != (flags & Compute::PARTIAL_FROM_BEGINNING)) {
		auto it = encoders.begin();
		for (; it != encoders.end(); ++it) {
			auto ptr = it->second.enc;
			if (ptr != nullptr && !ptr->ready())
				break;
		}
		if (it == encoders.end()) {
			pool_last_reported = static_cast<int16_t> (encoders.size() - 1);
			return {Error::NONE, pool_last_reported};
		}
		if (Compute::NONE != (flags & Compute::PARTIAL_FROM_BEGINNING) &&
									(pool_last_reported < (it->first - 1))) {
			pool_last_reported = it->first - 1;
			return {Error::NONE, pool_last_reported};
		}
		return {Error::WORKING, 0};
	}
	if (Compute::NONE != (flags & Compute::PARTIAL_ANY)) {
		for (auto &it : encoders) {
			if (!it.second.reported) {
				auto ptr = it.second.enc;
				if (ptr != nullptr && ptr->ready()) {
					return {Error::NONE, it.first};
				}
			}
		}
	}
	return {Error::WORKING, 0};	// should never be reached
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
	if (sbn >= interleave.blocks())
		return 0;

	std::unique_lock<std::mutex> lock (_mtx);
	auto it = encoders.find (sbn);
	if (use_pool) {
		if (it == encoders.end())
			return 0;
		auto shared_enc = it->second.enc;
		if (!shared_enc->ready())
			return 0;
		lock.unlock();
		return shared_enc->Enc (esi, output, end);
	} else {
		if (it == encoders.end()) {
			bool success;
			std::tie (it, success) = encoders.emplace (std::make_pair (sbn,
													Enc (&interleave, sbn)));
			auto shared_enc = it->second.enc;
			lock.unlock();
			RaptorQ__v1::Work_State state =
										RaptorQ__v1::Work_State::KEEP_WORKING;
			shared_enc->generate_symbols (&state);
			return shared_enc->Enc (esi, output, end);
		} else {
			auto shared_enc = it->second.enc;
			lock.unlock();
			if (!shared_enc->ready())
				return 0;
			return shared_enc->Enc (esi, output, end);
		}
	}
}


template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::free (const uint8_t sbn)
{
	std::unique_lock<std::mutex> lock (_mtx);
	RQ_UNUSED(lock);
	auto it = encoders.find (sbn);
	if (it != encoders.end())
		encoders.erase (it);
}

template <typename Rnd_It, typename Fwd_It>
uint8_t Encoder<Rnd_It, Fwd_It>::blocks() const
{
	if (!interleave)
		return 0;
	return interleave.blocks();
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::block_size (const uint8_t sbn) const
{
	if (!interleave)
		return 0;
	return interleave.source_symbols (sbn) * interleave.symbol_size();
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbol_size() const
{
	if (!interleave)
		return 0;
	return interleave.symbol_size();
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbols (const uint8_t sbn) const
{
	if (!interleave)
		return 0;
	return interleave.source_symbols (sbn);
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::max_repair (const uint8_t sbn) const
{
	if (!interleave)
		return 0;
	return static_cast<uint32_t> (std::pow (2, 20)) -
											interleave.source_symbols (sbn);
}

/////////////////
//
// Decoder
//
/////////////////

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::~Decoder()
{
	exiting = true;	// stop notifying thread
	pool_lock->second.notify_all();
	for (auto &it : decoders) {	// stop existing computations
		auto ptr = it.second.dec;
		if (ptr != nullptr)
			ptr->stop();
	}
	do {
		std::unique_lock<std::mutex> lock (_mtx);
		if (pool_wait.size() != 0)
			pool_lock->second.wait (lock);
	} while (pool_wait.size() != 0);
}


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
Error Decoder<In_It, Fwd_It>::add_symbol (In_It &start, const In_It end,
															const uint32_t id)
{
	uint32_t esi = (id << 8 ) >> 8;
	uint8_t sbn = id >> 24;

	return add_symbol (start, end, esi, sbn);
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::add_symbol (In_It &start, const In_It end,
										const uint32_t esi, const uint8_t sbn)
{
	if (sbn >= _blocks)
		return Error::WRONG_INPUT;
	std::unique_lock<std::mutex> lock (_mtx);
	auto it = decoders.find (sbn);
	if (it == decoders.end()) {
		const uint16_t symbols = sbn < part.num (0) ?
													part.size(0) : part.size(1);
		bool success;
		std::tie (it, success) = decoders.emplace (std::make_pair(sbn,
												Dec (symbols, _symbol_size)));
		assert (success);
	}
	auto dec = it->second.dec;
	lock.unlock();

 	auto err = dec->add_symbol (start, end, esi);
	if (err != Error::NONE)
		return err;
	// automatically add work to pool if we use it and have enough data
	lock.lock();
	if (use_pool && dec->can_decode()) {
		bool add_work = dec->add_concurrent (max_block_decoder_concurrency);
		if (add_work) {
			std::unique_ptr<Block_Work> work = std::unique_ptr<Block_Work>(
															new Block_Work());
			work->work = dec;
			work->notify = pool_lock;
			Impl::Thread_Pool::get().add_work (std::move(work));
		}
	}
	return Error::NONE;
}


template <typename In_It, typename Fwd_It>
Work_Exit_Status Decoder<In_It, Fwd_It>::Block_Work::do_work (
												RaptorQ__v1::Work_State *state)
{
	auto locked_dec = work.lock();
	auto locked_notify = notify.lock();
	if (locked_dec != nullptr && locked_notify != nullptr) {
		auto ret = locked_dec->decode (state);
		// initialize, do not lock yet
		std::unique_lock<std::mutex> locked_guard (locked_notify->first,
															std::defer_lock);
		switch (ret) {
		case RaptorQ__v1::Impl::Raw_Decoder<In_It>::Decoder_Result::DECODED:
			locked_guard.lock();
			locked_notify->second.notify_one();
			locked_dec->drop_concurrent();
			return Work_Exit_Status::DONE;
		case RaptorQ__v1::Impl::Raw_Decoder<In_It>::Decoder_Result::NEED_DATA:
			locked_dec->drop_concurrent();
			return Work_Exit_Status::DONE;
		case RaptorQ__v1::Impl::Raw_Decoder<In_It>::Decoder_Result::STOPPED:
			return Work_Exit_Status::STOPPED;
		case RaptorQ__v1::Impl::Raw_Decoder<In_It>::Decoder_Result::CAN_RETRY:
			return Work_Exit_Status::REQUEUE;
		}
	}
	return Work_Exit_Status::DONE;
}

template <typename In_It, typename Fwd_It>
std::future<std::pair<Error, uint8_t>> Decoder<In_It, Fwd_It>::compute (
														const Compute flags)
{
	using ret_t = std::pair<Error, uint8_t>;
	std::promise<ret_t> p;

	bool error = false;
	// need some flags
	if (flags == Compute::NONE)
		error = true;

	// flag incompatibilities
	if (Compute::NONE != (flags & Compute::PARTIAL_FROM_BEGINNING) &&
							(Compute::NONE != (flags & (Compute::PARTIAL_ANY |
														Compute::COMPLETE |
														Compute::NO_POOL)))) {
		error = true;
	} else if (Compute::NONE != (flags & Compute::PARTIAL_ANY) &&
				(Compute::NONE != (flags & (Compute::PARTIAL_FROM_BEGINNING |
											Compute::COMPLETE |
											Compute::NO_POOL)))) {
		error = true;
	} else if (Compute::NONE != (flags & Compute::COMPLETE) &&
					Compute::NONE != (flags &(Compute::PARTIAL_FROM_BEGINNING |
												Compute::PARTIAL_ANY |
												Compute::NO_POOL))) {
		error = true;
	}

	if (Compute::NONE != (flags & Compute::NO_POOL)) {
		std::unique_lock<std::mutex> lock (_mtx);
		RQ_UNUSED(lock);
		if (decoders.size() != 0) {
			// You can only say you won't use the pool *before* you start
			// decoding something!
			error = true;
		} else {
			use_pool = false;
			p.set_value ({Error::NONE, 0});
			return p.get_future();
		}
	}

	if (error) {
		p.set_value ({Error::WRONG_INPUT, 0});
		return p.get_future();
	}

	// do not add work to the pool to save up memory.
	// let "add_symbol craete the Decoders as needed.

	// spawn thread waiting for other thread exit.
	// this way we can set_value to the future when needed.
	auto future = p.get_future();
	if (Compute::NONE != (flags & Compute::NO_BACKGROUND)) {
		wait_threads (this, flags, std::move(p));
	} else {
		std::unique_lock<std::mutex> pool_wait_lock (_mtx);
		RQ_UNUSED(pool_wait_lock);
		pool_wait.emplace_back(wait_threads, this, flags, std::move(p));
	}
	return future;
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::wait_threads (Decoder<In_It, Fwd_It> *obj,
									const Compute flags,
									std::promise<std::pair<Error, uint8_t>> p)
{
	do {
		if (obj->exiting) {	// make sure we can exit
			p.set_value ({Error::EXITING, 0});
			break;
		}
		// pool is global (static), so wait only for our stuff.
		std::unique_lock<std::mutex> lock (obj->pool_lock->first);
		if (obj->exiting) { // make sure we can exit
			p.set_value ({Error::EXITING, 0});
			break;
		}
		auto status = obj->get_report (flags);
		if (Error::WORKING != status.first) {
			p.set_value (status);
			break;
		}

		obj->pool_lock->second.wait (lock); // conditional wait
		if (obj->exiting) {	// make sure we can exit
			p.set_value ({Error::EXITING, 0});
			break;
		}
		status = obj->get_report (flags);
		lock.unlock();
		if (Error::WORKING != status.first) {
			p.set_value (status);
			break;
		}
	} while (true);

	// delete ourselves from the waiting thread vector.
	std::unique_lock<std::mutex> lock (obj->_mtx);
	RQ_UNUSED (lock);
	for (auto it = obj->pool_wait.begin(); it != obj->pool_wait.end(); ++it) {
		if (it->get_id() == std::this_thread::get_id()) {
			it->detach();
			obj->pool_wait.erase (it);
			break;
		}
	}
	obj->pool_lock->second.notify_all();
}

template <typename In_It, typename Fwd_It>
std::pair<Error, uint8_t> Decoder<In_It, Fwd_It>::get_report (
														const Compute flags)
{
	if (Compute::COMPLETE == (flags & Compute::COMPLETE) ||
			Compute::PARTIAL_FROM_BEGINNING ==
									(flags & Compute::PARTIAL_FROM_BEGINNING)) {
		auto it = decoders.begin();
		// get first non-reported block.
		for (;it != decoders.end(); ++it) {
			if (pool_last_reported <= it->first)
				break;
		}
		uint16_t reportable = 0;
		// get last reportable block
		for (; it != decoders.end(); ++it) {
			auto ptr = it->second.dec;
			if (ptr != nullptr && !ptr->ready())
				break;
			++reportable;
		}
		if (reportable > 0) {
			pool_last_reported += reportable;
			if (Compute::PARTIAL_FROM_BEGINNING ==
									(flags & Compute::PARTIAL_FROM_BEGINNING)) {
				return {Error::NONE, pool_last_reported};
			} else {
				// complete
				if (pool_last_reported == _blocks)
					return {Error::NONE, pool_last_reported};
			}
		}
	} else if (Compute::PARTIAL_ANY == (flags & Compute::PARTIAL_ANY)) {
		for (auto &it : decoders) {
			if (!it.second.reported) {
				auto ptr = it.second.dec;
				if (ptr != nullptr && ptr->ready()) {
					return {Error::NONE, it.first};
				}
			}
		}
	}
	// can be reached if computing thread was stopped
	return {Error::WORKING, 0};
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::decode_bytes (Fwd_It &start, const Fwd_It end,
														const uint8_t skip)
{
	// Decode from the beginning, up untill we can.
	// return number of BYTES written, starting at "start + skip" bytes
	//
	// in case the last iterator is only half written, "start" will
	// point to the half-written iterator.

	uint64_t written = 0;
	uint8_t new_skip = skip;
	for (uint8_t sbn = 0; sbn < blocks(); ++sbn) {
		std::unique_lock<std::mutex> block_lock (_mtx);
		auto it = decoders.find (sbn);
		if (it == decoders.end())
			return written;
		auto dec_ptr = it->second.dec;
		block_lock.unlock();

		if (!dec_ptr->ready()) {
			if (!use_pool && dec_ptr->can_decode()) {
				RaptorQ__v1::Work_State state =
										RaptorQ__v1::Work_State::KEEP_WORKING;
				auto ret = dec_ptr->decode (&state);
				if (RaptorQ__v1::Impl::Raw_Decoder<In_It>::
											Decoder_Result::DECODED != ret) {
					return written;
				}
			} else {
				return written;
			}
		}

		Impl::De_Interleaver<Fwd_It> de_interleaving (dec_ptr->get_symbols(),
													_sub_blocks, _alignment);

		uint64_t max_bytes = block_size (sbn);
		if (sbn == (blocks() - 1)) {
			// size of the data (_size) is different from the sum of the size of
			// all blocks. get the real size, so we do not write more.
			// we obviously need to consider this only for the last block.
			uint64_t all_blocks = 0;
			for (uint8_t id = 0; id < blocks(); ++id)
				all_blocks += block_size (sbn);
			const uint64_t diff = all_blocks - _size;
			max_bytes -= diff;
		}
		auto tmp_start = start;
		uint64_t bytes_written = de_interleaving (tmp_start, end, max_bytes,
																	new_skip);
		written += bytes_written;
		uint64_t bytes_and_skip = new_skip + bytes_written;
		new_skip = bytes_and_skip %
					sizeof(typename std::iterator_traits<Fwd_It>::value_type);
		if (bytes_written == 0)
			return written;
		//new_skip = block_size (sbn) %
		//			sizeof(typename std::iterator_traits<Fwd_It>::value_type);
		// if we ended decoding in the middle of a Fwd_It, do not advance
		// start too much, or we will end up having additional zeros.
		if (new_skip == 0) {
			start = tmp_start;
		} else {
			uint64_t it_written = bytes_and_skip /
					sizeof(typename std::iterator_traits<Fwd_It>::value_type);
			// RaptorQ handles at most 881GB per rfc, so
			// casting uint64 to int64 is safe
			// we can not do "--start" since it's a forward iterator
			start += std::max (static_cast<int64_t> (0),
										static_cast<int64_t> (it_written - 1));
		}
	}
	return written;
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::decode_block_bytes (Fwd_It &start,
															const Fwd_It end,
															const uint8_t skip,
															const uint8_t sbn)
{
	if (sbn >= _blocks)
		return 0;

	std::shared_ptr<RaptorQ__v1::Impl::Raw_Decoder<In_It>> dec_ptr = nullptr;
	std::unique_lock<std::mutex> lock (_mtx);
	auto it = decoders.find (sbn);

	if (it == decoders.end())
		return 0;	// did not receiveany data yet.

	if (use_pool) {
		dec_ptr = it->second.dec;
		lock.unlock();
		if (!dec_ptr->ready())
			return 0;	// did not receive enough data, or could not decode yet.
	} else {
		dec_ptr = it->second.dec;
		lock.unlock();
		if (!dec_ptr->ready()) {
			if (!dec_ptr->can_decode())
				return 0;
			RaptorQ__v1::Work_State keep_working =
										RaptorQ__v1::Work_State::KEEP_WORKING;
			dec_ptr->decode (&keep_working);
			if (!dec_ptr->ready())
				return 0;
		}
	}
	// decoder has decoded the block

	Impl::De_Interleaver<Fwd_It> de_interleaving (dec_ptr->get_symbols(),
													_sub_blocks, _alignment);
	uint64_t max_bytes = block_size (sbn);
	if (sbn == (blocks() - 1)) {
		// size of the data (_size) is different from the sum of the size of
		// all blocks. get the real size, so we do not write more.
		// we obviously need to consider this only for the last block.
		uint64_t all_blocks = 0;
		for (uint8_t id = 0; id < blocks(); ++id)
			all_blocks += block_size (sbn);
		const uint64_t diff = all_blocks - _size;
		max_bytes -= diff;
	}
	return de_interleaving (start, end, max_bytes, skip);
}

template <typename In_It, typename Fwd_It>
std::pair<size_t, uint8_t> Decoder<In_It, Fwd_It>::decode_aligned (
														Fwd_It &start,
														const Fwd_It end,
														const uint8_t skip)
{
	const uint64_t bytes = decode_bytes (start, end, skip);
	const uint64_t skip_and_bytes = skip + bytes;
	const uint64_t iterators = skip_and_bytes /
					sizeof(typename std::iterator_traits<Fwd_It>::value_type);
	const uint8_t new_skip = skip_and_bytes %
					sizeof(typename std::iterator_traits<Fwd_It>::value_type);
	return {iterators, new_skip};
}

template <typename In_It, typename Fwd_It>
std::pair<size_t, uint8_t> Decoder<In_It, Fwd_It>::decode_block_aligned (
														Fwd_It &start,
														const Fwd_It end,
														const uint8_t skip,
														const uint8_t sbn)
{
	const uint64_t bytes = decode_block_bytes (start, end, skip, sbn);
	const uint64_t skip_and_bytes = skip + bytes;
	const uint64_t iterators = skip_and_bytes /
					sizeof(typename std::iterator_traits<Fwd_It>::value_type);
	const uint8_t new_skip = skip_and_bytes %
					sizeof(typename std::iterator_traits<Fwd_It>::value_type);
	return {iterators, new_skip};
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

}   // namespace Impl
}	// namespace RFC6330__v1

