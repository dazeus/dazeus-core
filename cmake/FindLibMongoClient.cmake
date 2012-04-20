# Once done, this will define
#
#  LibMongoClient_FOUND - system has LibMongoClient
#  LibMongoClient_INCLUDE_DIRS - the LibMongoClient include directories
#  LibMongoClient_LIBRARIES - link these to use LibMongoClient

include(LibFindMacros)

libfind_package(LibMongoClient GLIB2)

# Include dir
find_path(LibMongoClient_INCLUDE_DIR
  NAMES mongo-client.h
  PATHS ${LibMongoClient_PKGCONF_INCLUDE_DIRS}
      /usr/include /sw/include /usr/local/include
      /usr/include/mongo-client /sw/include/mongo-client /usr/local/include/mongo-client
)

# Finally the library itself
find_library(LibMongoClient_LIBRARY
  NAMES mongo-client
  PATHS ${LibMongoClient_PKGCONF_LIBRARY_DIRS}
      /usr/lib /lib /sw/lib /usr/local/lib
)

set(LibMongoClient_PROCESS_INCLUDES LibMongoClient_INCLUDE_DIR GLIB2_INCLUDE_DIRS)
set(LibMongoClient_PROCESS_LIBS LibMongoClient_LIBRARY GLIB2_LDFLAGS)
libfind_process(LibMongoClient)
