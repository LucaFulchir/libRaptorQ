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

#include "cRaptorQ.h"
#include "RaptorQ.hpp"
#include <memory>

struct RAPTORQ_LOCAL RaptorQ_ptr
{
	void *ptr;
	const RaptorQ_type type;

	RaptorQ_ptr (const RaptorQ_type _type) : ptr (nullptr), type (_type) {}
};

void RAPTORQ_LOCAL RaptorQ_test_del (struct RaptorQ_ptr **ptr);
void RAPTORQ_LOCAL RaptorQ_test_del (struct RaptorQ_ptr **ptr)
{
	if (ptr == nullptr || *ptr == nullptr || (*ptr)->type == RaptorQ_type::NONE
													|| (*ptr)->ptr == nullptr) {
		if (ptr != nullptr) {
			delete *ptr;
			*ptr = nullptr;
			return;
		}
		return;
	}
	switch ((*ptr)->type) {
	case RaptorQ_type::ENC_8:
		if(!(reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
																(*ptr)->ptr))) {
			RaptorQ_free (ptr);
			return;
		}
		break;
	case RaptorQ_type::ENC_16:
		if(!(reinterpret_cast<RaptorQ::Encoder<uint16_t, uint16_t*>*> (
																(*ptr)->ptr))) {
			RaptorQ_free (ptr);
			return;
		}
		break;
	case RaptorQ_type::ENC_32:
		if(!(reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
																(*ptr)->ptr))) {
			RaptorQ_free (ptr);
			return;
		}
		break;
	case RaptorQ_type::ENC_64:
		if(!(reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
																(*ptr)->ptr))) {
			RaptorQ_free (ptr);
			return;
		}
		break;
	case RaptorQ_type::DEC_8:
		if(!(reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
																(*ptr)->ptr))) {
			RaptorQ_free (ptr);
			return;
		}
		break;
	case RaptorQ_type::DEC_16:
		if(!(reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
																(*ptr)->ptr))) {
			RaptorQ_free (ptr);
			return;
		}
		break;
	case RaptorQ_type::DEC_32:
		if(!(reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
																(*ptr)->ptr))) {
			RaptorQ_free (ptr);
			return;
		}
		break;
	case RaptorQ_type::DEC_64:
		if(!(reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
																(*ptr)->ptr))) {
			RaptorQ_free (ptr);
			return;
		}
		break;
	case RaptorQ_type::NONE:
		assert(false && "RaptorQ: C Wrapper: should not have gotten here");
		break;
	}
	return;
}


struct RaptorQ_ptr *RaptorQ_Enc (const RaptorQ_type type, void *data,
											const uint64_t size,
											const uint16_t min_subsymbol_size,
											const uint16_t symbol_size,
											const size_t max_memory)
{
	std::unique_ptr<RaptorQ_ptr> ret (new RaptorQ_ptr (type));

	switch (type) {
	case RaptorQ_type::ENC_8:
		ret->ptr = reinterpret_cast<void *> (
					new RaptorQ::Encoder<uint8_t*, uint8_t*> (
									reinterpret_cast<uint8_t*> (data),
									reinterpret_cast<uint8_t*> (data) + size,
									min_subsymbol_size,
									symbol_size,
									max_memory));
		break;
	case RaptorQ_type::ENC_16:
		ret->ptr = reinterpret_cast<void *> (
					new RaptorQ::Encoder<uint16_t*, uint16_t*> (
									reinterpret_cast<uint16_t*> (data),
									reinterpret_cast<uint16_t*> (data) + size,
									min_subsymbol_size,
									symbol_size,
									max_memory));
		break;
	case RaptorQ_type::ENC_32:
		ret->ptr = reinterpret_cast<void *> (
					new RaptorQ::Encoder<uint32_t*, uint32_t*> (
									reinterpret_cast<uint32_t*> (data),
									reinterpret_cast<uint32_t*> (data) + size,
									min_subsymbol_size,
									symbol_size,
									max_memory));
		break;
	case RaptorQ_type::ENC_64:
		ret->ptr = reinterpret_cast<void *> (
					new RaptorQ::Encoder<uint64_t*, uint64_t*> (
									reinterpret_cast<uint64_t*> (data),
									reinterpret_cast<uint64_t*> (data) + size,
									min_subsymbol_size,
									symbol_size,
									max_memory));
		break;
	case RaptorQ_type::DEC_8:
	case RaptorQ_type::DEC_16:
	case RaptorQ_type::DEC_32:
	case RaptorQ_type::DEC_64:
	case RaptorQ_type::NONE:
		return new RaptorQ_ptr (RaptorQ_type::NONE);
	}
	auto raw_ptr = ret.release();
	RaptorQ_test_del (&raw_ptr);
	return raw_ptr;
}

