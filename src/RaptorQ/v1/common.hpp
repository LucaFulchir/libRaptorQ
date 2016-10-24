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


// These macros were taken from http://gcc.gnu.org/wiki/Visibility
// Generic helper definitions for shared library support

#if defined _WIN32 || defined __CYGWIN__
    #define RAPTORQ_HELPER_DLL_IMPORT __declspec (dllimport)
    #define RAPTORQ_HELPER_DLL_EXPORT __declspec (dllexport)
    #define RAPTORQ_HELPER_DLL_LOCAL
#else
    #if __GNUC__ >= 4
        #define RAPTORQ_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
        #define RAPTORQ_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
        #define RAPTORQ_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
    #else
        #define RAPTORQ_HELPER_DLL_IMPORT
        #define RAPTORQ_HELPER_DLL_EXPORT
        #define RAPTORQ_HELPER_DLL_LOCAL
    #endif
#endif

// Now we use the generic helper definitions above to define RAPTORQ_API
// and RAPTORQ_LOCAL.
// RAPTORQ_API is used for the public API symbols.
// It either DLL imports or DLL exports (or does nothing for static build)
// RAPTORQ_LOCAL is used for non-api symbols.

#ifdef RAPTORQ_DLL // defined if RAPTORQ is compiled as a DLL
    #ifdef RAPTORQ_DLL_EXPORTS // if we are building the RAPTORQ DLL (not using it)
        #define RAPTORQ_API RAPTORQ_HELPER_DLL_EXPORT
    #else
        #define RAPTORQ_API RAPTORQ_HELPER_DLL_IMPORT
    #endif // RAPTORQ_DLL_EXPORTS
    #define RAPTORQ_LOCAL RAPTORQ_HELPER_DLL_LOCAL
#else // RAPTORQ_DLL is not defined: this means RAPTORQ is a static lib.
    #define RAPTORQ_API
    #define RAPTORQ_LOCAL
#endif // RAPTORQ_DLL

#define RAPTORQ_DEPRECATED __attribute__ ((deprecated))

///////////////////////////
////
//// Common error codes
////
///////////////////////////
// NOTE: the C++ and C version should be kept in sync
//      so that we can just static_cast<>() from one to the other

// C version
typedef enum {  RQ_TIME_NANOSEC     = 0,
                RQ_TIME_MICROSEC    = 1,
                RQ_TIME_MILLISEC    = 2,
                RQ_TIME_SEC         = 3,
                RQ_TIME_MIN         = 4,
                RQ_TIME_HOUR        = 5
            } RaptorQ_Unit_Time;

typedef enum {  RQ_ERR_NONE = 0,
                RQ_ERR_NOT_NEEDED = 1,
                RQ_ERR_WRONG_INPUT = 2,
                RQ_ERR_NEED_DATA = 3,
                RQ_ERR_WORKING = 4,
                RQ_ERR_INITIALIZATION = 5,
                RQ_ERR_EXITING = 6
            } RaptorQ_Error;
typedef enum {
    RQ_COMPUTE_NONE = 0x00,                 // do nothing => get error
                                            // warn when;
    RQ_COMPUTE_PARTIAL_FROM_BEGINNING = 0x01,// first blocks can be decoded
    RQ_COMPUTE_PARTIAL_ANY = 0x02,          //  "some" blocks have been decoded.
    RQ_COMPUTE_COMPLETE = 0x04,             //  all blocks are decoded.
    RQ_COMPUTE_NO_BACKGROUND = 0x08,        // no background/async
    RQ_COMPUTE_NO_POOL = 0x10,              // do not use the thread pool
    RQ_COMPUTE_NO_RETRY = 0x20              // do not try again with different
                                            // repair symbol combination
} RaptorQ_Compute;

typedef enum {
    RQ_WORK_KEEP_WORKING = 0,
    RQ_WORK_ABORT_COMPUTATION = 1,
} RaptorQ_Work;

