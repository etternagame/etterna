add_library(curl INTERFACE)
target_include_directories(curl INTERFACE include)

if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64|amd64)")
    target_link_directories(curl INTERFACE .)
else()
    target_link_directories(curl INTERFACE 64bit)
endif()