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

#include "RaptorQ/v1/RFC.hpp"
#include "RaptorQ/v1/API_Wrapper.h"
#include "RaptorQ/v1/API_Wrapper.hpp"
#include <chrono>
#include <future>
#include <memory>

struct RAPTORQ_LOCAL RaptorQ_ptr
{
	void *ptr;
	const RaptorQ_type type;

	RaptorQ_ptr (const RaptorQ_type _type) : ptr (nullptr), type (_type) {}
};

struct RAPTORQ_LOCAL RaptorQ_future
{
	std::future<std::pair<RFC6330__v1::Error, uint8_t>> f;
};


struct RaptorQ_ptr *RaptorQ_Enc (const RaptorQ_type type, void *data,
											const uint64_t size,
											const uint16_t min_subsymbol_size,
											const uint16_t symbol_size,
											const size_t max_memory)
{
	std::unique_ptr<RaptorQ_ptr> ret (new RaptorQ_ptr (type));

	switch (type) {
	case RaptorQ_type::RQ_ENC_8:
		ret->ptr = reinterpret_cast<void *> (
					new RFC6330__v1::Encoder<uint8_t*, uint8_t*> (
									reinterpret_cast<uint8_t*> (data),
									reinterpret_cast<uint8_t*> (data) + size,
									min_subsymbol_size,
									symbol_size,
									max_memory));
		break;
	case RaptorQ_type::RQ_ENC_16:
		ret->ptr = reinterpret_cast<void *> (
					new RFC6330__v1::Encoder<uint16_t*, uint16_t*> (
									reinterpret_cast<uint16_t*> (data),
									reinterpret_cast<uint16_t*> (data) + size,
									min_subsymbol_size,
									symbol_size,
									max_memory));
		break;
	case RaptorQ_type::RQ_ENC_32:
		ret->ptr = reinterpret_cast<void *> (
					new RFC6330__v1::Encoder<uint32_t*, uint32_t*> (
									reinterpret_cast<uint32_t*> (data),
									reinterpret_cast<uint32_t*> (data) + size,
									min_subsymbol_size,
									symbol_size,
									max_memory));
		break;
	case RaptorQ_type::RQ_ENC_64:
		ret->ptr = reinterpret_cast<void *> (
					new RFC6330__v1::Encoder<uint64_t*, uint64_t*> (
									reinterpret_cast<uint64_t*> (data),
									reinterpret_cast<uint64_t*> (data) + size,
									min_subsymbol_size,
									symbol_size,
									max_memory));
		break;
	case RaptorQ_type::RQ_DEC_8:
	case RaptorQ_type::RQ_DEC_16:
	case RaptorQ_type::RQ_DEC_32:
	case RaptorQ_type::RQ_DEC_64:
	case RaptorQ_type::RQ_NONE:
		return new RaptorQ_ptr (RaptorQ_type::RQ_NONE);
	}
	return ret.release();
}

struct RaptorQ_ptr *RaptorQ_Dec (const RaptorQ_type type,
							const RaptorQ_OTI_Common_Data common,
							const RaptorQ_OTI_Scheme_Specific_Data scheme)
{
	std::unique_ptr<RaptorQ_ptr> ret (new RaptorQ_ptr (type));

	switch (type) {
	case RaptorQ_type::RQ_DEC_8:
		ret->ptr = reinterpret_cast<void *> (
				new RFC6330__v1::Decoder<uint8_t*, uint8_t*> (common, scheme));
		break;
	case RaptorQ_type::RQ_DEC_16:
		ret->ptr = reinterpret_cast<void *> (
				new RFC6330__v1::Decoder<uint16_t*, uint16_t*> (common,scheme));
		break;
	case RaptorQ_type::RQ_DEC_32:
		ret->ptr = reinterpret_cast<void *> (
				new RFC6330__v1::Decoder<uint32_t*, uint32_t*> (common,scheme));
		break;
	case RaptorQ_type::RQ_DEC_64:
		ret->ptr = reinterpret_cast<void *> (
				new RFC6330__v1::Decoder<uint64_t*, uint64_t*> (common,scheme));
		break;
	case RaptorQ_type::RQ_ENC_8:
	case RaptorQ_type::RQ_ENC_16:
	case RaptorQ_type::RQ_ENC_32:
	case RaptorQ_type::RQ_ENC_64:
	case RaptorQ_type::RQ_NONE:
		return new RaptorQ_ptr (RaptorQ_type::RQ_NONE);
	}
	return ret.release();
}

