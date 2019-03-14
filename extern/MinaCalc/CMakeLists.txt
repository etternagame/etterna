project(MinaCalc)

add_library(MinaCalc null.cpp)

file(GLOB_RECURSE ALL_LIB_FILES lib/*.a lib/*.lib)
target_link_libraries(MinaCalc PUBLIC ${ALL_LIB_FILES})
target_include_directories(MinaCalc PUBLIC ${PROJECT_SOURCE_DIR}/include)