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
    # build lz4, then it will be statically linked in the library
	include(ExternalProject)
	set(RQ_LZ4_BUILDNAME lz4_static)
    ExternalProject_Add(${RQ_LZ4_BUILDNAME}
		GIT_REPOSITORY file://${PROJECT_SOURCE_DIR}/external/lz4
        GIT_TAG d86dc916771c126afb797637dda9f6421c0cb998
		CONFIGURE_COMMAND ""
		BUILD_COMMAND make lib
		BUILD_IN_SOURCE 1
		INSTALL_COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/${RQ_LZ4_BUILDNAME}-prefix/src/${RQ_LZ4_BUILDNAME}/lib/liblz4.a ${CMAKE_CURRENT_BINARY_DIR}
    )

	add_custom_command(
		OUTPUT lz4_deterministic.run
		COMMAND make_deterministic ${CMAKE_CURRENT_BINARY_DIR}/liblz4.a
		DEPENDS ${RQ_LZ4_BUILDNAME} make_deterministic
		COMMENT "Removing creation date from lz4 library..."
		VERBATIM
	)
	add_library(rq_lz4 STATIC IMPORTED)
	set_target_properties(rq_lz4 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/liblz4.a)
	add_custom_target(LZ4 DEPENDS lz4_deterministic.run)
	set(RQ_LZ4_DEP rq_lz4)
else()
	add_custom_target(LZ4)
    if (USE_LZ4 MATCHES "ON")
        set(RQ_LZ4_DEP ${RQ_LZ4_LIB})
    endif()
endif()
