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

#ifndef RAPTORQ_H
#define RAPTORQ_H


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


#ifdef __cplusplus

#include <cassert>
#include <cstdint>
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

#endif

