#
# Copyright (c) 2014, Luca Fulchir<luca@fulchir.it>, All rights reserved.
#
# This file is part of "libRaptorQ".
#
# libRaptorQ is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3
# of the License, or (at your option) any later version.
#
# libRaptorQ is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# and a copy of the GNU Lesser General Public License
# along with libRaptorQ.  If not, see <http://www.gnu.org/licenses/>.

PROJECT(libRaptorQ)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
AUX_SOURCE_DIRECTORY(. SRC_LIST)

# easy compiler choice...doesn't set clang as default, though :(
SET(CMAKE_CXX_COMPILER "clang++" CACHE STRING "Compiler") # default compiler
SET_PROPERTY(CACHE CMAKE_CXX_COMPILER PROPERTY STRINGS clang++ c++ g++)

SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build mode") # default build type
SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel)

SET(STDLIB "libc++" CACHE STRING "Standard library to use")
SET_PROPERTY(CACHE STDLIB PROPERTY STRINGS libc++ stdlib)

# try to guess architecture if it was not provided
INCLUDE(cmake/guess_architecture.cmake)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake; ${CMAKE_MODULE_PATH})

FIND_PACKAGE(eigen REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${EIGEN3_INCLUDE_DIR})

# setup the compiler and the standard library
IF(STDLIB)
	IF(${STDLIB} STREQUAL "libc++" AND NOT (${CMAKE_CXX_COMPILER} MATCHES "^.*c[+][+]$" OR ${CMAKE_CXX_COMPILER} MATCHES "^(.*/)g[+][+]$"))
		# gcc is particoularly unhappy with libc++ headers...
		FIND_PACKAGE(clib_clang REQUIRED)
		INCLUDE_DIRECTORIES(SYSTEM ${CLANG_C_INCLUDE_DIR})
		FIND_PACKAGE(std_clang REQUIRED)
		INCLUDE_DIRECTORIES(SYSTEM ${CLANG_STD_INCLUDE_DIR})
	ENDIF()
	IF(${STDLIB} STREQUAL "stdlib" OR (${CMAKE_CXX_COMPILER} MATCHES "^.*c[+][+]$" OR ${CMAKE_CXX_COMPILER} MATCHES "^(.*/)g[+][+]$"))
		FIND_PACKAGE(clib_gcc REQUIRED)
		INCLUDE_DIRECTORIES(SYSTEM ${GCC_C_INCLUDE_DIR})
		FIND_PACKAGE(std_gcc REQUIRED)
		INCLUDE_DIRECTORIES(SYSTEM ${GCC_STD_INCLUDE_DIR})
	ENDIF()
ENDIF(STDLIB)
# generic standard library support, put it as compiler option:
IF(${STDLIB} MATCHES "^.+$")
	SET(STDLIB	"-stdlib=${STDLIB}")
ENDIF()

#This only works if *FIRST* you checkout a commit and *THEN* you run cmake
EXECUTE_PROCESS(COMMAND git rev-parse --sq HEAD OUTPUT_VARIABLE SEED)
SET(DETERMINISTIC "-frandom-seed=${SEED}")

