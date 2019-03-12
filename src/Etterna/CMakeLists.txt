# This is where everything related to Etterna's build is defined

# Project-Wide Settings
## Policies
cmake_policy(SET CMP0076 NEW)  # Tell cmake to convert target_sources input from relative path to absolute path

## Configuration file
include(CMakeListsLegacy.cmake)

# Process CMakeLists.txt in each
add_subdirectory(Actor)
add_subdirectory(FileTypes)
add_subdirectory(Globals)
add_subdirectory(Models)
add_subdirectory(Screen)
add_subdirectory(Singletons)

