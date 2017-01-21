cmake_minimum_required(VERSION 2.8)

file(GLOB sources "*.cpp")
file(GLOB headers "*.h" "db/database.h")

# Conditionally add database sources
if(${DB_POSTGRES})
  file(GLOB postgres_sources "db/postgres.cpp")
  file(GLOB postgres_headers "db/postgres.h")
  set(sources ${sources} ${postgres_sources})
  set(headers ${headers} ${postgres_headers})
endif()

if(${DB_SQLITE})
  file(GLOB sqlite_sources "db/sqlite.cpp")
  file(GLOB sqlite_headers "db/sqlite.h")
  set(sources ${sources} ${sqlite_sources})
  set(headers ${headers} ${sqlite_headers})
endif()

if(${DB_MONGO})
  file(GLOB mongo_sources "db/mongo.cpp")
  file(GLOB mongo_headers "db/mongo.h")
  set(sources ${sources} ${mongo_sources})
  set(headers ${headers} ${mongo_headers})
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

configure_file("dazeusglobal.h.in" "dazeusglobal.h")

add_library(dotconf "${CMAKE_SOURCE_DIR}/contrib/dotconf/src/dotconf.c")
set_target_properties(dotconf PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-pointer-sign -Wno-missing-field-initializers")

add_executable(dazeus ${sources} ${headers})
target_include_directories(dazeus SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/contrib/dotconf/src")
target_include_directories(dazeus PRIVATE "${CMAKE_BINARY_DIR}/src")

target_link_libraries(dazeus dotconf ${LIBS})

install (TARGETS dazeus DESTINATION bin)