struct RaptorQ_ptr *RaptorQ_Dec (const RaptorQ_type type,
							const RaptorQ_OTI_Common_Data common,
							const RaptorQ_OTI_Scheme_Specific_Data scheme)
{
	std::unique_ptr<RaptorQ_ptr> ret (new RaptorQ_ptr (type));

	switch (type) {
	case RaptorQ_type::DEC_8:
		ret->ptr = reinterpret_cast<void *> (
					new RaptorQ::Decoder<uint8_t*, uint8_t*> (common, scheme));
		break;
	case RaptorQ_type::DEC_16:
		ret->ptr = reinterpret_cast<void *> (
					new RaptorQ::Decoder<uint16_t*, uint16_t*> (common,scheme));
		break;
	case RaptorQ_type::DEC_32:
		ret->ptr = reinterpret_cast<void *> (
					new RaptorQ::Decoder<uint32_t*, uint32_t*> (common,scheme));
		break;
	case RaptorQ_type::DEC_64:
		ret->ptr = reinterpret_cast<void *> (
					new RaptorQ::Decoder<uint64_t*, uint64_t*> (common,scheme));
		break;
	case RaptorQ_type::ENC_8:
	case RaptorQ_type::ENC_16:
	case RaptorQ_type::ENC_32:
	case RaptorQ_type::ENC_64:
	case RaptorQ_type::NONE:
		return new RaptorQ_ptr (RaptorQ_type::NONE);
	}
	auto raw_ptr = ret.release();
	RaptorQ_test_del (&raw_ptr);
	return raw_ptr;
}

///////////
// Encoding
///////////

