cmake_minimum_required(VERSION 2.8)

file(GLOB sources "*.cpp")
file(GLOB headers "*.h")

add_executable(dazeus ${sources} ${headers})
add_definitions("-Wall -Wextra -Weffc++ -Wno-long-long -pedantic -DNDEBUG")
target_link_libraries(dazeus ${LIBS})

install (TARGETS dazeus DESTINATION bin)
