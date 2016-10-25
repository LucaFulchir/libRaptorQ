#
# Copyright (c) 2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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


if(RQ_BUILD_LZ4)
    #
    # target: LZ4 static library
    #
    set(RQ_LZ4_VERSION_MAJOR 1)
    set(RQ_LZ4_VERSION_MINOR 5)
    set(RQ_LZ4_VERSION_PATCH r128)
    set(RQ_LZ4_VERSION_STRING " \"${RQ_LZ4_VERSION_MAJOR}.${RQ_LZ4_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}\" ")

    set(RQ_LZ4_DIR ${PROJECT_SOURCE_DIR}/external/lz4/lib/)
    set(RQ_LZ4_SRCS_LIB ${RQ_LZ4_DIR}lz4.c
                        ${RQ_LZ4_DIR}lz4hc.c
                        ${RQ_LZ4_DIR}lz4.h
                        ${RQ_LZ4_DIR}lz4hc.h
                        ${RQ_LZ4_DIR}lz4frame.c
                        ${RQ_LZ4_DIR}lz4frame.h
                        ${RQ_LZ4_DIR}xxhash.c)

    add_library(rq_lz4_static STATIC ${RQ_LZ4_SRCS_LIB})
    include(CheckCCompilerFlag)
    check_c_compiler_flag("-std=c99" RQ_FLAG_C_99)
    if(RQ_FLAG_C_99)
        target_compile_options(
            rq_lz4_static PRIVATE
            "-std=c99"
        )
    endif()
    set_target_properties(rq_lz4_static PROPERTIES
        SOVERSION "${RQ_LZ4_VERSION_MAJOR}.${RQ_LZ4_VERSION_MINOR}")
    #set_property(TARGET lz4_static PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
    ADD_DEFINITIONS("-DLZ4_VERSION=\"${RQ_LZ4_VERSION_PATCH}\"")
    INCLUDE_DIRECTORIES (${RQ_LZ4_DIR})


    # set the dependency and build it all
    if(CMAKE_SYSTEM_NAME MATCHES "Windows")
        add_custom_target(LZ4 DEPENDS rq_lz4_static)
        set(RQ_LZ4_DEP rq_lz4_static)
    else()
        add_custom_command(
            OUTPUT lz4_deterministic.run
            COMMAND make_deterministic ${CMAKE_CURRENT_BINARY_DIR}/liblz4.a
            DEPENDS lz4_static make_deterministic
            COMMENT "Removing creation date from lz4 library..."
            VERBATIM
        )
        add_library(rq_lz4 STATIC IMPORTED)
        set_target_properties(rq_lz4 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/liblz4.a)
        add_custom_target(LZ4 DEPENDS lz4_deterministic.run)
        set(RQ_LZ4_DEP rq_lz4_static)
    endif()
else()
    add_custom_target(LZ4)
    if (USE_LZ4 MATCHES "ON")
        set(RQ_LZ4_DEP ${RQ_LZ4_LIB})
    endif()
endif()
