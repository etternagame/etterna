file(GLOB SOURCES src/*.cpp)
add_library(uWS ${SOURCES})
target_include_directories(uWS INTERFACE include)

find_package(OpenSSL REQUIRED)
target_link_libraries(uWS OpenSSL::SSL)
target_link_libraries(uWS zlib)
target_link_libraries(uWS uv_a)