RaptorQ_OTI_Common_Data RaptorQ_OTI_Common (struct RaptorQ_ptr *enc)
{
	if (enc == nullptr || enc->type == RaptorQ_type::NONE ||enc->ptr == nullptr)
		return 0;
	switch (enc->type) {
	case RaptorQ_type::ENC_8:
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
													enc->ptr))->OTI_Common();
	case RaptorQ_type::ENC_16:
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
													enc->ptr))->OTI_Common();
	case RaptorQ_type::ENC_32:
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
													enc->ptr))->OTI_Common();
	case RaptorQ_type::ENC_64:
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
													enc->ptr))->OTI_Common();
	case RaptorQ_type::DEC_8:
	case RaptorQ_type::DEC_16:
	case RaptorQ_type::DEC_32:
	case RaptorQ_type::DEC_64:
	case RaptorQ_type::NONE:
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
	if (enc == nullptr || enc->type == RaptorQ_type::NONE ||enc->ptr == nullptr)
		return 0;
	switch (enc->type) {
	case RaptorQ_type::ENC_8:
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
											enc->ptr))->OTI_Scheme_Specific();
	case RaptorQ_type::ENC_16:
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
											enc->ptr))->OTI_Scheme_Specific();
	case RaptorQ_type::ENC_32:
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
											enc->ptr))->OTI_Scheme_Specific();
	case RaptorQ_type::ENC_64:
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
											enc->ptr))->OTI_Scheme_Specific();
	case RaptorQ_type::DEC_8:
	case RaptorQ_type::DEC_16:
	case RaptorQ_type::DEC_32:
	case RaptorQ_type::DEC_64:
	case RaptorQ_type::NONE:
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
	if (ptr == nullptr || ptr->type == RaptorQ_type::NONE ||ptr->ptr == nullptr)
		return 0;
	switch (ptr->type) {
	case RaptorQ_type::ENC_8:
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::ENC_16:
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::ENC_32:
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::ENC_64:
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::DEC_8:
		return (reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::DEC_16:
		return (reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::DEC_32:
		return (reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::DEC_64:
		return (reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->symbol_size();
	case RaptorQ_type::NONE:
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
	if (ptr == nullptr || ptr->type == RaptorQ_type::NONE ||ptr->ptr == nullptr)
		return 0;
	switch (ptr->type) {
	case RaptorQ_type::ENC_8:
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::ENC_16:
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t, uint16_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::ENC_32:
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::ENC_64:
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::DEC_8:
		return (reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::DEC_16:
		return (reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::DEC_32:
		return (reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::DEC_64:
		return (reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
														ptr->ptr))->blocks();
	case RaptorQ_type::NONE:
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
	if (ptr == nullptr || ptr->type == RaptorQ_type::NONE ||ptr->ptr == nullptr)
		return 0;
	switch (ptr->type) {
	case RaptorQ_type::ENC_8:
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::ENC_16:
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::ENC_32:
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::ENC_64:
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::DEC_8:
		return (reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::DEC_16:
		return (reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::DEC_32:
		return (reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::DEC_64:
		return (reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->block_size(sbn);
	case RaptorQ_type::NONE:
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
	if (ptr == nullptr || ptr->type == RaptorQ_type::NONE ||ptr->ptr == nullptr)
		return 0;
	switch (ptr->type) {
	case RaptorQ_type::ENC_8:
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::ENC_16:
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::ENC_32:
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::ENC_64:
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::DEC_8:
		return (reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::DEC_16:
		return (reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::DEC_32:
		return (reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::DEC_64:
		return (reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
													ptr->ptr))->symbols(sbn);
	case RaptorQ_type::NONE:
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
	if (enc == nullptr || enc->type == RaptorQ_type::NONE ||enc->ptr == nullptr)
		return 0;
	switch (enc->type) {
	case RaptorQ_type::ENC_8:
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
												enc->ptr))->max_repair (sbn);
	case RaptorQ_type::ENC_16:
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
												enc->ptr))->max_repair (sbn);
	case RaptorQ_type::ENC_32:
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
												enc->ptr))->max_repair (sbn);
	case RaptorQ_type::ENC_64:
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
												enc->ptr))->max_repair (sbn);
	case RaptorQ_type::DEC_8:
	case RaptorQ_type::DEC_16:
	case RaptorQ_type::DEC_32:
	case RaptorQ_type::DEC_64:
	case RaptorQ_type::NONE:
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
	if (enc == nullptr || enc->type == RaptorQ_type::NONE ||enc->ptr == nullptr)
		return 0;
	switch (enc->type) {
	case RaptorQ_type::ENC_8:
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
											enc->ptr))->precompute_max_memory();
	case RaptorQ_type::ENC_16:
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
											enc->ptr))->precompute_max_memory();
	case RaptorQ_type::ENC_32:
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
											enc->ptr))->precompute_max_memory();
	case RaptorQ_type::ENC_64:
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
											enc->ptr))->precompute_max_memory();
	case RaptorQ_type::DEC_8:
	case RaptorQ_type::DEC_16:
	case RaptorQ_type::DEC_32:
	case RaptorQ_type::DEC_64:
	case RaptorQ_type::NONE:
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

void RaptorQ_precompute (RaptorQ_ptr *enc, const uint8_t threads,
														const bool background)
{
	if (enc == nullptr || enc->type == RaptorQ_type::NONE ||enc->ptr == nullptr)
		return;
	switch (enc->type) {
	case RaptorQ_type::ENC_8:
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
								enc->ptr))->precompute (threads, background);
	case RaptorQ_type::ENC_16:
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
								enc->ptr))->precompute (threads, background);
	case RaptorQ_type::ENC_32:
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
								enc->ptr))->precompute (threads, background);
	case RaptorQ_type::ENC_64:
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
								enc->ptr))->precompute (threads, background);
	case RaptorQ_type::DEC_8:
	case RaptorQ_type::DEC_16:
	case RaptorQ_type::DEC_32:
	case RaptorQ_type::DEC_64:
	case RaptorQ_type::NONE:
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
	if (enc == nullptr || enc->type == RaptorQ_type::NONE ||
				enc->ptr == nullptr || data == nullptr || *data == nullptr) {
		return 0;
	}
	// uhm... better ideas?
	uint8_t *p_8;
	uint16_t *p_16;
	uint32_t *p_32;
	uint64_t *p_64;
	switch (enc->type) {
	case RaptorQ_type::ENC_8:
		p_8 = reinterpret_cast<uint8_t*> (*data);
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
								enc->ptr))->encode (p_8, p_8 + size,esi, sbn);
	case RaptorQ_type::ENC_16:
		p_16 = reinterpret_cast<uint16_t*> (*data);
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
								enc->ptr))->encode (p_16, p_16 + size,esi, sbn);
	case RaptorQ_type::ENC_32:
		p_32 = reinterpret_cast<uint32_t*> (*data);
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
								enc->ptr))->encode (p_32, p_32 + size,esi, sbn);
	case RaptorQ_type::ENC_64:
		p_64 = reinterpret_cast<uint64_t*> (*data);
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
								enc->ptr))->encode (p_64, p_64 + size,esi, sbn);
	case RaptorQ_type::DEC_8:
	case RaptorQ_type::DEC_16:
	case RaptorQ_type::DEC_32:
	case RaptorQ_type::DEC_64:
	case RaptorQ_type::NONE:
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
	if (dec == nullptr || dec->type == RaptorQ_type::NONE)
		return 0;

	switch (dec->type) {
	case RaptorQ_type::DEC_8:
		return (reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
														dec->ptr))->bytes ();
	case RaptorQ_type::DEC_16:
		return (reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
														dec->ptr))->bytes ();
	case RaptorQ_type::DEC_32:
		return (reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
														dec->ptr))->bytes ();
	case RaptorQ_type::DEC_64:
		return (reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
														dec->ptr))->bytes ();
	case RaptorQ_type::ENC_8:
	case RaptorQ_type::ENC_16:
	case RaptorQ_type::ENC_32:
	case RaptorQ_type::ENC_64:
	case RaptorQ_type::NONE:
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