IF(${CMAKE_CXX_COMPILER} MATCHES ".*clang.*")
	SET(CMAKE_CXX_FLAGS                "${STDLIB} ${DETERMINISTIC} -std=c++11 -fno-rtti -fno-exceptions -Wall -pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_CXX_FLAGS_DEBUG          "${STDLIB} ${DETERMINISTIC} -std=c++11 -g -fno-rtti -fno-exceptions -Wall -pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_CXX_FLAGS_MINSIZEREL     "${STDLIB} ${DETERMINISTIC} -std=c++11 -Os -DNDEBUG -fno-rtti -fno-exceptions -Wall -pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_CXX_FLAGS_RELEASE        "${STDLIB} ${DETERMINISTIC} -std=c++11 -O3 -fvisibility=hidden -fvisibility-inlines-hidden -DNDEBUG -fno-rtti -fno-exceptions -Wall -pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-padded -fstack-protector-all -fstrict-aliasing -fwrapv -Wformat -Wformat-security -fPIE -fPIC ")
	ADD_DEFINITIONS(-DUSING_CLANG)
ELSE()
    # warn: clang+ and g++ get the same *g++ regexp, keep the "else" above or match the "/" in the regexp
	IF(${CMAKE_CXX_COMPILER} MATCHES "^.*c[+][+]$" OR ${CMAKE_CXX_COMPILER} MATCHES "^(.*/)g[+][+]$")
	# Totally untested.
	# gcc doesn't seem to support the "-stdlib" option :(
	SET(CMAKE_CXX_FLAGS		   "-std=c++11 ${DETERMINISTIC} -Wall -pedantic -Wno-padded  -fstack-protector-all -fstrict-aliasing -fwrapv")
# FIXME: some of these are gcc4.9+ only
	SET(CMAKE_CXX_FLAGS_DEBUG          "-std=c++11 ${DETERMINISTIC} -g -Wall -pedantic -Wno-padded -fstack-protector-all -Wno-aggressive-loop-optimizations -fstrict-aliasing -ftrapv -fwrapv -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fisolate-erroneous-paths-dereference -fisolate-erroneous-paths-attribute ")
	SET(CMAKE_CXX_FLAGS_MINSIZEREL     "-std=c++11 ${DETERMINISTIC} -Os -DNDEBUG -Wall -pedantic -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_CXX_FLAGS_RELEASE        "-std=c++11 ${DETERMINISTIC} -flto -O3 -fvisibility=hidden -fvisibility-inlines-hidden -O4 -DNDEBUG -Wall -pedantic -Wno-padded -fstack-protector-all -fstrict-aliasing -fwrapv -Wformat -Wformat-security -fPIE -fPIC ")
    ENDIF()
    # else I do not know your compiler options :)
ENDIF()

IF(APPLE)
  MESSAGE(SEND_ERROR "Totally untested. Buy me a mac?")
  ADD_DEFINITIONS(-DUNIX)
  SET(HEADERS_PLATFORM )
  SET(SOURCES_PLATFORM )
ENDIF(APPLE)
IF(UNIX)
  MESSAGE(STATUS "UNIX system found")
  ADD_DEFINITIONS(-DUNIX)
  SET(HEADERS_PLATFORM )
  SET(SOURCES_PLATFORM )
ELSEIF(WIN32)
  MESSAGE(FATAL_ERROR "Win32 systems not supported yet!")
  ADD_DEFINITION(-DWIN32)
ENDIF()


SET(SOURCES	src/Precode_Matrix_solver.cpp
		src/Precode_Matrix.cpp
		src/Rand.cpp
		src/Parameters.cpp
		src/Graph.cpp
		src/cRaptorQ.c
		${SOURCES_PLATFORM})

SET(HEADERS	src/Interleaver.hpp
		src/multiplication.hpp
		src/table2.hpp
		src/degree.hpp
		src/common.hpp
		src/Encoder.hpp
		src/Decoder.hpp
		src/Rand.hpp
		src/Precode_Matrix.hpp
		src/Parameters.hpp
		src/Graph.hpp
		src/De_Interleaver.hpp
		src/RaptorQ.hpp
		src/cRaptorQ.h
		${HEADERS_PLATFORM})

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_LIBRARY(RaptorQ_static STATIC ${SOURCES} ${HEADERS})
# ar: add deterministic build
SET_TARGET_PROPERTIES (
	RaptorQ_static
	PROPERTIES
	LINK_FLAGS "-Wl,-z,now,-z,relro -pie"
	STATIC_LIBRARY_FLAGS "-D"
)

ADD_EXECUTABLE(make_deterministic src/deterministic.cpp)
add_custom_command(
  OUTPUT deterministic.run
  COMMAND make_deterministic ${LIBRARY_OUTPUT_PATH}/libRaptorQ_static.a
  DEPENDS RaptorQ_static
  COMMENT "Removing creation date from library..."
  VERBATIM
)
add_custom_target(
  Make_static_deterministic ALL
  DEPENDS deterministic.run
)

ADD_DEFINITIONS(-DRAPTORQ_DLL)
ADD_DEFINITIONS(-DRAPTORQ_DLL_EXPORTS)

ADD_LIBRARY(RaptorQ SHARED ${SOURCES} ${HEADERS})
SET_TARGET_PROPERTIES (
	RaptorQ
	PROPERTIES
	LINK_FLAGS "-Wl,-z,now,-z,relro"
)
SET(CMAKE_INSTALL_PREFIX /usr)
INSTALL(FILES ${HEADERS} DESTINATION include/RaptorQ/)
INSTALL(TARGETS RaptorQ RaptorQ_static
     LIBRARY DESTINATION lib COMPONENT libraries
     ARCHIVE DESTINATION lib COMPONENT libraries)
