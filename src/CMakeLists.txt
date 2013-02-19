cmake_minimum_required(VERSION 2.8)

file(GLOB dotconf_sources "../contrib/dotconf/src/*.c")

file(GLOB sources "*.cpp")
file(GLOB headers "*.h")

add_executable(dazeus ${sources} ${headers} ${dotconf_sources})
add_definitions("-Wall -Wextra -Weffc++ -Wno-long-long")
# dotconf has unused variables
add_definitions("-Wno-unused-parameter")
# dotconf has wrong pointer signedness
add_definitions("-Wno-pointer-sign")
# dotconf has missing field initializers in LAST_OPTION
add_definitions("-Wno-missing-field-initializers")
include_directories("../contrib/dotconf/src")
target_link_libraries(dazeus ${LIBS})

install (TARGETS dazeus DESTINATION bin)
