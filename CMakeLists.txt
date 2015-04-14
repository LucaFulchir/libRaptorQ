#
# Copyright (c) 2015, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

FIND_PACKAGE(Threads) # optional. Do not build tests if not found.
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
	IF(${STDLIB} STREQUAL "libstdc++" OR (${CMAKE_CXX_COMPILER} MATCHES "^.*c[+][+]$" OR ${CMAKE_CXX_COMPILER} MATCHES "^(.*/)g[+][+]$"))
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

#
# C++ options
#
IF(${CMAKE_CXX_COMPILER} MATCHES ".*clang.*")
	SET(CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS} ${STDLIB} ${DETERMINISTIC} -std=c++11 -fno-rtti -fno-exceptions -Wall -pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS} ${STDLIB} ${DETERMINISTIC} -std=c++11 -g -fno-rtti -fno-exceptions -Wall -pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS} ${STDLIB} ${DETERMINISTIC} -std=c++11 -Os -DNDEBUG -fno-rtti -fno-exceptions -Wall -pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS} ${STDLIB} ${DETERMINISTIC} -std=c++11 -Ofast -fvisibility=hidden -fvisibility-inlines-hidden -DNDEBUG -fno-rtti -fno-exceptions -Wall -pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-padded -fstack-protector-all -fstrict-aliasing -fwrapv -Wformat -Wformat-security -fPIE -fPIC ")
	ADD_DEFINITIONS(-DUSING_CLANG)
ELSE()
	# warn: clang+ and g++ get the same *g++ regexp, keep the "else" above or match the "/" in the regexp
	IF(${CMAKE_CXX_COMPILER} MATCHES "^.*c[+][+]$" OR ${CMAKE_CXX_COMPILER} MATCHES "^(.*/)g[+][+]$")
	# Totally untested.
	# gcc doesn't seem to support the "-stdlib" option :(
	SET(CMAKE_CXX_FLAGS		   "${CMAKE_CXX_FLAGS} -std=c++11 ${DETERMINISTIC} -Wno-unknown-pragmas -Wall -pedantic -Wno-padded  -fstack-protector-all -fstrict-aliasing -fwrapv")
# FIXME: some of these are gcc4.9+ only
	SET(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS} -std=c++11 ${DETERMINISTIC} -Wno-unknown-pragmas -g -Wall -pedantic -Wno-padded -fstack-protector-all -Wno-aggressive-loop-optimizations -fstrict-aliasing -ftrapv -fwrapv -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fisolate-erroneous-paths-dereference -fisolate-erroneous-paths-attribute ")
	SET(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS} -std=c++11 ${DETERMINISTIC} -Wno-unknown-pragmas -Os -DNDEBUG -Wall -pedantic -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS} -std=c++11 ${DETERMINISTIC} -Wno-unknown-pragmas -Ofast -fvisibility=hidden -fvisibility-inlines-hidden -O4 -DNDEBUG -Wall -pedantic -Wno-padded -fstack-protector-all -fstrict-aliasing -fwrapv -Wformat -Wformat-security -fPIE -fPIC ")
	ENDIF()
	# else I do not know your compiler options :)
ENDIF()
#
# We hae a C interface example to build.
# set the proper options for that, too
#
IF(${CMAKE_C_COMPILER} MATCHES ".*clang.*")
	SET(CMAKE_C_FLAGS                "${CMAKE_C_FLAGS} ${DETERMINISTIC} -std=c11 -Wall -pedantic -Weverything -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS} ${DETERMINISTIC} -std=c11 -g -Wall -pedantic -Weverything -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_C_FLAGS_MINSIZEREL     "${CMAKE_C_FLAGS} ${DETERMINISTIC} -std=c11 -Os -DNDEBUG -Wall -pedantic -Weverything -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS} ${DETERMINISTIC} -std=c11 -Ofast -fvisibility=hidden -fvisibility-inlines-hidden -DNDEBUG -fno-rtti -fno-exceptions -Wall -pedantic -Weverything -Wno-padded -fstack-protector-all -fstrict-aliasing -fwrapv -Wformat -Wformat-security -fPIE -fPIC ")
	ADD_DEFINITIONS(-DUSING_CLANG)
ELSE()
	# warn: clang+ and g++ get the same *g++ regexp, keep the "else" above or match the "/" in the regexp
	IF(${CMAKE_C_COMPILER} MATCHES "^.*cc$" OR ${CMAKE_C_COMPILER} MATCHES "^(.*/)gcc$")
	# Totally untested.
	# gcc doesn't seem to support the "-stdlib" option :(
	SET(CMAKE_C_FLAGS		   "${CMAKE_C_FLAGS} -std=c11 ${DETERMINISTIC} -Wno-unknown-pragmas -Wall -pedantic -Wno-padded  -fstack-protector-all -fstrict-aliasing -fwrapv")
