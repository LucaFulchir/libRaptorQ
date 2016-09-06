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
#include "RaptorQ/v1/API_Iterators.hpp"
#include "RaptorQ/v1/RFC.hpp"
#include <cmath>
#include <future>
#include <memory>
#include <utility>

/////////////////////
//
//	These templates are just a wrapper around the
//	functionalities offered by the RaptorQ__v1::Impl namespace
//	So if you want to see what the algorithm looks like,
//	you are in the wrong place
//
/////////////////////

namespace RFC6330__v1 {

////////////////////
//// Free Functions
////////////////////

uint64_t RAPTORQ_API shared_cache_size (const uint64_t shared_cache,
													const Compress compression);
bool RAPTORQ_API local_cache_size (const uint64_t local_cache,
													const Compress compression);
uint64_t RAPTORQ_API get_shared_cache_size();
uint64_t RAPTORQ_API get_local_cache_size();

bool RAPTORQ_API set_thread_pool (const size_t threads,
									const uint16_t max_block_concurrency,
									const RaptorQ__v1::Work_State exit_type);

////////////////////
//// Encoder
////////////////////

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Encoder
{
public:
	~Encoder() {}
	Encoder (const Rnd_It data_from, const Rnd_It data_to,
											const uint16_t min_subsymbol_size,
											const uint16_t symbol_size,
											const size_t max_memory)
	{
        rfc_encoder = std::unique_ptr<Impl::Encoder<Rnd_It, Fwd_It>> (
								   new Impl::Encoder<Rnd_It, Fwd_It> (
                                        data_from, data_to, min_subsymbol_size,
                                                    symbol_size, max_memory));
	}

	Block_Iterator<Rnd_It, Fwd_It> begin ()
	{
        if (rfc_encoder != nullptr)
            return rfc_encoder->begin();
        return end();
	}
	const Block_Iterator<Rnd_It, Fwd_It> end ()
	{
        if (rfc_encoder != nullptr)
            return rfc_encoder->end();
        return Block_Iterator<Rnd_It, Fwd_It> (nullptr, Impl::Partition(), 1);
	}

	operator bool() const { return rfc_encoder != nullptr && (*rfc_encoder); }
	RQ_OTI_Common_Data OTI_Common() const
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->OTI_Common();
        return 0;
    }
	RQ_OTI_Scheme_Specific_Data OTI_Scheme_Specific() const
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->OTI_Scheme_Specific();
        return 0;
    }

	std::future<std::pair<Error, uint8_t>> compute (const Compute flags)
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->compute (flags);
        std::promise<std::pair<Error, uint8_t>> p;
		p.set_value ({Error::INITIALIZATION, 0});
        return p.get_future();
    }

	size_t precompute_max_memory ()
    {
		if (rfc_encoder != nullptr)
            return rfc_encoder->precompute_max_memory ();
        return 0;
    }
	uint64_t encode (Fwd_It &output, const Fwd_It end, const uint32_t esi,
															const uint8_t sbn)
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->encode (output, end, esi, sbn);
        return 0;
    }
	// id: 8-bit sbn + 24 bit esi
	uint64_t encode (Fwd_It &output, const Fwd_It end, const uint32_t &id)
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->encode (output, end, id);
        return 0;
    }
	void free (const uint8_t sbn)
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->free (sbn);
    }
	uint8_t blocks() const
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->blocks();
        return 0;
    }
	uint32_t block_size (const uint8_t sbn) const
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->block_size (sbn);
        return 0;
    }
	uint16_t symbol_size() const
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->symbol_size();
        return 0;
    }
	uint16_t symbols (const uint8_t sbn) const
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->symbols (sbn);
        return 0;
    }
	uint32_t max_repair (const uint8_t sbn) const
    {
        if (rfc_encoder != nullptr)
            return rfc_encoder->max_repair (sbn);
        return 0;
    }
private:
    std::unique_ptr<Impl::Encoder<Rnd_It, Fwd_It>> rfc_encoder;
};

