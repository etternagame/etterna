project(jsoncpp)

add_library(jsoncpp)
file(GLOB_RECURSE ALL_SOURCE_FILES src/lib_json/*.cpp)

target_sources(jsoncpp PRIVATE ${ALL_SOURCE_FILES})
target_include_directories(jsoncpp PUBLIC ${PROJECT_SOURCE_DIR}/include)