///////////////////////////
// Precomputation caching
///////////////////////////

uint64_t RaptorQ_shared_cache_size (const uint64_t shared_cache)
{
	return RFC6330__v1::shared_cache_size (shared_cache);
}

RaptorQ_Error RaptorQ_local_cache_size (const uint64_t local_cache)
{
	return static_cast<RaptorQ_Error> (RFC6330__v1::local_cache_size (
																local_cache));
}

uint64_t RaptorQ_get_shared_cache_size ()
{
	return RFC6330__v1::get_shared_cache_size();
}

uint64_t RaptorQ_get_local_cache_size ()
{
	return RFC6330__v1::get_local_cache_size();
}

/////////////////////
// Common functions
/////////////////////


bool RaptorQ_set_thread_pool (const size_t threads,
										const uint16_t max_block_concurrency,
										const RaptorQ_Work exit_type)
{
	return RFC6330__v1::set_thread_pool (threads, max_block_concurrency,
							static_cast<RaptorQ__v1::Work_State> (exit_type));
}

RaptorQ_Error RaptorQ_future_valid (struct RaptorQ_future *future)
{
	if (future == nullptr)
		return RQ_ERR_WRONG_INPUT;
	if (future->f.valid())
		return static_cast<RaptorQ_Error> (future->f.get().first);
	return RQ_ERR_WORKING;
}

RaptorQ_Error RaptorQ_future_wait_for (struct RaptorQ_future *future,
												const uint64_t time,
												const RaptorQ_Unit_Time unit)
{
	if (future == nullptr)
		return RQ_ERR_WRONG_INPUT;
	std::future_status status = std::future_status::timeout;
	switch (unit) {
	case RQ_TIME_NANOSEC:
		status = future->f.wait_for (std::chrono::nanoseconds (time));
		break;
	case RQ_TIME_MICROSEC:
		status = future->f.wait_for (std::chrono::microseconds (time));
		break;
	case RQ_TIME_MILLISEC:
		status = future->f.wait_for (std::chrono::milliseconds (time));
		break;
	case RQ_TIME_SEC:
		status = future->f.wait_for (std::chrono::seconds (time));
		break;
	case RQ_TIME_MIN:
		status = future->f.wait_for (std::chrono::minutes (time));
		break;
	case RQ_TIME_HOUR:
		status = future->f.wait_for (std::chrono::hours (time));
		break;
	}
	if (status == std::future_status::ready)
		return static_cast<RaptorQ_Error> (future->f.get().first);
	return RQ_ERR_WORKING;
}

void RaptorQ_future_wait (struct RaptorQ_future *future)
{
	if (future == nullptr)
		return;
	future->f.wait();
}

struct RaptorQ_Result RaptorQ_future_get (struct RaptorQ_future *future)
{
	RaptorQ_Result res = {RQ_ERR_WRONG_INPUT, 0};
	if (future == nullptr)
		return res;
	RFC6330__v1::Error err;
	std::tie (err, res.sbn) = future->f.get(); // already calls wait();
	res.error = static_cast<RaptorQ_Error> (err);
	return res;
}

void RaptorQ_future_free (struct RaptorQ_future **future)
{
	if (future == nullptr || *future == nullptr)
		return;
	delete *future;
	*future = nullptr;
}