////////////////////
//// Decoder
////////////////////

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
	Decoder (const RQ_OTI_Common_Data common,
                            const RQ_OTI_Scheme_Specific_Data scheme)
	{
        rfc_decoder = std::unique_ptr<Impl::Decoder<In_It, Fwd_It>> (
									new Impl::Decoder<In_It, Fwd_It> (
													        common, scheme));
	}

	Decoder (const uint64_t size, const uint16_t symbol_size,
													const uint16_t sub_blocks,
													const uint8_t blocks,
													const uint8_t alignment)
	{
        rfc_decoder = std::unique_ptr<Impl::Decoder<In_It, Fwd_It>> (
                                new Impl::Decoder<In_It, Fwd_It> (
												size, symbol_size, sub_blocks,
												blocks, alignment));
	}

	std::future<std::pair<Error, uint8_t>> compute (const Compute flags)
    {
        if (rfc_decoder != nullptr)
            return rfc_decoder->compute (flags);
        std::promise<std::pair<Error, uint8_t>> p;
		p.set_value ({Error::INITIALIZATION, 0});
        return p.get_future();
    }

	// result in BYTES
	uint64_t decode_bytes (Fwd_It &start, const Fwd_It end, const uint8_t skip)
    {
        if (rfc_decoder == nullptr)
            return 0;
        return rfc_decoder->decode_bytes (start, end, skip);
    }
	uint64_t decode_block_bytes (Fwd_It &start, const Fwd_It end,
															const uint8_t skip,
															const uint8_t sbn)
    {
        if (rfc_decoder == nullptr)
            return 0;
        return rfc_decoder->decode_block_bytes (start, end, skip, sbn);
    }
	// result in ITERATORS
	// last *might* be half written depending on data alignments
	std::pair<size_t, uint8_t> decode_aligned (Fwd_It &start, const Fwd_It end,
															const uint8_t skip)
    {
        if (rfc_decoder == nullptr)
			return {0, 0};
        return rfc_decoder->decode_aligned (start, end, skip);
    }
	std::pair<size_t, uint8_t> decode_block_aligned (Fwd_It &start,
															const Fwd_It end,
															const uint8_t skip,
															const uint8_t sbn)
    {
        if (rfc_decoder == nullptr)
			return {0, 0};
        return rfc_decoder->decode_block_aligned (start, end, skip, sbn);
    }
	// id: 8-bit sbn + 24 bit esi
	Error add_symbol (In_It &start, const In_It end, const uint32_t id)
    {
        if (rfc_decoder == nullptr)
            return Error::INITIALIZATION;
        return rfc_decoder->add_symbol (start, end, id);
    }
	Error add_symbol (In_It &start, const In_It end, const uint32_t esi,
															const uint8_t sbn)
    {
        if (rfc_decoder == nullptr)
            return Error::INITIALIZATION;
        return rfc_decoder->add_symbol (start, end, esi, sbn);
    }
	void free (const uint8_t sbn)
    {
        if (rfc_decoder != nullptr)
            return rfc_decoder->free (sbn);
    }
	uint64_t bytes() const
    {
        if (rfc_decoder == nullptr)
            return 0;
        return rfc_decoder->bytes();
    }
	uint8_t blocks() const
    {
        if (rfc_decoder == nullptr)
            return 0;
        return rfc_decoder->blocks();
    }
	uint32_t block_size (const uint8_t sbn) const
    {
        if (rfc_decoder == nullptr)
            return 0;
        return rfc_decoder->block_size (sbn);
    }
	uint16_t symbol_size() const
    {
        if (rfc_decoder == nullptr)
            return 0;
        return rfc_decoder->symbol_size();
    }
	uint16_t symbols (const uint8_t sbn) const
    {
        if (rfc_decoder == nullptr)
            return 0;
        return rfc_decoder->symbols (sbn);
    }
private:
    std::unique_ptr<Impl::Decoder<In_It, Fwd_It>> rfc_decoder;
};

}   // namespace RFC6330__v1
