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
FIND_PROGRAM(GIT_EXECUTABLE git
    PATHS
        ${PATH}
        "C:/Program Files/Git/bin"
        "C:/Program Files (x86)/Git/bin"
    DOC "git command line client"
)

IF(GIT_EXECUTABLE)
    SET(GIT_FOUND TRUE)
ELSE(GIT_EXECUTABLE)
    SET(GIT_FOUND FALSE)
ENDIF(GIT_EXECUTABLE)

IF(GIT_FOUND)
    MESSAGE(STATUS "Found git in ${GIT_EXECUTABLE}")
ELSE(GIT_FOUND)
    IF(GIT_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find \"git\" command")
    ENDIF(GIT_REQUIRED)
ENDIF(GIT_FOUND)

MARK_AS_ADVANCED(
    GIT_EXECUTABLE
)



