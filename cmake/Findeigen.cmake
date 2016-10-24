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

FIND_PATH(EIGEN3_INCLUDE_DIR
    NAMES Eigen
    PATH_SUFFIXES include/ include/eigen3/ eigen3/
    PATHS
    ${EIGEN3_ROOT}
    $ENV{EIGEN3_ROOT}
    /usr/
    /usr/local/
    ${PROJECT_SOURCE_DIR}/external/eigen3
)

IF(EIGEN3_INCLUDE_DIR)
    SET(EIGEN3_FOUND TRUE)
ELSE(EIGEN3_INCLUDE_DIR)
    SET(EIGEN3_FOUND FALSE)
ENDIF(EIGEN3_INCLUDE_DIR)

IF(EIGEN3_FOUND)
    MESSAGE(STATUS "Found eigen3 in ${EIGEN3_INCLUDE_DIR}")
ELSE(EIGEN3_FOUND)
    IF(EIGEN3_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find \"eigen3\" library")
    ENDIF(EIGEN3_FIND_REQUIRED)
ENDIF(EIGEN3_FOUND)

MARK_AS_ADVANCED(
    EIGEN3_INCLUDE_DIR
)