# FIXME: some of these are gcc4.9+ only
	SET(CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS} -std=c11 ${DETERMINISTIC} -Wno-unknown-pragmas -g -Wall -pedantic -Wno-padded -fstack-protector-all -Wno-aggressive-loop-optimizations -fstrict-aliasing -ftrapv -fwrapv -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fisolate-erroneous-paths-dereference -fisolate-erroneous-paths-attribute ")
	SET(CMAKE_C_FLAGS_MINSIZEREL     "${CMAKE_C_FLAGS} -std=c11 ${DETERMINISTIC} -Wno-unknown-pragmas -Os -DNDEBUG -Wall -pedantic -Wno-padded -fstack-protector-all -fstrict-aliasing ")
	SET(CMAKE_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS} -std=c11 ${DETERMINISTIC} -Wno-unknown-pragmas -Ofast -fvisibility=hidden -fvisibility-inlines-hidden -O4 -DNDEBUG -Wall -pedantic -Wno-padded -fstack-protector-all -fstrict-aliasing -fwrapv -Wformat -Wformat-security -fPIE -fPIC ")
	ENDIF()
	# else I do not know your compiler options :)
ENDIF()
IF(APPLE)
  MESSAGE(STATUS "MacOSX system found. WARN: not tested!")
  ADD_DEFINITIONS(-DUNIX)
ENDIF(APPLE)
IF(UNIX)
  MESSAGE(STATUS "UNIX system found")
  ADD_DEFINITIONS(-DUNIX)
ELSEIF(WIN32)
  MESSAGE(STATUS "Wndows system found. WARN: not tested!")
  ADD_DEFINITION(-DWIN32)
ENDIF()


SET(SOURCES	src/Precode_Matrix_solver.cpp
		src/Precode_Matrix.cpp
		src/Rand.cpp
		src/Parameters.cpp
		src/Graph.cpp
		src/cRaptorQ.cpp)

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
		src/cRaptorQ.h)

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# LATEX documentation
add_subdirectory(${CMAKE_SOURCE_DIR}/doc)

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

# Tests need to be linked to threading library
IF (CMAKE_THREAD_LIBS_INIT)
	# add main tests
	ADD_EXECUTABLE(libRaptorQ-test EXCLUDE_FROM_ALL test/rfc_test.cpp)
	ADD_DEPENDENCIES(libRaptorQ-test RaptorQ)
	TARGET_LINK_LIBRARIES(libRaptorQ-test ${CMAKE_THREAD_LIBS_INIT})
	# link the library we just built, too
	TARGET_LINK_LIBRARIES(libRaptorQ-test ${LIBRARY_OUTPUT_PATH}/libRaptorQ.so)
	ADD_CUSTOM_TARGET(tests DEPENDS libRaptorQ-test)

	# build examples
	# C interface
	ADD_EXECUTABLE(test_c EXCLUDE_FROM_ALL test/test_c.c)
	ADD_DEPENDENCIES(test_c RaptorQ)
	TARGET_LINK_LIBRARIES(test_c ${CMAKE_THREAD_LIBS_INIT})
	# link the library we just built, too
	TARGET_LINK_LIBRARIES(test_c ${LIBRARY_OUTPUT_PATH}/libRaptorQ.so)
	# CPP interface
	ADD_EXECUTABLE(test_cpp EXCLUDE_FROM_ALL test/test_cpp.cpp)
	ADD_DEPENDENCIES(test_cpp RaptorQ)
	TARGET_LINK_LIBRARIES(test_cpp ${CMAKE_THREAD_LIBS_INIT})
	# link the library we just built, too
	TARGET_LINK_LIBRARIES(test_cpp ${LIBRARY_OUTPUT_PATH}/libRaptorQ.so)
	ADD_CUSTOM_TARGET(examples DEPENDS test_c test_cpp)

	ADD_CUSTOM_TARGET(everything DEPENDS Make_static_deterministic tests examples docs)
ELSE()
	ADD_CUSTOM_TARGET(everything DEPENDS Make_static_deterministic docs)
ENDIF()

INSTALL(TARGETS libRaptorQ-test RUNTIME DESTINATION bin/ OPTIONAL)


INSTALL(FILES ${HEADERS} DESTINATION include/RaptorQ/)
INSTALL(TARGETS RaptorQ RaptorQ_static
	 LIBRARY DESTINATION lib COMPONENT libraries
	 ARCHIVE DESTINATION lib COMPONENT libraries)
