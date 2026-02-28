# This is the top level CMakeLists.txt file.

cmake_minimum_required(VERSION 2.8)
set(BUILD_SHARED_LIBS ON)
project(GT)
set(GT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/source/include")

# The command project(<PROJECT-NAME>) sets following variables:
# PROJECT_SOURCE_DIR
# PROJECT_BINARY_DIR
# <PROJECT-NAME>_SOURCE_DIR
# <PROJECT-NAME>_BINARY_DIR
#message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	message(STATUS "  (for debug, do cmake -DCMAKE_BUILD_TYPE=Debug)")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake")
find_package(ROOT COMPONENTS Gui)
find_package(Boost COMPONENTS thread filesystem)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Machine depenent settings.
if("$ENV{HOSTNAME}" STREQUAL "smrc-chi22.ronin.roncap.com")
  set(CMAKE_INSTALL_PREFIX "/home/mercator1/paramjobs/gtbin")
else()
  set(CMAKE_INSTALL_PREFIX "/home/jelee/gtbin")
endif()

include_directories(${GT_INCLUDE_DIR})
include_directories(${ROOT_INCLUDE_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib64")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

# Avoid using LD_LIBRARY_PATH.
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN/../lib64/")

# -w to inhibit all warnings.
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "-g -w -std=c++11 -lcnpy -lz")
else()
	set(CMAKE_CXX_FLAGS "-g -w -std=c++11 -flto -lcnpy -lz")
endif()

add_subdirectory(source/src)
add_subdirectory(source/bin)