uint64_t RaptorQ_decode (RaptorQ_ptr *dec, void **data, const size_t size)
{
	if (dec == nullptr || dec->type == RaptorQ_type::NONE ||
									dec->ptr == nullptr || data == nullptr) {
		return false;
	}
	uint8_t *p_8;
	uint16_t *p_16;
	uint32_t *p_32;
	uint64_t *p_64;
	uint64_t ret = 0;
	switch (dec->type) {
	case RaptorQ_type::DEC_8:
		p_8 = reinterpret_cast<uint8_t*> (*data);
		ret = (reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
										dec->ptr))->decode (p_8, p_8 + size);
		*data = p_8;
		break;
	case RaptorQ_type::DEC_16:
		p_16 = reinterpret_cast<uint16_t*> (*data);
		ret = (reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
										dec->ptr))->decode (p_16, p_16 + size);
		*data = p_16;
		break;
	case RaptorQ_type::DEC_32:
		p_32 = reinterpret_cast<uint32_t*> (*data);
		ret = (reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
										dec->ptr))->decode (p_32, p_32 + size);
		*data = p_32;
		break;
	case RaptorQ_type::DEC_64:
		p_64 = reinterpret_cast<uint64_t*> (*data);
		ret = (reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
										dec->ptr))->decode (p_64, p_64 + size);
		*data = p_64;
		break;
	case RaptorQ_type::ENC_8:
	case RaptorQ_type::ENC_16:
	case RaptorQ_type::ENC_32:
	case RaptorQ_type::ENC_64:
	case RaptorQ_type::NONE:
		break;
	}
	return ret;
}

uint64_t RaptorQ_decode_block (RaptorQ_ptr *dec, void **data, const size_t size,
															const uint8_t sbn)
{
	if (dec == nullptr || dec->type == RaptorQ_type::NONE ||
									dec->ptr == nullptr || data == nullptr) {
		return false;
	}
	uint8_t *p_8;
	uint16_t *p_16;
	uint32_t *p_32;
	uint64_t *p_64;
	uint64_t ret = 0;
	switch (dec->type) {
	case RaptorQ_type::DEC_8:
		p_8 = reinterpret_cast<uint8_t*> (*data);
		ret = (reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
									dec->ptr))->decode (p_8, p_8 + size, sbn);
		*data = p_8;
		break;
	case RaptorQ_type::DEC_16:
		p_16 = reinterpret_cast<uint16_t*> (*data);
		ret = (reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
									dec->ptr))->decode (p_16, p_16 + size, sbn);
		*data = p_16;
		break;
	case RaptorQ_type::DEC_32:
		p_32 = reinterpret_cast<uint32_t*> (*data);
		ret = (reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
									dec->ptr))->decode (p_32, p_32 + size, sbn);
		*data = p_32;
		break;
	case RaptorQ_type::DEC_64:
		p_64 = reinterpret_cast<uint64_t*> (*data);
		ret = (reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
									dec->ptr))->decode (p_64, p_64 + size, sbn);
		*data = p_64;
		break;
	case RaptorQ_type::ENC_8:
	case RaptorQ_type::ENC_16:
	case RaptorQ_type::ENC_32:
	case RaptorQ_type::ENC_64:
	case RaptorQ_type::NONE:
		break;
	}
	return ret;
}

