#
# Copyright (c) 2015-2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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


if(NOT RQ_LZ4_FOUND)
    # build lz4, then it will be statically linked
    # in the library
    ExternalProject_Add(LZ4
        PREFIX lz4/lib/
        BINARY_DIR lz4/lib/build/
        INSTALL_COMMAND ""  # nothing to install.
    )
    set(RQ_LZ4_DEP lz4/lib/build/liblz4.a)
endif()

#ExternalProject_Add(<name>    # Name for custom target
#    [DEPENDS projects...]       # Targets on which the project depends
#    [PREFIX dir]                # Root dir for entire project
#    [LIST_SEPARATOR sep]        # Sep to be replaced by ; in cmd lines
#    [TMP_DIR dir]               # Directory to store temporary files
#    [STAMP_DIR dir]             # Directory to store step timestamps
#   #--Download step--------------
#    [DOWNLOAD_NAME fname]       # File name to store (if not end of URL)
#    [DOWNLOAD_DIR dir]          # Directory to store downloaded files
#    [DOWNLOAD_COMMAND cmd...]   # Command to download source tree
#    [CVS_REPOSITORY cvsroot]    # CVSROOT of CVS repository
#    [CVS_MODULE mod]            # Module to checkout from CVS repo
#    [CVS_TAG tag]               # Tag to checkout from CVS repo
#    [SVN_REPOSITORY url]        # URL of Subversion repo
#    [SVN_REVISION rev]          # Revision to checkout from Subversion repo
#    [SVN_USERNAME john ]        # Username for Subversion checkout and update
#    [SVN_PASSWORD doe ]         # Password for Subversion checkout and update
#    [SVN_TRUST_CERT 1 ]         # Trust the Subversion server site certificate
#    [GIT_REPOSITORY url]        # URL of git repo
#    [GIT_TAG tag]               # Git branch name, commit id or tag
#    [HG_REPOSITORY url]         # URL of mercurial repo
#    [HG_TAG tag]                # Mercurial branch name, commit id or tag
#    [URL /.../src.tgz]          # Full path or URL of source
#    [URL_HASH ALGO=value]       # Hash of file at URL
#    [URL_MD5 md5]               # Equivalent to URL_HASH MD5=md5
#    [TLS_VERIFY bool]           # Should certificate for https be checked
#    [TLS_CAINFO file]           # Path to a certificate authority file
#    [TIMEOUT seconds]           # Time allowed for file download operations
#   #--Update/Patch step----------
#    [UPDATE_COMMAND cmd...]     # Source work-tree update command
#    [PATCH_COMMAND cmd...]      # Command to patch downloaded source
#   #--Configure step-------------
#    [SOURCE_DIR dir]            # Source dir to be used for build
#    [CONFIGURE_COMMAND cmd...]  # Build tree configuration command
#    [CMAKE_COMMAND /.../cmake]  # Specify alternative cmake executable
#    [CMAKE_GENERATOR gen]       # Specify generator for native build
#    [CMAKE_GENERATOR_TOOLSET t] # Generator-specific toolset name
#    [CMAKE_ARGS args...]        # Arguments to CMake command line
#    [CMAKE_CACHE_ARGS args...]  # Initial cache arguments, of the form -Dvar:string=on
#   #--Build step-----------------
#    [BINARY_DIR dir]            # Specify build dir location
#    [BUILD_COMMAND cmd...]      # Command to drive the native build
#    [BUILD_IN_SOURCE 1]         # Use source dir for build dir
#   #--Install step---------------
#    [INSTALL_DIR dir]           # Installation prefix
#    [INSTALL_COMMAND cmd...]    # Command to drive install after build
#   #--Test step------------------
#    [TEST_BEFORE_INSTALL 1]     # Add test step executed before install step
#    [TEST_AFTER_INSTALL 1]      # Add test step executed after install step
#    [TEST_COMMAND cmd...]       # Command to drive test
#   #--Output logging-------------
#    [LOG_DOWNLOAD 1]            # Wrap download in script to log output
#    [LOG_UPDATE 1]              # Wrap update in script to log output
#    [LOG_CONFIGURE 1]           # Wrap configure in script to log output
#    [LOG_BUILD 1]               # Wrap build in script to log output
#    [LOG_TEST 1]                # Wrap test in script to log output
#    [LOG_INSTALL 1]             # Wrap install in script to log output
#   #--Custom targets-------------
#    [STEP_TARGETS st1 st2 ...]  # Generate custom targets for these steps
#    )
