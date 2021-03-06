cmake_minimum_required(VERSION 2.8)

project(dazeus)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/" "${CMAKE_SOURCE_DIR}/contrib/libdazeus-irc/cmake/")
set(DAZEUS_VERSION_MAJOR "2")
set(DAZEUS_VERSION_MINOR "0")
set(DAZEUS_VERSION_SUFFIX "-beta1")
set(DAZEUS_VERSION "${DAZEUS_VERSION_MAJOR}.${DAZEUS_VERSION_MINOR}${DAZEUS_VERSION_SUFFIX}")

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
IF(NOT EXISTS "${CMAKE_SOURCE_DIR}/contrib/mstd/CMakeLists.txt")
	message("It seems you do not have a checkout of mstd.")
	message("It is required to compile dazeus, and can be automatically")
	message("created by Git by running:")
	message("  git submodule update --init")
	message(FATAL_ERROR "mstd is not checked out")
ENDIF()

option(USE_ASAN "Enable Address Sanitizer" OFF)
if(USE_ASAN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif(USE_ASAN)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/contrib/jansson/src")
include_directories("${CMAKE_SOURCE_DIR}/contrib/libdazeus-irc/src")
include_directories("${CMAKE_SOURCE_DIR}/contrib/mstd/include")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/jansson_config.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/contrib/jansson/src/jansson_config.h")
file(GLOB jansson_sources "${CMAKE_CURRENT_SOURCE_DIR}/contrib/jansson/src/*.c")
file(GLOB jansson_headers "${CMAKE_CURRENT_SOURCE_DIR}/contrib/jansson/src/*.h")

add_library(jansson ${jansson_sources} ${jansson_headers})
target_link_libraries(jansson)

set(LIBS ${LIBS} jansson dazeus-irc ${LibJson_LIBRARIES})

##
## Database layers
##

# PostgreSQL
find_package(LibPqxx)
if(${LibPqxx_FOUND})
	include_directories(SYSTEM ${LibPqxx_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${LibPqxx_LIBRARIES})
	set(DB_POSTGRES true)
	add_definitions(-DDB_POSTGRES)
endif()

# SQLite
find_package(Sqlite3)
if(${SQLITE3_FOUND})
	include_directories(SYSTEM ${SQLITE3_INCLUDE_DIR})
	set(LIBS ${LIBS} ${SQLITE3_LIBRARY})
	set(DB_SQLITE true)
	add_definitions(-DDB_SQLITE)
endif()

# MongoDB
find_package(LibMongoClient)
if(${LibMongoClient_FOUND})
	include_directories(SYSTEM ${LibMongoClient_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${LibMongoClient_LIBRARIES})
	set(DB_MONGO true)
	add_definitions(-DDB_MONGO)
endif()

# Documentation
find_package(Doxygen)
if(${DOXYGEN_FOUND})
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
