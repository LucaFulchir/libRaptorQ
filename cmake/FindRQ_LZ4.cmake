#
# Copyright (c) 2015, Luca Fulchir<luca@fulchir.it>, All rights reserved.
#
# This file is part of libRaptorQ.
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
#

FIND_PATH(RQ_LZ4_INCLUDE_DIR
    NAMES lz4.h
    PATH_SUFFIXES include/ include/lz4/ lz4/
    PATHS
    ${LZ4_ROOT}
    $ENV{LZ4_ROOT}
    /usr/
    /usr/local/
    ${CMAKE_CURRENT_SOURCE_DIR}/external/lz4/lib
)

FIND_LIBRARY(RQ_LZ4_LIB
    NAMES lz4
    PATH_SUFFIXES lib/
    PATHS
    ${LZ4_ROOT}
    $ENV{LZ4_ROOT}
    /usr/
    /usr/local/
)

IF(RQ_LZ4_INCLUDE_DIR)
    SET(RQ_LZ4_FOUND TRUE)
ELSE(RQ_LZ4_INCLUDE_DIR)
    SET(RQ_LZ4_FOUND FALSE)
ENDIF(RQ_LZ4_INCLUDE_DIR)

IF(RQ_LZ4_FOUND)
    IF(RQ_LZ4_USE_OWN)
        # force our own lz4 libary
        SET(RQ_LZ4_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/lz4/lib)
    ENDIF()
    MESSAGE(STATUS "Found lz4 in ${RQ_LZ4_INCLUDE_DIR}")
    # we need both headers and library, or we use our own
    IF(RQ_LZ4_INCLUDE_DIR MATCHES "${CMAKE_CURRENT_SOURCE_DIR}/external/lz4/lib" OR RQ_LZ4_LIB MATCHES "RQ_LZ4_LIB-NOTFOUND")
        MESSAGE(WARNING "We will build our own lz4 library and statically link it.")
        SET(RQ_BUILD_LZ4 TRUE)
    ENDIF()
ELSE(RQ_LZ4_FOUND)
    IF(RQ_LZ4_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find \"lz4\" library."
                            "  Please install the \"lz4\" library"
                            " or at least run:\n"
                            "  cd ${CMAKE_CURRENT_SOURCE_DIR}\n"
                            "  git submodule init\n"
                            "  git submodule update\n")
    ENDIF(RQ_LZ4_FIND_REQUIRED)
ENDIF(RQ_LZ4_FOUND)

MARK_AS_ADVANCED(
    RQ_LZ4_INCLUDE_DIR
)
