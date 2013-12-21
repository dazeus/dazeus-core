cmake_minimum_required(VERSION 2.8)

project(dazeus)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/" "${CMAKE_SOURCE_DIR}/contrib/libdazeus-irc/cmake/")
set(DAZEUS_VERSION_MAJOR "2")
set(DAZEUS_VERSION_MINOR "0")
set(DAZEUS_VERSION "${DAZEUS_VERSION_MAJOR}.${DAZEUS_VERSION_MINOR}")

IF(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/libdazeus-irc/CMakeLists.txt")
	message("It seems you do not have a checkout of libdazeus-irc.")
	message("It is required to compile dazeus, and can be automatically")
	message("created by Git by running:")
	message("  git submodule update --init")
	message(FATAL_ERROR "libdazeus-irc is not checked out")
ENDIF()
IF(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/jansson/README.rst")
	message("It seems you do not have a checkout of jansson.")
	message("It is required to compile dazeus, and can be automatically")
	message("created by Git by running:")
	message("  git submodule update --init")
	message(FATAL_ERROR "jansson is not checked out")
ENDIF()

find_package(LibMongoClient REQUIRED)
find_package(Boost REQUIRED)

include_directories(SYSTEM ${LibMongoClient_INCLUDE_DIRS})
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/contrib/jansson/src")
include_directories("${CMAKE_SOURCE_DIR}/contrib/libdazeus-irc/src")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/jansson_config.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/contrib/jansson/src/jansson_config.h")
file(GLOB jansson_sources "${CMAKE_CURRENT_SOURCE_DIR}/contrib/jansson/src/*.c")
file(GLOB jansson_headers "${CMAKE_CURRENT_SOURCE_DIR}/contrib/jansson/src/*.h")

add_library(jansson ${jansson_sources} ${jansson_headers})
target_link_libraries(jansson)

set(LIBS ${LIBS} jansson ssl crypto dazeus-irc ${LibIRCClient_LIBRARIES} ${LibJson_LIBRARIES} ${LibMongoClient_LIBRARIES} ${Boost_LIBRARIES})

find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
		${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/misc/mainpage.dox.in
		${CMAKE_CURRENT_SOURCE_DIR}/misc/mainpage.dox)
	add_custom_target(doc ${DOXYGEN_EXECUTABLE}
		${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating documentation with Doxygen" VERBATIM
	)
endif()

add_subdirectory(contrib/libdazeus-irc)
add_subdirectory(src)
