cmake_minimum_required(VERSION 2.8)

file(GLOB dotconf_sources "../contrib/dotconf/src/*.c")

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")

configure_file("dazeusglobal.h.in" "dazeusglobal.h")

add_executable(dazeus ${sources} ${headers} ${dotconf_sources})
add_definitions("-Wall -Wextra -Wno-long-long")
# dotconf has unused variables
add_definitions("-Wno-unused-parameter")
# dotconf has wrong pointer signedness
add_definitions("-Wno-pointer-sign")
# dotconf has missing field initializers in LAST_OPTION
add_definitions("-Wno-missing-field-initializers")
include_directories("${CMAKE_SOURCE_DIR}/contrib/dotconf/src" "${CMAKE_BINARY_DIR}/src")
target_link_libraries(dazeus ${LIBS})

install (TARGETS dazeus DESTINATION bin)
