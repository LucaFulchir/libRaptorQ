#
# Copyright (c) 2015-2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

cmake_minimum_required (VERSION 2.8.12)
project (libRaptorQ)
enable_language(CXX)
enable_language(C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake; ${CMAKE_MODULE_PATH})
set(RQ_VERSION 1.0.0-beta1)
set(RQ_ABI 1)
message(STATUS "libRaptorQ version ${RQ_VERSION}")
add_definitions(-DRQ_VERSION="${RQ_VERSION}")
add_definitions(-DEIGEN_MPL2_ONLY)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()


include(GNUInstallDirs)

option(DYNAMIC_LIB "Build dynamic library" ON)
option(STATIC_LIB "Build static library" ON)
option(CLANG_STDLIB "Use clang's libc++" OFF)
option(CLI "BUild CLI tools" ON)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build Type")
set(USE_LZ4 "ON" CACHE STRING "Use LZ4 compression for result caching")
set(RQ_LINKER CACHE STRING "linker to use (auto/gold/ld/bsd)")
set(RQ_ENDIANNESS CACHE STRING "endianness of your system")
set_property(CACHE CMAKE_BUILD_TYPE   PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
set_property(CACHE USE_LZ4   PROPERTY STRINGS ON BUNDLED OFF)
set_property(CACHE RQ_LINKER PROPERTY STRINGS GOLD LD BSD)
set_property(CACHE RQ_ENDIANNESS PROPERTY STRINGS Auto BigEndian LittleEndian)
mark_as_advanced(FORCE RQ_LINKER RQ_ENDIANNESS RQ_GOLD RQ_LZ4_LIB)
if (USE_LZ4 MATCHES "BUNDLED")
    set(USE_LZ4 "ON")
    set(RQ_LZ4_USE_OWN TRUE)
endif()

if (NOT RQ_ENDIANNESS)
    set(RQ_ENDIANNESS Auto)
endif()
if (RQ_ENDIANNESS MATCHES "Auto")
    include(TestBigEndian)
    TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
    if (IS_BIG_ENDIAN)
        add_definitions(-DRQ_BIG_ENDIAN)
        message (STATUS "Detected big endian machine")
    else()
        add_definitions(-DRQ_LITTLE_ENDIAN)
        message (STATUS "Detected little endian machine")
    endif()
elseif (RQ_ENDIANNESS MATCHES "BigEndian")
    add_definitions(-DRQ_BIG_ENDIAN)
    message (STATUS "Forced to big endian")
elseif (RQ_ENDIANNESS MATCHES "LittleEndian")
    add_definitions(-DRQ_LITTLE_ENDIAN)
    message (STATUS "Forced to little endian")
endif()

# supported linkers: gold, linux standard, bsd
if(NOT RQ_LINKER)
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin" OR CMAKE_SYSTEM_NAME MATCHES "FreeBSD" OR CMAKE_SYSTEM_NAME MATCHES "Windows")
        set(RQ_LINKER BSD)
    else()
        find_program(RQ_GOLD ld.gold)
        if (NOT RQ_GOLD MATCHES "RQ_GOLD-NOTFOUND")
            set(RQ_LINKER GOLD)
        else()
            set(RQ_LINKER LD)
        endif()
    endif()
endif()
if(RQ_LINKER MATCHES GOLD)
    message(STATUS "Using the GOLD linker")
elseif(RQ_LINKER MATCHES LD)
    message(STATUS "Using the LD linker")
elseif(RQ_LINKER MATCHES BSD)
    message(STATUS "Using the BSD linker")
else()
    message(WARNING "Linker not found? Default no option.")
    set(RQ_LINKER BSD)
endif()

# defaults: only enable LTO/PROFILING with known compilers
# exception: osx/BSD. llvm linker/profiling not default?
if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU") AND
        (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin") AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD"))
    option(LTO "Link Time Optimization" ON)
    option(PROFILING "Profiling: speedup library" ON)
else()
    option(LTO "Link Time Optimization" OFF)
    option(PROFILING "Profiling: speedup library" OFF)
endif()

message(STATUS "Build selected: ${CMAKE_BUILD_TYPE}")

