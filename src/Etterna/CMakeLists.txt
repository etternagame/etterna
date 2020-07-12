# This is where everything related to Etterna's build is defined

# Project-Wide Settings
## Configuration file
include(CMakeListsLegacy.cmake) # TODO: Reduce this to removal, or as much as possible.

# Process CMakeLists.txt in each
add_subdirectory(Actor)
add_subdirectory(FileTypes)
add_subdirectory(Globals)
add_subdirectory(MinaCalc)
add_subdirectory(Models)
add_subdirectory(Screen)
add_subdirectory(Singletons)

# Generated Source Code
include(${PROJECT_SOURCE_DIR}/CMake/Helpers/SetupGit.cmake)
configure_file(config.in.hpp ${CMAKE_BINARY_DIR}/generated/config.hpp)
configure_file(verstub.in.cpp ${CMAKE_BINARY_DIR}/generated/verstub.cpp)

# Add generated source to target
target_sources(Etterna PUBLIC ${PROJECT_BINARY_DIR}/generated/verstub.cpp)
