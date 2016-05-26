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
)

message(STATUS "Found lz4 at: ${RQ_LZ4_INCLUDE_DIR}.")

IF(RQ_LZ4_INCLUDE_DIR)
  SET(RQ_LZ4_FOUND TRUE)
ELSE(RQ_LZ4_INCLUDE_DIR)
  SET(RQ_LZ4_FOUND FALSE)
ENDIF(RQ_LZ4_INCLUDE_DIR)

IF(RQ_LZ4_FOUND)
  MESSAGE(STATUS "Found lz4 in ${RQ_LZ4_INCLUDE_DIR}")
ELSE(RQ_LZ4_FOUND)
  IF(RQ_LZ4_FIND_REQUIRED)
    MESSAGE(STATUS "Could not find \"lz4\" library. We'll build our own...")
    MESSAGE(STATUS "Found lz4 in ${PROJECT_SOURCE_DIR}/external/lz4/lib")
    SET(RQ_LZ4_INCLUDE_DIR   ${PROJECT_SOURCE_DIR}/external/lz4/lib)
  ENDIF(RQ_LZ4_FIND_REQUIRED)
ENDIF(RQ_LZ4_FOUND)

MARK_AS_ADVANCED(
  RQ_LZ4_INCLUDE_DIR
)