if(NOT (STATIC_LIB MATCHES "ON"))
    if (NOT (DYNAMIC_LIB MATCHES "ON"))
        message(FATAL_ERROR "Do you want to actually build the library?")
    endif()
endif()


# with LTO each compiler needs its own AR/NM/RANLIB
if(LTO MATCHES "ON")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            find_program(RQ_AR "llvm-ar")
            find_program(RQ_NM "llvm-nm")
            find_program(RQ_RANLIB "llvm-ranlib")
            find_program(RQ_LLVM_PROF llvm-profdata)
            mark_as_advanced(FORCE RQ_LLVM_PROF)
            if (RQ_LLVM_PROF MATCHES "RQ_LLVM_PROF-NOTFOUND")
                message(FATAL_ERROR "Profiling support with clang requires \"llvm-profdata\". Disable profiling with cmake option \"-DPROFILING=OFF\"")
            endif()
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            find_program(RQ_AR "gcc-ar")
            find_program(RQ_NM "gcc-nm")
            find_program(RQ_RANLIB "gcc-ranlib")
        endif()
        # check if we found the linker
        if (RQ_AR STREQUAL "RQ_AR-NOTFOUND" OR RQ_NM STREQUAL "RQ_NM-NOTFOUND" OR RQ_RANLIB STREQUAL "RQ_RANLIB-NOTFOUND")
            message(FATAL_ERROR "Can't find the linker programs. Please disable LTO with cmake option \"-DLTO=OFF\"")
        endif()
        set(CMAKE_AR ${RQ_AR})
        set(CMAKE_NM ${RQ_NM})
        set(CMAKE_RANLIB ${RQ_RANLIB})
        mark_as_advanced(FORCE RQ_AR RQ_NM RQ_RANLIB)
        message(STATUS "Link Time Optimization activated")
    else()
        message(FATAL_ERROR "Sorry, don't know how to do LTO with your compiler")
    endif()
else()
    message(STATUS "Link Time Optimization deactivated")
endif()

# Profiling is pretty compiler-specific....
if(PROFILING MATCHES "ON")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(STATUS "Profiling activated")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(WARNING "Clang's profile support breaks deterministic builds!")
        endif()
    else()
        message(FATAL_ERROR "Sorry, don't know how to profile with your compiler")
    endif()
else()
    message(STATUS "Profiling deactivated")
endif()

if(CLANG_STDLIB MATCHES "ON")
    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        message(FATAL_ERROR "libc++ is only supported by clang")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(STDLIB stdc++)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CLANG_STDLIB MATCHES "ON")
        set(RQ_STDLIB_FLAG -stdlib=libc++)
        set(STDLIB c++)
    else()
        set(STDLIB stdc++)
    endif()
endif()


find_package(Threads REQUIRED)
find_package(git)
find_package(eigen REQUIRED)
include_directories(SYSTEM ${EIGEN3_INCLUDE_DIR})
if (USE_LZ4 MATCHES "ON")
    find_package(RQ_LZ4 REQUIRED)
    include_directories(SYSTEM ${RQ_LZ4_INCLUDE_DIR})
    add_definitions(-DRQ_USE_LZ4)
endif()
#lz4 build if necessary
include(${CMAKE_CURRENT_SOURCE_DIR}/external/build_deps.cmake)
if (CLI MATCHES "ON")
    message(STATUS "Building CLI tools")
else()
    message(STATUS "NOT Building CLI tools")
endif()

# our include tree
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# LATEX documentation
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/doc)


if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/.git/ AND GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --sq HEAD OUTPUT_VARIABLE RQ_SEED)
else()
    set(RQ_SEED RaptorQ_nongit_v${RQ_VERSION})
    message(WARNING "Not inside a git repository. Compiler seed is now constant.")
endif()
set(DETERMINISTIC -frandom-seed=${RQ_SEED})

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DRQ_WINDOWS)
    message(WARNING, "RaptorQ builds are not deterministic in Windows yet")
else()
    add_definitions(-DRQ_UNIX)
endif()


SET(SOURCES
            src/RaptorQ/v1/wrapper/C_RAW_API.cpp
            src/RaptorQ/v1/wrapper/C_RFC_API.cpp
            src/RaptorQ/v1/wrapper/CPP_RAW_API_void.cpp
            src/RaptorQ/v1/wrapper/CPP_RFC_API_void.cpp
            src/RaptorQ/v1/wrapper/CPP_caches.cpp
            )

