if(NOT MSVC)
    find_package(OpenSSL REQUIRED)
    find_package(ZLIB REQUIRED)
    
    file(GLOB SOURCES src/*.cpp)
    add_library(uWS ${SOURCES})
    target_include_directories(uWS PUBLIC ${OPENSSL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/extern/libuv/include" "${PROJECT_SOURCE_DIR}/extern/uWebSocket/include")
    target_link_libraries("uWS" PUBLIC ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} "${PROJECT_SOURCE_DIR}/extern/libuv/libuv.a")
    
    set_property(TARGET uWS PROPERTY FOLDER "External Libraries")
endif()