typedef enum {
    RQ_COMPRESS_NONE = 0x00,
    RQ_COMPRESS_LZ4 = 0x01,
} RaptorQ_Compress;



#ifndef __cplusplus
#include <stdint.h>

#else
// C++ version. keep the enum synced
#include <cstdint>

namespace RaptorQ__v1 {

enum class Compress : uint8_t { NONE = RQ_COMPRESS_NONE,
                                LZ4 = RQ_COMPRESS_LZ4
                                };

inline Compress operator| (const Compress a, const Compress b)
{
    return static_cast<Compress> (static_cast<uint8_t> (a) |
                                                    static_cast<uint8_t> (b));
}
inline Compress operator& (const Compress a, const Compress b)
{
    return static_cast<Compress> (static_cast<uint8_t> (a) &
                                                    static_cast<uint8_t> (b));
}
    enum class RAPTORQ_API Error : uint8_t {
                            NONE = RQ_ERR_NONE,
                            NOT_NEEDED = RQ_ERR_NOT_NEEDED,
                            WRONG_INPUT = RQ_ERR_WRONG_INPUT,
                            NEED_DATA = RQ_ERR_NEED_DATA,
                            WORKING = RQ_ERR_WORKING,
                            INITIALIZATION = RQ_ERR_INITIALIZATION,
                            EXITING = RQ_ERR_EXITING
                            };
    enum class RAPTORQ_API Work_State : uint8_t {
        KEEP_WORKING = RQ_WORK_KEEP_WORKING,
        ABORT_COMPUTATION = RQ_WORK_ABORT_COMPUTATION,
    };
} // namespace RaptorQ__v1
namespace RFC6330__v1 {
    using Error = RaptorQ__v1::Error; // easier
    using Compress = RaptorQ__v1::Compress; // easier
    enum class RAPTORQ_API Compute : uint8_t {
        NONE = RQ_COMPUTE_NONE,
        PARTIAL_FROM_BEGINNING = RQ_COMPUTE_PARTIAL_FROM_BEGINNING,
        PARTIAL_ANY = RQ_COMPUTE_PARTIAL_ANY,
        COMPLETE = RQ_COMPUTE_COMPLETE,
        NO_BACKGROUND = RQ_COMPUTE_NO_BACKGROUND,
        NO_POOL = RQ_COMPUTE_NO_POOL,
        NO_RETRY = RQ_COMPUTE_NO_RETRY
    };

inline Compute operator| (const Compute a, const Compute b)
{
    return static_cast<Compute> (static_cast<uint8_t> (a) |
                                                    static_cast<uint8_t> (b));
}
inline Compute operator& (const Compute a, const Compute b)
{
    return static_cast<Compute> (static_cast<uint8_t> (a) &
                                                    static_cast<uint8_t> (b));
}
} // namespace RFC6330__v1
#endif


#define RQ_UNUSED(x)    ((void)x)


// Now just some macros to check the iterator type
#ifdef __cplusplus

namespace RaptorQ__v1 {
namespace Impl {
#ifndef RQ_DEBUG
    #define RQ_DEBUG false
#endif
// don't use #define everywhere. let the dead code elimination take care
// of eliminating unused code.
constexpr bool debug = RQ_DEBUG;
}
}


#include <cassert>
#include <iterator>

#define IS_RANDOM(IT, CLASS) \
        static_assert ( \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::random_access_iterator_tag>::value, \
            CLASS " is supposed to get a RANDOM ACCESS iterator\n");
#define IS_INPUT(IT, CLASS) \
        static_assert ( \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::input_iterator_tag>::value || \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::forward_iterator_tag>::value || \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::bidirectional_iterator_tag>::value || \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::random_access_iterator_tag>::value, \
            CLASS " is supposed to get an INPUT iterator\n");
#define IS_FORWARD(IT, CLASS) \
        static_assert ( \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::forward_iterator_tag>::value || \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::bidirectional_iterator_tag>::value || \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::random_access_iterator_tag>::value, \
            CLASS " is supposed to get a FORWARD iterator\n");
#endif