RaptorQ_future* RaptorQ_compute (RaptorQ_ptr *ptr, const RaptorQ_Compute flags)
{
	if (ptr == nullptr || ptr->type == RaptorQ_type::RQ_NONE ||
															ptr->ptr == nullptr)
		return nullptr;
	const RFC6330__v1::Compute cpp_flags =
									static_cast<RFC6330__v1::Compute> (flags);
	std::future<std::pair<RaptorQ__v1::Error, uint8_t>> f;
	switch (ptr->type) {
	case RaptorQ_type::RQ_ENC_8:
		f = (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
											ptr->ptr))->compute (cpp_flags);
		break;
	case RaptorQ_type::RQ_ENC_16:
		f = (reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
											ptr->ptr))->compute (cpp_flags);
		break;
	case RaptorQ_type::RQ_ENC_32:
		f = (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
											ptr->ptr))->compute (cpp_flags);
		break;
	case RaptorQ_type::RQ_ENC_64:
		f = (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
											ptr->ptr))->compute (cpp_flags);
		break;
	case RaptorQ_type::RQ_DEC_8:
		f = (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
											ptr->ptr))->compute (cpp_flags);
		break;
	case RaptorQ_type::RQ_DEC_16:
		f = (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
											ptr->ptr))->compute (cpp_flags);
		break;
	case RaptorQ_type::RQ_DEC_32:
		f = (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
											ptr->ptr))->compute (cpp_flags);
		break;
	case RaptorQ_type::RQ_DEC_64:
		f = (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
											ptr->ptr))->compute (cpp_flags);
		break;
	case RaptorQ_type::RQ_NONE:
		return nullptr;
	}
	RaptorQ_future *ret = new RaptorQ_future();
	ret->f = std::move(f);
	return ret;
}

///////////
// Encoding
///////////

RaptorQ_OTI_Common_Data RaptorQ_OTI_Common (struct RaptorQ_ptr *enc)
{
	if (enc == nullptr || enc->type == RaptorQ_type::RQ_NONE ||
															enc->ptr == nullptr)
		return 0;
	switch (enc->type) {
	case RaptorQ_type::RQ_ENC_8:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
													enc->ptr))->OTI_Common();
	case RaptorQ_type::RQ_ENC_16:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
													enc->ptr))->OTI_Common();
	case RaptorQ_type::RQ_ENC_32:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
													enc->ptr))->OTI_Common();
	case RaptorQ_type::RQ_ENC_64:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
													enc->ptr))->OTI_Common();
	case RaptorQ_type::RQ_DEC_8:
	case RaptorQ_type::RQ_DEC_16:
	case RaptorQ_type::RQ_DEC_32:
	case RaptorQ_type::RQ_DEC_64:
	case RaptorQ_type::RQ_NONE:
		return 0;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return 0;
#endif
}

RaptorQ_OTI_Scheme_Specific_Data RaptorQ_OTI_Scheme (struct RaptorQ_ptr *enc)
{
	if (enc == nullptr || enc->type == RaptorQ_type::RQ_NONE ||
															enc->ptr == nullptr)
		return 0;
	switch (enc->type) {
	case RaptorQ_type::RQ_ENC_8:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
											enc->ptr))->OTI_Scheme_Specific();
	case RaptorQ_type::RQ_ENC_16:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
											enc->ptr))->OTI_Scheme_Specific();
	case RaptorQ_type::RQ_ENC_32:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
											enc->ptr))->OTI_Scheme_Specific();
	case RaptorQ_type::RQ_ENC_64:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
											enc->ptr))->OTI_Scheme_Specific();
	case RaptorQ_type::RQ_DEC_8:
	case RaptorQ_type::RQ_DEC_16:
	case RaptorQ_type::RQ_DEC_32:
	case RaptorQ_type::RQ_DEC_64:
	case RaptorQ_type::RQ_NONE:
		return 0;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return 0;
#endif
}