SET(HEADERS
            src/RaptorQ/v1/block_sizes.hpp
            src/RaptorQ/v1/caches.hpp
            src/RaptorQ/v1/caches.ipp
            src/RaptorQ/v1/common.hpp
            src/RaptorQ/v1/De_Interleaver.hpp
            src/RaptorQ/v1/Decoder.hpp
            src/RaptorQ/v1/degree.hpp
            src/RaptorQ/v1/Encoder.hpp
            src/RaptorQ/v1/Interleaver.hpp
            src/RaptorQ/v1/multiplication.hpp
            src/RaptorQ/v1/Octet.hpp
            src/RaptorQ/v1/Operation.hpp
            src/RaptorQ/v1/Parameters.hpp
            src/RaptorQ/v1/Precode_Matrix.hpp
            src/RaptorQ/v1/Precode_Matrix_Init.hpp
            src/RaptorQ/v1/Precode_Matrix_Solver.hpp
            src/RaptorQ/v1/Rand.hpp
            src/RaptorQ/v1/RaptorQ.hpp
            src/RaptorQ/v1/RaptorQ_Iterators.hpp
            src/RaptorQ/v1/RFC.hpp
            src/RaptorQ/v1/RFC_Iterators.hpp
            src/RaptorQ/v1/Shared_Computation/Decaying_LF.hpp
            src/RaptorQ/v1/table2.hpp
            src/RaptorQ/v1/Thread_Pool.hpp
            src/RaptorQ/v1/util/Bitmask.hpp
            src/RaptorQ/v1/util/div.hpp
            src/RaptorQ/v1/util/endianess.hpp
            src/RaptorQ/v1/util/Graph.hpp
            )

SET(HEADERS_LINKED
            src/RaptorQ/RaptorQ.h
            src/RaptorQ/RaptorQ_v1.hpp
            src/RaptorQ/RFC6330.h
            src/RaptorQ/RFC6330_v1.hpp
            src/RaptorQ/v1/wrapper/C_common.h
            src/RaptorQ/v1/wrapper/C_RAW_API.h
            src/RaptorQ/v1/wrapper/C_RFC_API.h
            src/RaptorQ/v1/wrapper/CPP_RFC_API.hpp
            src/RaptorQ/v1/wrapper/CPP_RFC_API_void.hpp
            src/RaptorQ/v1/wrapper/CPP_RAW_API.hpp
            src/RaptorQ/v1/wrapper/CPP_RAW_API_void.hpp
            )

SET(HEADERS_ONLY
            src/RaptorQ/RaptorQ_v1_hdr.hpp
            src/RaptorQ/RFC6330_v1_hdr.hpp
            )

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    # windows-only placeholder
else()
    # linux-only placeholder
endif()

if (USE_LZ4 MATCHES "ON")
    SET (HEADERS ${HEADERS}
            src/RaptorQ/v1/Shared_Computation/LZ4_Wrapper.hpp)
endif()

include(cmake/compiler_flags.cmake)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_definitions(-DUSING_CLANG)
    if (LTO MATCHES "ON")
        if (RQ_LINKER MATCHES GOLD)
            set(LD_OPT "-flto -fuse-ld=gold" )
        else()
            set(LD_OPT "-flto" )
        endif()
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(LD_OPT "")
endif()


########################################
## TODO: if windows, add notelemetry.obj
########################################


