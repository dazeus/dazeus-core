# Once done, this will define
#
#  LibJson_FOUND - system has LibJson
#  LibJson_INCLUDE_DIRS - the LibJson include directories
#  LibJson_LIBRARIES - link these to use LibJson

include(LibFindMacros)

# Include dir
find_path(LibJson_INCLUDE_DIR
  NAMES JSONOptions.h
  PATHS ${LibJson_PKGCONF_INCLUDE_DIRS}
      /usr/include /sw/include /usr/local/include
      /usr/include/libjson /sw/include/libjson /usr/local/include/libjson
)

# Finally the library itself
find_library(LibJson_LIBRARY
  NAMES json
  PATHS ${LibJson_PKGCONF_LIBRARY_DIRS}
      /usr/lib /lib /sw/lib /usr/local/lib
)

set(LibJson_PROCESS_INCLUDES LibJson_INCLUDE_DIR)
set(LibJson_PROCESS_LIBS LibJson_LIBRARY)
libfind_process(LibJson)
