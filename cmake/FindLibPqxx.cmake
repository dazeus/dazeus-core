# Run this for the first run, it finds the library, but it doesn't compile the module

# Once done, this will define
#
#  LIBPQXX_FOUND - system has LibPqxx
#  LibPqxx_INCLUDE_DIRS - the LibPqxx include directories
#  LibPqxx_LIBRARIES - link these to use LibPqxx

# Include dir
find_path(LibPqxx_INCLUDE_DIR
  NAMES pqxx/pqxx
  PATHS ${LibPqxx_PKGCONF_INCLUDE_DIRS}
      /usr/include /usr/local/include /sw/include
)

# The library itself
find_library(LibPqxx_LIBRARY
  NAMES pqxx
  PATHS ${LibPqxx_PKGCONF_LIBRARY_DIRS}
      /usr/lib /lib /sw/lib /usr/local/lib
)

set(LibPqxx_INCLUDE_DIRS ${LibPqxx_INCLUDE_DIR})
set(LibPqxx_LIBRARIES ${LibPqxx_LIBRARY})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibPqxx
    FOUND_VAR LibPqxx_FOUND
    REQUIRED_VARS LibPqxx_LIBRARY LibPqxx_INCLUDE_DIR
)
mark_as_advanced(LibPqxx_INCLUDE_DIR LibPqxx_LIBRARY)