uint16_t RaptorQ_symbol_size (RaptorQ_ptr *ptr) {
	if (ptr == nullptr || ptr->type == RaptorQ_type::RQ_NONE ||
															ptr->ptr == nullptr)
		return 0;
	switch (ptr->type) {
	case RaptorQ_type::RQ_ENC_8:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::RQ_ENC_16:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::RQ_ENC_32:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::RQ_ENC_64:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::RQ_DEC_8:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::RQ_DEC_16:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::RQ_DEC_32:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::RQ_DEC_64:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::RQ_NONE:
		return 0;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return 0;
#endif
}
uint8_t RaptorQ_blocks (RaptorQ_ptr *ptr)
{
	if (ptr == nullptr || ptr->type == RaptorQ_type::RQ_NONE ||
															ptr->ptr == nullptr)
		return 0;
	switch (ptr->type) {
	case RaptorQ_type::RQ_ENC_8:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::RQ_ENC_16:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint16_t, uint16_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::RQ_ENC_32:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::RQ_ENC_64:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::RQ_DEC_8:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::RQ_DEC_16:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::RQ_DEC_32:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::RQ_DEC_64:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::RQ_NONE:
		return 0;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return 0;
#endif
}
uint32_t RaptorQ_block_size (RaptorQ_ptr *ptr, const uint8_t sbn)
{
	if (ptr == nullptr || ptr->type == RaptorQ_type::RQ_NONE ||
															ptr->ptr == nullptr)
		return 0;
	switch (ptr->type) {
	case RaptorQ_type::RQ_ENC_8:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::RQ_ENC_16:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::RQ_ENC_32:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::RQ_ENC_64:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::RQ_DEC_8:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::RQ_DEC_16:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::RQ_DEC_32:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::RQ_DEC_64:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::RQ_NONE:
		return 0;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return 0;
#endif
}

uint16_t RaptorQ_symbols (RaptorQ_ptr *ptr, const uint8_t sbn)
{
	if (ptr == nullptr || ptr->type == RaptorQ_type::RQ_NONE ||
															ptr->ptr == nullptr)
		return 0;
	switch (ptr->type) {
	case RaptorQ_type::RQ_ENC_8:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::RQ_ENC_16:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::RQ_ENC_32:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::RQ_ENC_64:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::RQ_DEC_8:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::RQ_DEC_16:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::RQ_DEC_32:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::RQ_DEC_64:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::RQ_NONE:
		return 0;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return 0;
#endif
}

uint32_t RaptorQ_max_repair (RaptorQ_ptr *enc, const uint8_t sbn)
{
	if (enc == nullptr || enc->type == RaptorQ_type::RQ_NONE ||
															enc->ptr == nullptr)
		return 0;
	switch (enc->type) {
	case RaptorQ_type::RQ_ENC_8:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
												enc->ptr))->max_repair (sbn);
	case RaptorQ_type::RQ_ENC_16:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
												enc->ptr))->max_repair (sbn);
	case RaptorQ_type::RQ_ENC_32:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
												enc->ptr))->max_repair (sbn);
	case RaptorQ_type::RQ_ENC_64:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
												enc->ptr))->max_repair (sbn);
	case RaptorQ_type::RQ_DEC_8:
	case RaptorQ_type::RQ_DEC_16:
	case RaptorQ_type::RQ_DEC_32:
	case RaptorQ_type::RQ_DEC_64:
	case RaptorQ_type::RQ_NONE:
		return 0;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return 0;
#endif
}

size_t RaptorQ_precompute_max_memory (RaptorQ_ptr *enc)
{
	if (enc == nullptr || enc->type == RaptorQ_type::RQ_NONE ||
															enc->ptr == nullptr)
		return 0;
	switch (enc->type) {
	case RaptorQ_type::RQ_ENC_8:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
											enc->ptr))->precompute_max_memory();
	case RaptorQ_type::RQ_ENC_16:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
											enc->ptr))->precompute_max_memory();
	case RaptorQ_type::RQ_ENC_32:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
											enc->ptr))->precompute_max_memory();
	case RaptorQ_type::RQ_ENC_64:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
											enc->ptr))->precompute_max_memory();
	case RaptorQ_type::RQ_DEC_8:
	case RaptorQ_type::RQ_DEC_16:
	case RaptorQ_type::RQ_DEC_32:
	case RaptorQ_type::RQ_DEC_64:
	case RaptorQ_type::RQ_NONE:
		return 0;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return 0;
