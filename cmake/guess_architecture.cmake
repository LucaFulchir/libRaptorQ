#
# Copyright (c) 2015-2018, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

# Now we try to guess the architecture... this will be fun... sigh..


# list of linux architectures (might be old, not all will be supported):
# i386 i686 x86_64 ia64 alpha amd64
# arm armeb armel hppa m32r m68k mips mipsel
# powerpc ppc64 s390 s390x sh3 sh3eb sh4 sh4eb sparc


SET(ARCH "X86_64" CACHE STRING "Processor architecture") # defult, most common
SET_PROPERTY(CACHE ARCH PROPERTY STRINGS X86_64 X86 ARM)

IF(ARCH)
	# you provided us the right architecture? good ^^
ELSE()
  #we need to guess it somehow...
  IF(WIN32)
    SET(ARCH "X86_64")
  ENDIF()
  IF(APPLE)
    EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCH )
  ENDIF()
  IF(UNIX)
    EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCH )
    IF(${ARCH} STREQUAL "x86_64" OR ${ARCH} STREQUAL "amd64" OR ${ARCH} STREQUAL "ia64")
      SET(ARCH, "X86_64")
    ELSE()
      IF(${ARCH} STREQUAL "x86" OR ${ARCH} STREQUAL "i686")
	SET(ARCH, "X86")
      ELSE()
	IF(${ARCH} STREQUAL "arm" OR ${ARCH} STREQUAL "armeb" OR ${ARCH} STREQUAL "armel")
	  SET(ARCH, "ARM")
	ELSE()
	ENDIF()
      ENDIF()
    ENDIF()
  ENDIF()
ENDIF()

