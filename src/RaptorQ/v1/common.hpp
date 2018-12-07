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

/* Supporting header-only and compiled modes mean that sometimes a function
 * should be inline and other times not.
 * This becouse inline means both "single implementation" (useful for hdr-only)
 * and "local to the translation unit" (bad for compiled)
 */
#ifdef RQ_HEADER_ONLY
#define RQ_HDR_INLINE inline
#else
#define RQ_HDR_INLINE
#endif


#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-macros"
#pragma clang diagnostic ignored "-Wunused-variable"
#ifdef RQ_VERSION
    static char RaptorQ_version[] = RQ_VERSION;
#else
    // let's see if I remember to update this...
    static char RaptorQ_version[] = "1.0.0-rc1";
#endif
#define RQ_UNUSED(x)    ((void)x)
#pragma GCC diagnostic pop
#pragma clang diagnostic pop

#include "RaptorQ/v1/wrapper/C_common.h"

#ifndef __cplusplus
#include <stddef.h>
#include <stdint.h>
#else
#include <cstddef>
#include <cstdint>
#include <type_traits>
//windows does not have ssize_t
typedef std::make_signed<size_t>::type ssize_t;
#endif

static const uint64_t RFC6330_max_data = 946270874880;  // ~881 GB
typedef uint64_t RFC6330_OTI_Common_Data;
typedef uint32_t RFC6330_OTI_Scheme_Specific_Data;

#if defined(__cplusplus)&& ( __cplusplus >= 201103L || _MSC_VER > 1900 )
// C++ version. keep the enum synced
#include <memory>
#include <utility>

namespace RaptorQ__v1 {
// Bring back make_unique from C++14
// but do not pollute the std:: namespace
namespace Impl {
template<typename T, typename... Ts>
std::unique_ptr<T> RAPTORQ_LOCAL make_unique (Ts&&... params)
    { return std::unique_ptr<T> (new T(std::forward<Ts> (params)...)); }
} // namespace Impl

#ifndef RQ_DEBUG
    #define RQ_DEBUG false
#endif
// don't use #define everywhere. let the dead code elimination take care
// of eliminating unused code.
constexpr bool debug = RQ_DEBUG;

enum class Decoder_Result : uint8_t {
    DECODED = RQ_DEC_DECODED,
    STOPPED = RQ_DEC_STOPPED,
    CAN_RETRY = RQ_DEC_CAN_RETRY,
    NEED_DATA = RQ_DEC_NEED_DATA
};

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
enum class Error : uint8_t {
                        NONE = RQ_ERR_NONE,
                        NOT_NEEDED = RQ_ERR_NOT_NEEDED,
                        WRONG_INPUT = RQ_ERR_WRONG_INPUT,
                        NEED_DATA = RQ_ERR_NEED_DATA,
                        WORKING = RQ_ERR_WORKING,
                        INITIALIZATION = RQ_ERR_INITIALIZATION,
                        EXITING = RQ_ERR_EXITING
                        };
enum class Work_State : uint8_t {
    KEEP_WORKING = RQ_WORK_KEEP_WORKING,
    ABORT_COMPUTATION = RQ_WORK_ABORT_COMPUTATION,
};
// tracked by C_RAW_API.h/RaptorQ_Dec_wait_res
struct RAPTORQ_API Decoder_wait_res {
    Error error;
    uint16_t symbol;
};
// different than RFC6330_v1::Decoder_written
// the sizes are *not* forced from the RFC
// tracked by C_RAW_API.h/RaptorQ_Dec_written
struct RAPTORQ_API Decoder_written {
    size_t written;
    size_t offset;
};

// tracked by C_RAW_API.h/RQ_Dec_Report
enum class Dec_Report : uint8_t {
    PARTIAL_FROM_BEGINNING = RQ_COMPUTE_PARTIAL_FROM_BEGINNING,
    PARTIAL_ANY = RQ_COMPUTE_PARTIAL_ANY,
    COMPLETE = RQ_COMPUTE_COMPLETE
};

// tracks C_common.h/RFC6330_Compute
enum class Compute : uint8_t {
    NONE = RQ_COMPUTE_NONE,
    PARTIAL_FROM_BEGINNING = RQ_COMPUTE_PARTIAL_FROM_BEGINNING,
    PARTIAL_ANY = RQ_COMPUTE_PARTIAL_ANY,
    COMPLETE = RQ_COMPUTE_COMPLETE,
    NO_BACKGROUND = RQ_COMPUTE_NO_BACKGROUND,
    NO_POOL = RQ_COMPUTE_NO_POOL,
    NO_RETRY = RQ_COMPUTE_NO_RETRY
};

// tracks C_common.h/RaptorQ_Fill_With_Zeros
enum class Fill_With_Zeros : uint8_t { NO  = RQ_NO_FILL,
                                       YES = RQ_FILL_WITH_ZEROS };

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

} // namespace RaptorQ__v1


//////////
// RFC6330
//////////

namespace RFC6330__v1 {
namespace Impl {
    using RaptorQ__v1::Impl::make_unique;
} // namespace Impl

using Compute = RaptorQ__v1::Compute;
using Compress = RaptorQ__v1::Compress;
using Error = RaptorQ__v1::Error;
using Fill_With_Zeros = RaptorQ__v1::Fill_With_Zeros;
using Work_State = RaptorQ__v1::Work_State;

// dieffrent than RaptorQ_v1::Decoder_written
// the sizes are forced from the RFC
// tracked by C_RFC.h/RFC6330_Dec_Result
struct RAPTORQ_API Decoder_written
{
    uint64_t written;
    uint8_t offset;
};
} // namespace RFC6330__v1



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
#endif //__cplusplus >= 201103L
