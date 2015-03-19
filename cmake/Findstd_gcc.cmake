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

