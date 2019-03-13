# Find pcre using standard tools.

# The following will be set:

# PCRE_INCLURE_DIRS
# PCRE_LIBRARIES
# PCRE_FOUND

find_path(PCRE_INCLURE_DIRS NAMES pcre.h)
find_library(PCRE_LIBRARIES NAMES pcre)

# Properly pass QUIETLY and REQUIRED.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE DEFAULT_MSG PCRE_INCLURE_DIRS PCRE_LIBRARIES)

mark_as_advanced(PCRE_INCLURE_DIRS PCRE_LIBRARIES)

