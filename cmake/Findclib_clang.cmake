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

file(GLOB sub-dir /usr/lib/clang/[0-9]*.[0-9]*.[0-9]*)
foreach(dir ${sub-dir})
    if(IS_DIRECTORY ${dir}/include/)
        set(CLANG_C_PATH_SUFFIXES ${CLANG_C_PATH_SUFFIXES};${dir}/include/)
    endif()
endforeach()

FIND_PATH(CLANG_C_INCLUDE_DIR
  NAMES stddef.h
  PATH_SUFFIXES / ${CLANG_C_PATH_SUFFIXES}
  PATHS "${PROJECT_SOURCE_DIR}/../clang_c"
  ${CLANG_C_ROOT}
  $ENV{CLANG_C_ROOT}
  /
)


message("Found clang C library at: ${CLANG_C_INCLUDE_DIR}.")

IF(CLANG_C_INCLUDE_DIR)
  SET(CLANG_C_FOUND TRUE)
ELSE(CLANG_C_INCLUDE_DIR)
  SET(CLANG_C_FOUND FALSE)
ENDIF(CLANG_C_INCLUDE_DIR)

IF(CLANG_C_FOUND)
  MESSAGE(STATUS "Found clang C library in ${CLANG_C_INCLUDE_DIR}")
ELSE(CLANG_C_FOUND)
  IF(CLANG_C_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find the clang C library")
  ENDIF(CLANG_C_FIND_REQUIRED)
ENDIF(CLANG_C_FOUND)

MARK_AS_ADVANCED(
  CLANG_C_INCLUDE_DIR
)