#endif
}

uint64_t RaptorQ_encode_id (RaptorQ_ptr *enc, void **data, const uint64_t size,
															const uint32_t id)
{
	uint8_t sbn = id >> 24;
	uint32_t esi = (id << 8) >> 8;
	return RaptorQ_encode (enc, data, size, esi, sbn);
}

uint64_t RaptorQ_encode (RaptorQ_ptr *enc, void **data, const uint64_t size,
															const uint32_t esi,
															const uint8_t sbn)
{
	if (enc == nullptr || enc->type == RaptorQ_type::RQ_NONE ||
				enc->ptr == nullptr || data == nullptr || *data == nullptr) {
		return 0;
	}
	// uhm... better ideas?
	uint8_t *p_8;
	uint16_t *p_16;
	uint32_t *p_32;
	uint64_t *p_64;
	switch (enc->type) {
	case RaptorQ_type::RQ_ENC_8:
		p_8 = reinterpret_cast<uint8_t*> (*data);
		return (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
								enc->ptr))->encode (p_8, p_8 + size,esi, sbn);
	case RaptorQ_type::RQ_ENC_16:
		p_16 = reinterpret_cast<uint16_t*> (*data);
		return (reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
								enc->ptr))->encode (p_16, p_16 + size,esi, sbn);
	case RaptorQ_type::RQ_ENC_32:
		p_32 = reinterpret_cast<uint32_t*> (*data);
		return (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
								enc->ptr))->encode (p_32, p_32 + size,esi, sbn);
	case RaptorQ_type::RQ_ENC_64:
		p_64 = reinterpret_cast<uint64_t*> (*data);
		return (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
								enc->ptr))->encode (p_64, p_64 + size,esi, sbn);
	case RaptorQ_type::RQ_DEC_8:
	case RaptorQ_type::RQ_DEC_16:
	case RaptorQ_type::RQ_DEC_32:
	case RaptorQ_type::RQ_DEC_64:
	case RaptorQ_type::RQ_NONE:
		return 0;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return 0;
#endif
}

uint32_t RaptorQ_id (const uint32_t esi, const uint8_t sbn)
{
	uint32_t ret = static_cast<uint32_t> (sbn) << 24;
	ret += esi % static_cast<uint32_t> (std::pow (2, 24));
	return ret;
}


///////////
// Decoding
///////////

uint64_t RAPTORQ_API RaptorQ_bytes (struct RaptorQ_ptr *dec)
{
	if (dec == nullptr || dec->type == RaptorQ_type::RQ_NONE)
		return 0;

	switch (dec->type) {
	case RaptorQ_type::RQ_DEC_8:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
														dec->ptr))->bytes ();
	case RaptorQ_type::RQ_DEC_16:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
														dec->ptr))->bytes ();
	case RaptorQ_type::RQ_DEC_32:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
														dec->ptr))->bytes ();
	case RaptorQ_type::RQ_DEC_64:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
														dec->ptr))->bytes ();
	case RaptorQ_type::RQ_ENC_8:
	case RaptorQ_type::RQ_ENC_16:
	case RaptorQ_type::RQ_ENC_32:
	case RaptorQ_type::RQ_ENC_64:
	case RaptorQ_type::RQ_NONE:
		return 0;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return 0;
#endif

}

