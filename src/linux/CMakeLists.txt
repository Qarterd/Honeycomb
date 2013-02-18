cmake_minimum_required(VERSION 2.8)
project(Honeycomb)
set(Honeycomb_versionMajor 1)
set(Honeycomb_versionMinor 0)

include(pch.cmake)
set(_ "${CMAKE_TOOLCHAIN_FILE}") #ignore warning

#curl
find_path(curl_root NAMES include/curl/curl.h)
find_library(curl_lib NAMES curl HINTS ${curl_root}/lib)
find_path(curl_inc NAMES curl/curl.h HINTS ${curl_root}/include)

#yajl
set(yajl_inc ../../ext/yajl/src/api)
file(GLOB_RECURSE yajl_src ../../ext/yajl/src/*.c)
set_source_files_properties(${yajl_src} PROPERTIES LANGUAGE CXX COMPILE_FLAGS "-x c++")

#lib
set(Honeycomb Honeycomb-${Honeycomb_versionMajor}.${Honeycomb_versionMinor}-${CMAKE_BUILD_TYPE})
file(GLOB_RECURSE src_common ../common/*.cpp)
file(GLOB_RECURSE src_linux Honey/*.cpp)
include_directories(. ../common ${yajl_inc} ${curl_inc})
add_library(${Honeycomb} ${yajl_src} ${src_common} ${src_linux})
dependPch(${Honeycomb} Honeycomb.pch)
target_link_libraries(${Honeycomb} pthread ${curl_lib})
install(DIRECTORY ../common/Honey DESTINATION inc FILES_MATCHING PATTERN *.h)
install(DIRECTORY Honey DESTINATION inc FILES_MATCHING PATTERN *.h)
install(TARGETS ${Honeycomb} DESTINATION lib)

#test
file(GLOB_RECURSE src_test ../../test/*.cpp)
add_executable(test ${src_test} test.cpp)
dependPch(test Honeycomb.pch)
target_link_libraries(test ${Honeycomb})
install(TARGETS test DESTINATION bin)