if(PROFILING MATCHES "ON")
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(PROFILE_GET           -fprofile-instr-generate)
        set(PROFILE_GET_FILE      RaptorQ.profraw)
        set(PROFILE_SET_FILE      RaptorQ.profdata)
        set(PROFILE_SET           "-fprofile-instr-use=${PROFILE_SET_FILE}")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(PROFILE_GET           -fprofile-generate)
        # gcc creates a couple more files than clang for prifiling. Track them.
        set(PROFILE_GET_FILE
CMakeFiles/test_c_profiled.dir/test/test_c.c.gcda
CMakeFiles/RaptorQ_Static_Profiling.dir/src/RaptorQ/v1/Shared_Computation/Decaying_LF.cpp.gcda
CMakeFiles/RaptorQ_Static_Profiling.dir/src/RaptorQ/v1/Graph.cpp.gcda
CMakeFiles/RaptorQ_Static_Profiling.dir/src/RaptorQ/v1/Rand.cpp.gcda
CMakeFiles/RaptorQ_Static_Profiling.dir/src/RaptorQ/v1/Operation.cpp.gcda
CMakeFiles/RaptorQ_Static_Profiling.dir/src/RaptorQ/v1/RFC.cpp.gcda
CMakeFiles/RaptorQ_Static_Profiling.dir/src/RaptorQ/v1/Precode_Matrix_Instantiation.cpp.gcda
CMakeFiles/RaptorQ_Static_Profiling.dir/src/RaptorQ/v1/Parameters.cpp.gcda
CMakeFiles/RaptorQ_Static_Profiling.dir/src/RaptorQ/v1/Bitmask.cpp.gcda
CMakeFiles/RaptorQ_Static_Profiling.dir/src/RaptorQ/v1/Thread_Pool.cpp.gcda
CMakeFiles/RaptorQ_Static_Profiling.dir/src/RaptorQ/v1/API_Wrapper_C.cpp.gcda)
        set(PROFILE_SET_FILE      ${PROFILE_GET_FILE})
        set(PROFILE_SET           -fprofile-use)
    endif()

    # PRE-run: build a library, generate the profile.
    add_library(RaptorQ_Static_Profiling STATIC ${SOURCES} ${HEADERS} ${HEADERS_LINKED})
    target_link_libraries(RaptorQ_Static_Profiling ${STDLIB} ${RQ_LZ4_DEP} ${RQ_UBSAN})
    add_dependencies(RaptorQ_Static_Profiling LZ4)
    target_compile_options(
        RaptorQ_Static_Profiling PRIVATE
        ${CXX_COMPILER_FLAGS}
        ${PROFILE_GET}
    )
    set_target_properties(RaptorQ_Static_Profiling PROPERTIES COMPILER_FLAGS ${PROFILE_GET})

    # build the C example test for profiling
    add_executable(test_c_profiled test/test_c.c)
    add_dependencies(test_c_profiled RaptorQ_Static_Profiling)
    target_compile_options(
        test_c_profiled PRIVATE
        ${C_COMPILER_FLAGS}
        ${PROFILE_GET}
    )
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
        target_link_libraries(test_c_profiled RaptorQ_Static_Profiling ${STDLIB} m ${CMAKE_THREAD_LIBS_INIT})
    else()
        target_link_libraries(test_c_profiled RaptorQ_Static_Profiling ${STDLIB}   ${CMAKE_THREAD_LIBS_INIT})
    endif()
    set_target_properties(test_c_profiled PROPERTIES LINK_FLAGS ${PROFILE_GET})

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_custom_command(
            OUTPUT ${PROFILE_GET_FILE}
            COMMAND LLVM_PROFILE_FILE=${PROFILE_GET_FILE} ./test_c_profiled
            DEPENDS test_c_profiled
            COMMENT "Running profiling test..."
            VERBATIM
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_custom_command(
            OUTPUT ${PROFILE_GET_FILE}
            COMMAND ./test_c_profiled
            DEPENDS test_c_profiled
            COMMENT "Running profiling test..."
            VERBATIM
        )
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # the profile must be translated into readable form
        add_custom_command(
            OUTPUT ${PROFILE_SET_FILE}
            COMMAND llvm-profdata merge -output=${PROFILE_SET_FILE} ${PROFILE_GET_FILE}
            DEPENDS ${PROFILE_GET_FILE}
            COMMENT "Creating profile data..."
            VERBATIM
        )
    endif()
    add_custom_target(
        profile ALL
        DEPENDS ${PROFILE_SET_FILE}
    )
endif()



# build the static library
if(STATIC_LIB MATCHES "ON")
    add_library(RaptorQ_Static STATIC ${SOURCES} ${HEADERS} ${HEADERS_LINKED})
    add_dependencies(RaptorQ_Static LZ4)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
        set_target_properties(RaptorQ_Static PROPERTIES OUTPUT_NAME RaptorQ.${RQ_ABI})
    endif()
    target_compile_options(
        RaptorQ_Static PRIVATE
        ${CXX_COMPILER_FLAGS}
    )
    if(LTO MATCHES "ON")
        target_compile_options(RaptorQ_Static PRIVATE -flto)
    endif()
    if(PROFILING MATCHES "ON")
        add_dependencies(RaptorQ_Static profile)
        target_compile_options(RaptorQ_Static PRIVATE ${PROFILE_SET})
    endif()
    if (NOT RQ_LINKER MATCHES BSD)
        set_target_properties (
            RaptorQ_Static
            PROPERTIES
            LINK_FLAGS "-Wl,-z,now,-z,relro${RQ_LZ4_EXCLUDE_SYM} -pie ${LD_OPT}"
        )
    endif()
    set_property(TARGET RaptorQ_Static PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
    if(CMAKE_SYSTEM_NAME MATCHES "Windows")
        # not really deterministic?
        # don't know how to do that in Windows.
        add_custom_target(
            make_static_deterministic ALL
            DEPENDS RaptorQ_Static
        )
    else()
        # make the library deterministic (only on *nix)
        add_executable(make_deterministic src/deterministic.cpp)
        target_compile_options(
            make_deterministic PRIVATE
            ${CXX_COMPILER_FLAGS}
        )
        target_link_libraries(make_deterministic ${STDLIB} ${RQ_UBSAN})
        add_custom_command(
            OUTPUT deterministic.run
            COMMAND make_deterministic ${CMAKE_CURRENT_BINARY_DIR}/lib/libRaptorQ.${RQ_ABI}.a
            DEPENDS RaptorQ_Static
            COMMENT "Removing creation date from library..."
            VERBATIM
        )
        add_custom_target(
            make_static_deterministic ALL
            DEPENDS deterministic.run
        )
    endif()
endif()

#build dynamic library
if(DYNAMIC_LIB MATCHES "ON")
    add_definitions(-DRAPTORQ_DLL)
    add_definitions(-DRAPTORQ_DLL_EXPORTS)

    add_library(RaptorQ SHARED ${SOURCES} ${HEADERS} ${HEADERS_LINKED})
    add_dependencies(RaptorQ LZ4)
    target_link_libraries(RaptorQ ${RQ_LZ4_DEP})
    target_compile_options(
        RaptorQ PRIVATE
        ${CXX_COMPILER_FLAGS}
    )
    if(LTO MATCHES "ON")
        target_compile_options(RaptorQ PRIVATE -flto)
    endif()
    if(PROFILING MATCHES "ON")
        add_dependencies(RaptorQ profile)
        target_compile_options(RaptorQ PRIVATE ${PROFILE_SET})
    endif()
    if (RQ_LINKER MATCHES BSD)
        # keep the last sace at the end here, otherwise cmake gets called
        # with the wrong number of parameters in the next set_target_properties
        set(RQ_LINK_LIB "${LD_OPT} ")
    else()
        set(RQ_LINK_LIB "-Wl,-z,now,-z,relro${RQ_LZ4_EXCLUDE_SYM} ${LD_OPT}")
    endif()
    set_target_properties (
        RaptorQ
        PROPERTIES
        LINK_FLAGS ${RQ_LINK_LIB}
        SOVERSION ${RQ_ABI}
        VERSION ${RQ_VERSION}
    )
    set_property(TARGET RaptorQ PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
endif()


# benchamrks (header only)
add_executable(libRaptorQ-test EXCLUDE_FROM_ALL test/rfc_test.cpp ${HEADERS_ONLY} ${HEADERS})
target_compile_options(
    libRaptorQ-test PRIVATE
    ${CXX_COMPILER_FLAGS}
)
target_link_libraries(libRaptorQ-test ${RQ_UBSAN} ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${RQ_LZ4_DEP})

# build examples
# C interface (linked)
add_executable(test_c EXCLUDE_FROM_ALL test/test_c.c)
target_compile_options(
    test_c PRIVATE
    ${C_COMPILER_FLAGS}
)

add_dependencies(test_c RaptorQ)
if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
    # Linux, *BSD
    target_link_libraries(test_c RaptorQ m ${RQ_UBSAN} ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${RQ_LZ4_DEP})
else()
    # windows: no "m" library
    target_link_libraries(test_c RaptorQ   ${RQ_UBSAN} ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${RQ_LZ4_DEP})
endif()

# CPP interface - RFC interface (header only)
add_executable(test_cpp_rfc EXCLUDE_FROM_ALL test/test_cpp_rfc.cpp ${HEADERS_ONLY} ${HEADERS})
target_compile_options(
    test_cpp_rfc PRIVATE
    ${CXX_COMPILER_FLAGS} "-DTEST_HDR_ONLY"
)
target_link_libraries(test_cpp_rfc ${RQ_UBSAN} ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${RQ_LZ4_DEP})

# CPP interface - RFC interface (linked version)
add_executable(test_cpp_rfc_linked EXCLUDE_FROM_ALL test/test_cpp_rfc.cpp ${HEADERS_ONLY} ${HEADERS})
target_compile_options(
    test_cpp_rfc_linked PRIVATE
    ${CXX_COMPILER_FLAGS}
)
add_dependencies(test_cpp_rfc_linked RaptorQ)
target_link_libraries(test_cpp_rfc_linked RaptorQ ${RQ_UBSAN} ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${RQ_LZ4_DEP})

# CPP interface - RAW interface (header only)
add_executable(test_cpp_raw EXCLUDE_FROM_ALL test/test_cpp_raw.cpp ${HEADERS_ONLY} ${HEADERS})
target_compile_options(
    test_cpp_raw PRIVATE
    ${CXX_COMPILER_FLAGS} "-DTEST_HDR_ONLY"
)
target_link_libraries(test_cpp_raw RaptorQ ${RQ_UBSAN} ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${RQ_LZ4_DEP})

# CPP interface - RAW interface (linked version)
add_executable(test_cpp_raw_linked EXCLUDE_FROM_ALL test/test_cpp_raw.cpp)
target_compile_options(
    test_cpp_raw_linked PRIVATE
    ${CXX_COMPILER_FLAGS}
)
add_dependencies(test_cpp_raw_linked RaptorQ)
target_link_libraries(test_cpp_raw_linked RaptorQ ${RQ_UBSAN} ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${RQ_LZ4_DEP})

# CLI tool - RAW API interface (header only)
set(CLI_raw_sources src/cli/RaptorQ.cpp external/optionparser-1.4/optionparser.h ${HEADERS} ${HEADERS_ONLY})
if(CLI MATCHES "ON")
    add_executable(CLI_raw ${CLI_raw_sources})
    add_custom_target(CLI_tools DEPENDS CLI_raw)
else()
    add_executable(CLI_raw EXCLUDE_FROM_ALL ${CLI_raw_sources})
    add_custom_target(CLI_tools "")
endif()
target_compile_options(
    CLI_raw PRIVATE
    ${CXX_COMPILER_FLAGS}
)
set_target_properties(CLI_raw PROPERTIES OUTPUT_NAME RaptorQ
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
target_link_libraries(CLI_raw ${RQ_UBSAN} ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${RQ_LZ4_DEP})

#### EXAMPLES

# CPP interface - RAW interface (header only)
add_executable(example_cpp_raw EXCLUDE_FROM_ALL examples/example_cpp_raw.cpp ${HEADERS_ONLY} ${HEADERS})
target_compile_options(
    example_cpp_raw PRIVATE
    ${CXX_COMPILER_FLAGS}
)
target_link_libraries(example_cpp_raw ${RQ_UBSAN} ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${RQ_LZ4_DEP})

add_custom_target(examples DEPENDS test_c test_cpp_rfc test_cpp_rfc_linked test_cpp_raw test_cpp_raw_linked libRaptorQ-test example_cpp_raw)



add_custom_target(everything DEPENDS make_static_deterministic examples docs CLI_tools)

install(FILES ${HEADERS} ${HEADERS_ONLY} ${HEADERS_LINKED} DESTINATION include/RaptorQ/)
install(TARGETS RaptorQ RaptorQ_Static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib COMPONENT libraries)
install(TARGETS CLI_raw
    RUNTIME DESTINATION bin COMPONENT runtime)