uint64_t RaptorQ_decode_bytes (RaptorQ_ptr *dec, void **data,
										const size_t size, const uint8_t skip)
{
	uint64_t written = 0;
	if (dec == nullptr || dec->type == RaptorQ_type::RQ_NONE ||
						dec->ptr == nullptr || data == nullptr || size <= skip){
		return written;
	}
	uint8_t *p_8;
	uint16_t *p_16;
	uint32_t *p_32;
	uint64_t *p_64;
	switch (dec->type) {
	case RaptorQ_type::RQ_DEC_8:
		p_8 = reinterpret_cast<uint8_t*> (*data);
		written = (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
								dec->ptr))->decode_bytes (p_8,  p_8  + size, skip);
		*data = p_8;
		break;
	case RaptorQ_type::RQ_DEC_16:
		p_16 = reinterpret_cast<uint16_t*> (*data);
		written = (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
								dec->ptr))->decode_bytes (p_16, p_16 + size, skip);
		*data = p_16;
		break;
	case RaptorQ_type::RQ_DEC_32:
		p_32 = reinterpret_cast<uint32_t*> (*data);
		written = (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
								dec->ptr))->decode_bytes (p_32, p_32 + size, skip);
		*data = p_32;
		break;
	case RaptorQ_type::RQ_DEC_64:
		p_64 = reinterpret_cast<uint64_t*> (*data);
		written = (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
								dec->ptr))->decode_bytes (p_64, p_64 + size, skip);
		*data = p_64;
		break;
	case RaptorQ_type::RQ_ENC_8:
	case RaptorQ_type::RQ_ENC_16:
	case RaptorQ_type::RQ_ENC_32:
	case RaptorQ_type::RQ_ENC_64:
	case RaptorQ_type::RQ_NONE:
		break;
	}
	return written;
}

uint64_t RaptorQ_decode_block_bytes (RaptorQ_ptr *dec, void **data,
															const size_t size,
															const uint8_t skip,
															const uint8_t sbn)
{
	uint64_t written = 0;
	if (dec == nullptr || dec->type == RaptorQ_type::RQ_NONE ||
									dec->ptr == nullptr || data == nullptr) {
		return written;
	}
	uint8_t *p_8;
	uint16_t *p_16;
	uint32_t *p_32;
	uint64_t *p_64;
	switch (dec->type) {
	case RaptorQ_type::RQ_DEC_8:
		p_8 = reinterpret_cast<uint8_t*> (*data);
		written = (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
							dec->ptr))->decode_block_bytes (p_8, p_8 + size,
																	skip, sbn);
		*data = p_8;
		break;
	case RaptorQ_type::RQ_DEC_16:
		p_16 = reinterpret_cast<uint16_t*> (*data);
		written = (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
							dec->ptr))->decode_block_bytes (p_16, p_16 + size,
																	skip, sbn);
		*data = p_16;
		break;
	case RaptorQ_type::RQ_DEC_32:
		p_32 = reinterpret_cast<uint32_t*> (*data);
		written = (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
							dec->ptr))->decode_block_bytes (p_32, p_32 + size,
																	skip, sbn);
		*data = p_32;
		break;
	case RaptorQ_type::RQ_DEC_64:
		p_64 = reinterpret_cast<uint64_t*> (*data);
		written = (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
							dec->ptr))->decode_block_bytes (p_64, p_64 + size,
																	skip, sbn);
		*data = p_64;
		break;
	case RaptorQ_type::RQ_ENC_8:
	case RaptorQ_type::RQ_ENC_16:
	case RaptorQ_type::RQ_ENC_32:
	case RaptorQ_type::RQ_ENC_64:
	case RaptorQ_type::RQ_NONE:
		break;
	}
	return written;
}

