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

///////////////////////////
////
//// Common error codes
////
///////////////////////////
// NOTE: the C++ and C version should be kept in sync
//		so that we can just static_cast<>() from the C++ to the C version.

#ifdef __cplusplus
// C++ version
#include <cstdint>
namespace RaptorQ__v1 {
	enum class Error : uint8_t {
							NONE = 0,
							NOT_NEEDED = 1,
							WRONG_INPUT = 2
							};
}
// versioning support
namespace RaptorQ = RaptorQ__v1;
#endif
// C version
typedef enum {	RQ_ERR_NONE = 0,
				RQ_ERR_NOT_NEEDED = 1,
				RQ_ERR_WRONG_INPUT = 2,
				RQ_ERR_CHECK_INIT = 3,
				RQ_ERR_INTERNAL = 4
			} RaptorQ_Error;


// Now just some macros to check the iterator type
#ifdef __cplusplus

#include <cassert>
#include <iterator>

#define UNUSED(x)	((void)x)

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
