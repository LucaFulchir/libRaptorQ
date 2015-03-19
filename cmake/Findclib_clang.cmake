
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

