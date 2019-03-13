# Find the Video Acceleration library.

# The following will be set:

# VA_INCLUDE_DIRS
# VA_LIBRARIES
# VA_FOUND

find_path(VA_INCLUDE_DIRS NAMES va/va.h)

set(VA_NAMES "${VA_NAMES}" "va" "libva")
find_library(VA_LIBRARIES NAMES ${VA_NAMES})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VA DEFAULT_MSG VA_LIBRARIES VA_INCLUDE_DIRS)

mark_as_advanced(VA_LIBRARIES VA_INCLUDE_DIRS)