RaptorQ_Dec_Result RaptorQ_decode_aligned (RaptorQ_ptr *dec, void **data,
										const size_t size, const uint8_t skip)
{
	RaptorQ_Dec_Result c_ret = {0, 0};
	if (dec == nullptr || dec->type == RaptorQ_type::RQ_NONE ||
						dec->ptr == nullptr || data == nullptr || size <= skip){
		return c_ret;
	}
	uint8_t *p_8;
	uint16_t *p_16;
	uint32_t *p_32;
	uint64_t *p_64;
	std::pair<size_t, uint8_t> ret = {0, 0};
	switch (dec->type) {
	case RaptorQ_type::RQ_DEC_8:
		p_8 = reinterpret_cast<uint8_t*> (*data);
		ret = (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
												dec->ptr))->decode_aligned (
													p_8,  p_8  + size, skip);
		*data = p_8;
		break;
	case RaptorQ_type::RQ_DEC_16:
		p_16 = reinterpret_cast<uint16_t*> (*data);
		ret = (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
												dec->ptr))->decode_aligned (
													p_16, p_16 + size, skip);
		*data = p_16;
		break;
	case RaptorQ_type::RQ_DEC_32:
		p_32 = reinterpret_cast<uint32_t*> (*data);
		ret = (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
												dec->ptr))->decode_aligned (
													p_32, p_32 + size, skip);
		*data = p_32;
		break;
	case RaptorQ_type::RQ_DEC_64:
		p_64 = reinterpret_cast<uint64_t*> (*data);
		ret = (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
												dec->ptr))->decode_aligned (
													p_64, p_64 + size, skip);
		*data = p_64;
		break;
	case RaptorQ_type::RQ_ENC_8:
	case RaptorQ_type::RQ_ENC_16:
	case RaptorQ_type::RQ_ENC_32:
	case RaptorQ_type::RQ_ENC_64:
	case RaptorQ_type::RQ_NONE:
		break;
	}
	c_ret.written = std::get<0> (ret);
	c_ret.skip = std::get<1> (ret);
	return c_ret;
}

RaptorQ_Dec_Result RaptorQ_decode_block_aligned (RaptorQ_ptr *dec, void **data,
															const size_t size,
															const uint8_t skip,
															const uint8_t sbn)
{
	RaptorQ_Dec_Result c_ret = {0, 0};
	if (dec == nullptr || dec->type == RaptorQ_type::RQ_NONE ||
									dec->ptr == nullptr || data == nullptr) {
		return c_ret;
	}
	uint8_t *p_8;
	uint16_t *p_16;
	uint32_t *p_32;
	uint64_t *p_64;
	std::pair<size_t, uint8_t> ret = {0, 0};
	switch (dec->type) {
	case RaptorQ_type::RQ_DEC_8:
		p_8 = reinterpret_cast<uint8_t*> (*data);
		ret = (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
							dec->ptr))->decode_block_aligned (p_8, p_8 + size,
																	skip, sbn);
		*data = p_8;
		break;
	case RaptorQ_type::RQ_DEC_16:
		p_16 = reinterpret_cast<uint16_t*> (*data);
		ret = (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
							dec->ptr))->decode_block_aligned (p_16, p_16 + size,
																	skip, sbn);
		*data = p_16;
		break;
	case RaptorQ_type::RQ_DEC_32:
		p_32 = reinterpret_cast<uint32_t*> (*data);
		ret = (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
							dec->ptr))->decode_block_aligned (p_32, p_32 + size,
																	skip, sbn);
		*data = p_32;
		break;
	case RaptorQ_type::RQ_DEC_64:
		p_64 = reinterpret_cast<uint64_t*> (*data);
		ret = (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
							dec->ptr))->decode_block_aligned (p_64, p_64 + size,
																	skip, sbn);
		*data = p_64;
		break;
	case RaptorQ_type::RQ_ENC_8:
	case RaptorQ_type::RQ_ENC_16:
	case RaptorQ_type::RQ_ENC_32:
	case RaptorQ_type::RQ_ENC_64:
	case RaptorQ_type::RQ_NONE:
		break;
	}
	c_ret.written = std::get<0> (ret);
	c_ret.skip = std::get<1> (ret);
	return c_ret;
}

RaptorQ_Error RaptorQ_add_symbol_id (RaptorQ_ptr *dec, void **data,
															const uint32_t size,
															const uint32_t id)
{
	uint8_t sbn = id >> 24;
	uint32_t esi = (id << 8) >> 8;
	return RaptorQ_add_symbol (dec, data, size, esi, sbn);
}

