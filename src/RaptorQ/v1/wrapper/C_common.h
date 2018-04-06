/*
 * Copyright (c) 2015-2018, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

#ifdef __cplusplus
extern "C"
{
#endif
typedef enum { RQ_NONE = 0, RQ_ENC_8 = 1, RQ_ENC_16 = 2, RQ_ENC_32 = 3,
                RQ_ENC_64 = 4, RQ_DEC_8 = 5, RQ_DEC_16 = 6, RQ_DEC_32 = 7,
                RQ_DEC_64 = 8 } RaptorQ_type;
typedef RaptorQ_type RFC6330_type;


///////////////////////////
////
//// Common error codes
////
///////////////////////////
// NOTE: these enum and structs MUST be synched with the C++ version
//      so that we can just static_cast<>() from one to the other

typedef enum {  RQ_TIME_NANOSEC     = 0,
                RQ_TIME_MICROSEC    = 1,
                RQ_TIME_MILLISEC    = 2,
                RQ_TIME_SEC         = 3,
                RQ_TIME_MIN         = 4,
                RQ_TIME_HOUR        = 5
            } RaptorQ_Unit_Time;
typedef RaptorQ_Unit_Time RFC6330_Unit_Time;


typedef enum {  RQ_ERR_NONE = 0,
                RQ_ERR_NOT_NEEDED = 1,
                RQ_ERR_WRONG_INPUT = 2,
                RQ_ERR_NEED_DATA = 3,
                RQ_ERR_WORKING = 4,
                RQ_ERR_INITIALIZATION = 5,
                RQ_ERR_EXITING = 6
            } RaptorQ_Error;
typedef RaptorQ_Error RFC6330_Error;

// tracked by common.hpp/RaptorQ__v1::Impl::Compute
typedef enum {
    RQ_COMPUTE_NONE = 0x00,                 // do nothing => get error
                                            // used to check flags.

                                            // report when:
    RQ_COMPUTE_PARTIAL_FROM_BEGINNING = 0x01,// first blocks can be decoded
    RQ_COMPUTE_PARTIAL_ANY = 0x02,          //  "some" blocks have been decoded.
    RQ_COMPUTE_COMPLETE = 0x04,             //  all blocks are decoded.
    RQ_COMPUTE_NO_BACKGROUND = 0x08,        // no background/async. RFC
    RQ_COMPUTE_NO_POOL = 0x10,              // do not use the thread pool. RFC
    RQ_COMPUTE_NO_RETRY = 0x20              // do not try again with different
                                            // repair symbol combination. RFC
} RFC6330_Compute;

typedef enum {
    RQ_WORK_KEEP_WORKING = 0,
    RQ_WORK_ABORT_COMPUTATION = 1,
} RaptorQ_Work;
typedef RaptorQ_Work RFC6330_Work;

typedef enum {
    RQ_COMPRESS_NONE = 0x00,
    RQ_COMPRESS_LZ4 = 0x01,
} RaptorQ_Compress;
typedef RaptorQ_Compress RFC6330_Compress;

typedef enum {
    RQ_DEC_DECODED     = 0,
    RQ_DEC_STOPPED     = 1,
    RQ_DEC_CAN_RETRY   = 2,
    RQ_DEC_NEED_DATA   = 3,
    RQ_DEC_WRONG_INPUT = 4 // used by the C wrapper
} RaptorQ_Decoder_Result;

// tracked by RaptorQ__v1::Fill_With_Zeros
typedef enum {
    RQ_NO_FILL         = 0,
    RQ_FILL_WITH_ZEROS = 1
} RaptorQ_Fill_With_Zeros;
typedef RaptorQ_Fill_With_Zeros RFC6330_Fill_With_Zeros;

#ifdef __cplusplus
}   // extern "C"
#endif
