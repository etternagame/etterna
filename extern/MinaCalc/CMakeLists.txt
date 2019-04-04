project(MinaCalc)

# null.cpp since all libs require source, but since this one doesn't have it,
#   an empty cpp file will work.
add_library(MinaCalc INTERFACE)

# file(GLOB_RECURSE ALL_LIB_FILES lib/*.a lib/*.lib)
# target_link_libraries(MinaCalc INTERFACE ${ALL_LIB_FILES})
target_include_directories(MinaCalc INTERFACE ${PROJECT_SOURCE_DIR}/include)

if(MSVC AND "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64|amd64)")
    target_link_libraries(MinaCalc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/MinaCalc.lib)
else()
    target_link_libraries(MinaCalc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/MinaCalc_x86.lib)
endif()

if(APPLE)
  target_link_libraries(MinaCalc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/libMinaCalc.a)
elseif(UNIX)
  target_link_libraries(MinaCalc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/MinaCalc.a)
endif()
