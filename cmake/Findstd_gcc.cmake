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

file(GLOB sub-dir /usr/include/c++/[0-9].[0-9]*.[0-9]*)
foreach(dir ${sub-dir})
    if(IS_DIRECTORY ${dir})
        set(GCC_PATH_SUFFIXES ${GCC_PATH_SUFFIXES};${dir})
    endif()
endforeach()

FIND_PATH(GCC_STD_INCLUDE_DIR
  NAMES unordered_map
  PATH_SUFFIXES / ${GCC_PATH_SUFFIXES}
  PATHS "${PROJECT_SOURCE_DIR}/../stdlib/"
  ${GCC_STD_ROOT}
  $ENV{GCC_STD_ROOT}
  /
)


message("Found gcc stdandard library at: ${GCC_STD_INCLUDE_DIR}.")

IF(GCC_STD_INCLUDE_DIR)
  SET(GCC_STD_FOUND TRUE)
ELSE(GCC_STD_INCLUDE_DIR)
  SET(GCC_STD_FOUND FALSE)
ENDIF(GCC_STD_INCLUDE_DIR)

IF(GCC_STD_FOUND)
  MESSAGE(STATUS "Found gcc standard library in ${GCC_STD_INCLUDE_DIR}")
ELSE(GCC_STD_FOUND)
  IF(GCC_STD_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find the gcc standard library")
  ENDIF(GCC_STD_FIND_REQUIRED)
ENDIF(GCC_STD_FOUND)

MARK_AS_ADVANCED(
  GCC_STD_INCLUDE_DIR
)

