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


# with a lot of thanks to the gcc guys for making the search of the path so easy and obvious.
# path is like:
#/usr/lib/gcc/x86_64-unknown-linux-gnu/4.9.0/include/stddef.h

IF(${ARCH} MATCHES "X86_64")
	SET(GCC_ARCH "x86_64")
ELSE()
  IF(${ARCH} MATCHES "ARM")
    SET(GCC_ARCH "arm")
  ELSE()
    #default
    SET(GCC_ARCH "x86")
  ENDIF()
ENDIF()

file(GLOB arch-dir /usr/lib/gcc/${GCC_ARCH}*gnu)
foreach(arch ${arch-dir})
	if(IS_DIRECTORY ${arch})
		file(GLOB sub-dir ${arch}/[0-9]*.[0-9]*.[0-9]*)
		foreach(dir ${sub-dir})
			if(IS_DIRECTORY ${dir}/include/)
				set(GCC_C_PATH_SUFFIXES ${GCC_C_PATH_SUFFIXES};${dir}/include/)
			endif()
		endforeach()
	endif()
endforeach()

FIND_PATH(GCC_C_INCLUDE_DIR
  NAMES "stddef.h"
  PATH_SUFFIXES / ${GCC_C_PATH_SUFFIXES}
  PATHS "${PROJECT_SOURCE_DIR}/../gcc_c"
  ${GCC_C_ROOT}
  $ENV{GCC_C_ROOT}
  /
)


message("Found gcc C library at: ${GCC_C_INCLUDE_DIR}.")

IF(GCC_C_INCLUDE_DIR)
  SET(GCC_C_FOUND TRUE)
ELSE(GCC_C_INCLUDE_DIR)
  SET(GCC_C_FOUND FALSE)
ENDIF(GCC_C_INCLUDE_DIR)

IF(GCC_C_FOUND)
  MESSAGE(STATUS "Found gcc C library in ${GCC_C_INCLUDE_DIR}")
ELSE(GCC_C_FOUND)
  IF(GCC_C_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find the gcc C library")
  ENDIF(GCC_C_FIND_REQUIRED)
ENDIF(GCC_C_FOUND)

MARK_AS_ADVANCED(
  GCC_C_INCLUDE_DIR
)

