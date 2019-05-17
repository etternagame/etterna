project(MinaCalc)

add_library(MinaCalc INTERFACE)
target_include_directories(MinaCalc INTERFACE ${PROJECT_SOURCE_DIR}/include)

if(WIN32 AND ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64" OR "${CMAKE_GENERATOR}" STREQUAL "Ninja"))
    target_link_libraries(MinaCalc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/MinaCalc.lib)
else()
    target_link_libraries(MinaCalc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/MinaCalc_x86.lib)
endif()

if(APPLE)
  target_link_libraries(MinaCalc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/libMinaCalc.a)
elseif(UNIX)
  target_link_libraries(MinaCalc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/MinaCalc.a)
endif()