RaptorQ_Error RaptorQ_add_symbol (RaptorQ_ptr *dec, void **data,
															const uint32_t size,
															const uint32_t esi,
															const uint8_t sbn)
{
	if (dec == nullptr || dec->type == RaptorQ_type::RQ_NONE ||
									dec->ptr == nullptr || data == nullptr) {
		return RaptorQ_Error::RQ_ERR_INITIALIZATION;
	}
	uint8_t *p_8;
	uint16_t *p_16;
	uint32_t *p_32;
	uint64_t *p_64;
	RFC6330__v1::Error error = RFC6330__v1::Error::WRONG_INPUT;
	switch (dec->type) {
	case RaptorQ_type::RQ_DEC_8:
		p_8 = reinterpret_cast<uint8_t*> (*data);
		error = reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
							dec->ptr)->add_symbol (p_8, p_8 + size, esi, sbn);
		break;
	case RaptorQ_type::RQ_DEC_16:
		p_16 = reinterpret_cast<uint16_t*> (*data);
		error = reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
						dec->ptr)->add_symbol (p_16, p_16 + size, esi, sbn);
		break;
	case RaptorQ_type::RQ_DEC_32:
		p_32 = reinterpret_cast<uint32_t*> (*data);
		error = reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
						dec->ptr)->add_symbol (p_32, p_32 + size, esi, sbn);
		break;
	case RaptorQ_type::RQ_DEC_64:
		p_64 = reinterpret_cast<uint64_t*> (*data);
		error = reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
						dec->ptr)->add_symbol (p_64, p_64 + size, esi, sbn);
		break;
	case RaptorQ_type::RQ_ENC_8:
	case RaptorQ_type::RQ_ENC_16:
	case RaptorQ_type::RQ_ENC_32:
	case RaptorQ_type::RQ_ENC_64:
	case RaptorQ_type::RQ_NONE:
		return RaptorQ_Error::RQ_ERR_INITIALIZATION;
	}
	return static_cast<RaptorQ_Error> (error);
}

///////////////////////
// General: free memory
///////////////////////

void RaptorQ_free (struct RaptorQ_ptr **ptr)
{

	std::unique_ptr<RaptorQ_ptr> uptr;
	if (ptr == nullptr || *ptr == nullptr ||
			(*ptr)->type == RaptorQ_type::RQ_NONE || (*ptr)->ptr == nullptr) {
		if (ptr != nullptr) {
			uptr = std::unique_ptr<RaptorQ_ptr> (*ptr);
			*ptr = nullptr;
			return;
		}
		return;
	}
	uptr = std::unique_ptr<RaptorQ_ptr> (*ptr);
	*ptr = nullptr;
	switch (uptr->type) {
	case RaptorQ_type::RQ_ENC_8:
		delete reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::RQ_ENC_16:
		delete reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::RQ_ENC_32:
		delete reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::RQ_ENC_64:
		delete reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::RQ_DEC_8:
		delete reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::RQ_DEC_16:
		delete reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::RQ_DEC_32:
		delete reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::RQ_DEC_64:
		delete reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::RQ_NONE:
		break;
	}
	uptr->ptr = nullptr;
	return;
}

void RaptorQ_free_block (struct RaptorQ_ptr *ptr, const uint8_t sbn)
{
	if (ptr == nullptr || ptr->type == RaptorQ_type::RQ_NONE ||
															ptr->ptr == nullptr)
		return;
	switch (ptr->type) {
	case RaptorQ_type::RQ_ENC_8:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint8_t*, uint8_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::RQ_ENC_16:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint16_t*, uint16_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::RQ_ENC_32:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint32_t*, uint32_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::RQ_ENC_64:
		return (reinterpret_cast<RFC6330__v1::Encoder<uint64_t*, uint64_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::RQ_DEC_8:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint8_t*, uint8_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::RQ_DEC_16:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint16_t*, uint16_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::RQ_DEC_32:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint32_t*, uint32_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::RQ_DEC_64:
		return (reinterpret_cast<RFC6330__v1::Decoder<uint64_t*, uint64_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::RQ_NONE:
		return;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return;
#endif
}
