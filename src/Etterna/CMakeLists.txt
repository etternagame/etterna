# This is where everything related to Etterna's build is defined

# Project-Wide Settings


## Configuration file
include(CMakeListsLegacy.cmake)

# Process CMakeLists.txt in each
add_subdirectory(Actor)
add_subdirectory(FileTypes)
add_subdirectory(Globals)
add_subdirectory(Models)
add_subdirectory(Screen)
add_subdirectory(Singletons)

configure_file(config.in.hpp ${CMAKE_BINARY_DIR}/generated/config.hpp)
configure_file(verstub.in.cpp ${CMAKE_BINARY_DIR}/generated/verstub.cpp)