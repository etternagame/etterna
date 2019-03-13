# Stick to traditional approaches here. The following are defined.

# DLFCN_FOUND - The system has the dl library.
# DLFCN_INCLUDE_DIRS - The dl include directory.
# DLFCN_LIBRARIES - The library file to link to.

if (DLFCN_INCLUDE_DIRS AND DLFCN_LIBRARIES)
  # Already in cache, so don't repeat the finding procedures.
  set(DL_FIND_QUIETLY TRUE)
endif()

find_path(DLFCN_INCLUDE_DIRS dlfcn.h
  PATHS /usr/local/include /usr/include
)

find_library(DLFCN_LIBRARIES dl
  PATHS /usr/local/lib /usr/lib /lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DL DEFAULT_MSG DLFCN_LIBRARIES DLFCN_INCLUDE_DIRS)

mark_as_advanced(DLFCN_INCLUDE_DIRS DLFCN_LIBRARIES)

