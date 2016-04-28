#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra -O2 -fno-strict-aliasing")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_REENTRANT -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFUSE_USE_VERSION=26")

enable_testing()

# Define "make check" as an alias for "make test."
add_custom_target(check COMMAND ctest)

function(FLATTEN_LIST INPUT SEPARATOR OUTPUT)
    string (REPLACE ";" "${SEPARATOR}" _TMPS "${INPUT}")
    set (${OUTPUT} "${_TMPS}" PARENT_SCOPE)
endfunction()

# Find Linux FUSE
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE fuse)
IF(FUSE_FOUND)
    FLATTEN_LIST("${FUSE_CFLAGS}" " " FUSE_CFLAGS)
    FLATTEN_LIST("${FUSE_LDFLAGS}" " " FUSE_LDFLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FUSE_CFLAGS}")
    set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} ${FUSE_LDFLAGS}")
    MESSAGE(STATUS "Building Linux FUSE client.")
    include_directories(${FUSE_INCLUDE_DIRS})
ELSE(FUSE_FOUND)
    MESSAGE(FATAL_ERROR "Failed to find Linux FUSE libraries or include files.")
ENDIF(FUSE_FOUND)

add_executable(iohub
    file.c
    fs.c
    htable.c
    log.c
    meta.c
    throttle.c
    util.c
)
target_link_libraries(iohub
    ${FUSE_LIBRARIES}
    m
    pthread
    rt
)

macro(add_utest utest)
    add_test(${utest} ${CMAKE_CURRENT_BINARY_DIR}/${utest} ${utest})
endmacro(add_utest)

add_library(utest
    test.c
)
target_link_libraries(utest pthread)

add_executable(util_unit
    util_unit.c 
    util.c
    test.c
)
target_link_libraries(util_unit utest)
add_utest(util_unit)

add_executable(htable_unit
    htable_unit.c 
    htable.c
    test.c
)
target_link_libraries(htable_unit utest)
add_utest(htable_unit)
