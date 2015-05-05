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

#ifndef RAPTORQ_C_H
#define RAPTORQ_C_H

#include "common.hpp"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
	typedef uint64_t RaptorQ_OTI_Common_Data;
	typedef uint32_t RaptorQ_OTI_Scheme_Specific_Data;

	typedef enum { NONE = 0, ENC_8 = 1, ENC_16 = 2, ENC_32 = 3, ENC_64 = 4,
								DEC_8 = 5, DEC_16 = 6, DEC_32 = 7, DEC_64 = 8}
				 RaptorQ_type;

	struct RAPTORQ_LOCAL RaptorQ_ptr;

	RAPTORQ_API struct RaptorQ_ptr* RaptorQ_Enc (const RaptorQ_type type,
											void *data,
											const uint64_t size,
											const uint16_t min_subsymbol_size,
											const uint16_t symbol_size,
											const size_t max_memory);
	RAPTORQ_API struct RaptorQ_ptr* RaptorQ_Dec (const RaptorQ_type type,
								const RaptorQ_OTI_Common_Data common,
								const RaptorQ_OTI_Scheme_Specific_Data scheme);

	///////////
	// Encoding
	///////////

	RaptorQ_OTI_Common_Data RAPTORQ_API RaptorQ_OTI_Common (
													struct RaptorQ_ptr *enc);
	RaptorQ_OTI_Scheme_Specific_Data RAPTORQ_API RaptorQ_OTI_Scheme (
													struct RaptorQ_ptr *enc);

	uint16_t RAPTORQ_API RaptorQ_symbol_size (struct RaptorQ_ptr *ptr);
	uint8_t RAPTORQ_API RaptorQ_blocks (struct RaptorQ_ptr *ptr);
	uint32_t RAPTORQ_API RaptorQ_block_size (struct RaptorQ_ptr *ptr,
															const uint8_t sbn);
	uint16_t RAPTORQ_API RaptorQ_symbols (struct RaptorQ_ptr *ptr,
															const uint8_t sbn);
	uint32_t RAPTORQ_API RaptorQ_max_repair (struct RaptorQ_ptr *enc,
															const uint8_t sbn);
	size_t RAPTORQ_API RaptorQ_precompute_max_memory (struct RaptorQ_ptr *enc);

	void RAPTORQ_API RaptorQ_precompute (struct RaptorQ_ptr *enc,
														const uint8_t threads,
														const bool background);

	uint64_t RAPTORQ_API RaptorQ_encode_id (struct RaptorQ_ptr *enc,
															void **data,
															const uint64_t size,
															const uint32_t id);
	uint64_t RAPTORQ_API RaptorQ_encode (struct RaptorQ_ptr *enc, void **data,
															const uint64_t size,
															const uint32_t esi,
															const uint8_t sbn);
	uint32_t RAPTORQ_API RaptorQ_id (const uint32_t esi, const uint8_t sbn);


	///////////
	// Decoding
	///////////

	uint64_t RAPTORQ_API RaptorQ_bytes (struct RaptorQ_ptr *dec);

	uint64_t RAPTORQ_API RaptorQ_decode (struct RaptorQ_ptr *dec, void **data,
															const size_t size);
	uint64_t RAPTORQ_API RaptorQ_decode_block (struct RaptorQ_ptr *dec,
															void **data,
															const size_t size,
															const uint8_t sbn);

	bool RAPTORQ_API RaptorQ_add_symbol_id (struct RaptorQ_ptr *dec,void **data,
														const uint32_t size,
														const uint32_t id);
	bool RAPTORQ_API RaptorQ_add_symbol (struct RaptorQ_ptr *dec, void **data,
															const uint32_t size,
															const uint32_t esi,
															const uint8_t sbn);

	///////////////////////
	// General: free memory
	///////////////////////

	void RAPTORQ_API RaptorQ_free (struct RaptorQ_ptr **ptr);
	void RAPTORQ_API RaptorQ_free_block (struct RaptorQ_ptr *ptr,
															const uint8_t sbn);

#ifdef __cplusplus
}	// extern "C"
#endif

#endif
