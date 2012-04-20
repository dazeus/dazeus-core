cmake_minimum_required(VERSION 2.8)

project(dazeus)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/" "${CMAKE_SOURCE_DIR}/contrib/libdazeus-irc/cmake/")
set(DAZEUS_VERSION_MAJOR "2")
set(DAZEUS_VERSION_MINOR "0")
set(DAZEUS_VERSION "${DAZEUS_VERSION_MAJOR}.${DAZEUS_VERSION_MINOR}")

IF(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/libdazeus-irc/CMakeLists.txt")
	message("It seems you do not have a checkout of libdazeus-irc.")
	message("It is required to compile libdazeus, and can be automatically")
	message("created by Git by running:")
	message("  git submodule update --init")
	message(FATAL_ERROR "libdazeus-irc is not checked out")
ENDIF()

find_package(LibJson REQUIRED)
find_package(LibMongoClient REQUIRED)

include_directories(${LibJson_INCLUDE_DIRS})
include_directories(${LibMongoClient_INCLUDE_DIRS})
include_directories("${CMAKE_SOURCE_DIR}/contrib/libdazeus-irc/src")

set(LIBS ${LIBS} ssl crypto dazeus-irc ${LibIRCClient_LIBRARIES} ${LibJson_LIBRARIES} ${LibMongoClient_LIBRARIES})

add_subdirectory(contrib/libdazeus-irc)
add_subdirectory(src)
