

file(GLOB_RECURSE TRACY_SRC
    common/*.cpp client/*.cpp TracyClient.cpp
)

file(GLOB_RECURSE TRACY_HPP
    common/*.hpp client/*.hpp Tracy.hpp 
)
#list(APPEND TRACY_SRC
#  "common/tracy_lz4.cpp"
#  "client/tracy_rpmalloc.cpp")

#list(APPEND TRACY_HPP
#  "common/tracy_lz4.hpp"
#  "client/tracy_rpmalloc.hpp")

source_group("Source Files" FILES ${TRACY_SRC})
source_group("Header Files" FILES ${TRACY_HPP})

add_library("tracy" ${TRACY_SRC})

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")

            foreach(CompilerFlag
                    CMAKE_CXX_FLAGS
                    CMAKE_CXX_FLAGS_DEBUG
                    CMAKE_CXX_FLAGS_RELEASE
                    CMAKE_C_FLAGS
                    CMAKE_C_FLAGS_DEBUG
                    CMAKE_C_FLAGS_RELEASE)
                string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
            endforeach()

if(ENABLE_TRACY_PROFILER)
  target_compile_definitions("tracy" PUBLIC TRACY_ENABLE)
endif()

set_property(TARGET "tracy" PROPERTY FOLDER "External Libraries")

target_include_directories("tracy" PUBLIC .)