bool RaptorQ_add_symbol_id (RaptorQ_ptr *dec, void **data, const uint32_t size,
															const uint32_t id)
{
	uint8_t sbn = id >> 24;
	uint32_t esi = (id << 8) >> 8;
	return RaptorQ_add_symbol (dec, data, size, esi, sbn);
}

bool RaptorQ_add_symbol (RaptorQ_ptr *dec, void **data, const uint32_t size,
															const uint32_t esi,
															const uint8_t sbn)
{
	if (dec == nullptr || dec->type == RaptorQ_type::NONE ||
									dec->ptr == nullptr || data == nullptr) {
		return false;
	}
	uint8_t *p_8;
	uint16_t *p_16;
	uint32_t *p_32;
	uint64_t *p_64;
	switch (dec->type) {
	case RaptorQ_type::DEC_8:
		p_8 = reinterpret_cast<uint8_t*> (*data);
		return (reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
							dec->ptr))->add_symbol (p_8, p_8 + size, esi, sbn);
	case RaptorQ_type::DEC_16:
		p_16 = reinterpret_cast<uint16_t*> (*data);
		return (reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
						dec->ptr))->add_symbol (p_16, p_16 + size, esi, sbn);
	case RaptorQ_type::DEC_32:
		p_32 = reinterpret_cast<uint32_t*> (*data);
		return (reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
						dec->ptr))->add_symbol (p_32, p_32 + size, esi, sbn);
	case RaptorQ_type::DEC_64:
		p_64 = reinterpret_cast<uint64_t*> (*data);
		return (reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
						dec->ptr))->add_symbol (p_64, p_64 + size, esi, sbn);
	case RaptorQ_type::ENC_8:
	case RaptorQ_type::ENC_16:
	case RaptorQ_type::ENC_32:
	case RaptorQ_type::ENC_64:
	case RaptorQ_type::NONE:
		return false;
	}
#ifndef USING_CLANG
	// uncomment the return and:
	// clang: WARN: will never be executed (exaustive switch)
	// if commented, GCC: warn: control reaches end of non-void
	// ...make up your mind, guys?
	return false;
#endif
}

///////////////////////
// General: free memory
///////////////////////

void RaptorQ_free (struct RaptorQ_ptr **ptr)
{

	std::unique_ptr<RaptorQ_ptr> uptr;
	if (ptr == nullptr || *ptr == nullptr || (*ptr)->type == RaptorQ_type::NONE
													|| (*ptr)->ptr == nullptr) {
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
	case RaptorQ_type::ENC_8:
		delete reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::ENC_16:
		delete reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::ENC_32:
		delete reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::ENC_64:
		delete reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::DEC_8:
		delete reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::DEC_16:
		delete reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::DEC_32:
		delete reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::DEC_64:
		delete reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
																uptr->ptr);
		break;
	case RaptorQ_type::NONE:
		break;
	}
	uptr->ptr = nullptr;
	return;
}

void RaptorQ_free_block (struct RaptorQ_ptr *ptr, const uint8_t sbn)
{
	if (ptr == nullptr || ptr->type == RaptorQ_type::NONE ||ptr->ptr == nullptr)
		return;
	switch (ptr->type) {
	case RaptorQ_type::ENC_8:
		return (reinterpret_cast<RaptorQ::Encoder<uint8_t*, uint8_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::ENC_16:
		return (reinterpret_cast<RaptorQ::Encoder<uint16_t*, uint16_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::ENC_32:
		return (reinterpret_cast<RaptorQ::Encoder<uint32_t*, uint32_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::ENC_64:
		return (reinterpret_cast<RaptorQ::Encoder<uint64_t*, uint64_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::DEC_8:
		return (reinterpret_cast<RaptorQ::Decoder<uint8_t*, uint8_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::DEC_16:
		return (reinterpret_cast<RaptorQ::Decoder<uint16_t*, uint16_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::DEC_32:
		return (reinterpret_cast<RaptorQ::Decoder<uint32_t*, uint32_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::DEC_64:
		return (reinterpret_cast<RaptorQ::Decoder<uint64_t*, uint64_t*>*> (
														ptr->ptr))->free(sbn);
	case RaptorQ_type::NONE:
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
