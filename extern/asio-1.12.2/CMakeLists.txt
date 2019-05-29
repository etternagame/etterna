add_library(asio INTERFACE)
target_include_directories(asio INTERFACE include)

find_package(OpenSSL REQUIRED)
target_link_libraries(asio INTERFACE OpenSSL::SSL)
