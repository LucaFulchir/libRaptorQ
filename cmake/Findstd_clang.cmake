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

FIND_PATH(CLANG_STD_INCLUDE_DIR
  NAMES unordered_map
  PATH_SUFFIXES include/c++ include/c++/v1
  PATHS "${PROJECT_SOURCE_DIR}/../libc++/"
  ${CLANG_STD_ROOT}
  $ENV{CLANG_STD_ROOT}
  /usr/local/
  /usr/
)

message("Found clang stdandard library at: ${CLANG_STD_INCLUDE_DIR}.")

IF(CLANG_STD_INCLUDE_DIR)
  SET(CLANG_STD_FOUND TRUE)
ELSE(CLANG_STD_INCLUDE_DIR)
  SET(CLANG_STD_FOUND FALSE)
ENDIF(CLANG_STD_INCLUDE_DIR)

IF(CLANG_STD_FOUND)
  MESSAGE(STATUS "Found clang standard library in ${CLANG_STD_INCLUDE_DIR}")
ELSE(CLANG_STD_FOUND)
  IF(CLANG_STD_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find the clang standard library")
  ENDIF(CLANG_STD_FIND_REQUIRED)
ENDIF(CLANG_STD_FOUND)

MARK_AS_ADVANCED(
  CLANG_STD_INCLUDE_DIR
)

