cmake_minimum_required(VERSION 2.8)

file(GLOB dotconf_sources "../contrib/dotconf/src/*.c")

file(GLOB sources "*.cpp")
file(GLOB headers "*.h" "dba/*.h")

# Conditionally add database sources
if(${DBA_POSTGRES})
  file(GLOB postgres_sources "dba/postgres.cpp")
  set(sources ${sources} ${postgres_sources})
endif()

if(${DBA_MONGO})
  file(GLOB mongo_sources "dba/mongo.cpp")
  set(sources ${sources} ${mongo_sources})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")

add_executable(dazeus ${sources} ${headers} ${dotconf_sources})
add_definitions("-Wall -Wextra -Wno-long-long")
# dotconf has unused variables
add_definitions("-Wno-unused-parameter")
# dotconf has wrong pointer signedness
add_definitions("-Wno-pointer-sign")
# dotconf has missing field initializers in LAST_OPTION
add_definitions("-Wno-missing-field-initializers")
include_directories("../contrib/dotconf/src")
target_link_libraries(dazeus ${LIBS})

install (TARGETS dazeus DESTINATION bin)
