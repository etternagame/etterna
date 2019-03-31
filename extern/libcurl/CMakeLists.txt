add_library(curl INTERFACE)
target_include_directories(curl INTERFACE include)

if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64|amd64)")
    target_link_libraries(curl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/64bit/libcurl.lib")
else()
    target_link_libraries(curl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libcurl_x86.lib")
endif()