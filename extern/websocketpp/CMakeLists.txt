add_library(websocketpp INTERFACE)
target_include_directories(websocketpp INTERFACE .)
target_link_libraries(websocketpp INTERFACE asio)