# Disable all warnings for this directory
# Possible CMAKE_CXX_COMPILER_ID values: https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html
if(MSVC)
    add_compile_options(/W0)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL AppleClang)
    set_directory_properties(PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_INHIBIT_ALL_WARNINGS YES)
else() # All remaining compilers
    add_compile_options(-w)
endif()

# Library Options
set(BUILD_SHARED_LIBS OFF) # Tell CMake to prefer static libs

# External Libraries
## Curl Specific Options
set(CMAKE_USE_LIBSSH2 OFF CACHE BOOL "" FORCE) # Disable curl libssh2
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)    # Tell curl not to build standalone binary
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)     # Disable curl testing
set(CMAKE_USE_OPENSSL ON CACHE BOOL "" FORCE)  # Require OpenSSL
set(HTTP_ONLY ON CACHE BOOL "" FORCE)          # Disable all protocols except HTTP/S
add_subdirectory(curl)

## SQLiteCpp
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE) # Disable SQLiteCpp static analysis
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)  # Disable SQLiteCpp lintiPng
add_subdirectory(SQLiteCpp)

## websocketpp
add_subdirectory(asio-1.12.2) # Only required for websocketpp
add_subdirectory(websocketpp)

## discord-rpc
set(CLANG_FORMAT_CMD OFF CACHE BOOL "" FORCE)   # Disable discord clang-format
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)     # Disable discord examples
add_subdirectory(discord-rpc)

## Header Only
add_subdirectory(picosha2)
add_subdirectory(rapidjson)
add_subdirectory(stb)

## Remaining Libraries
add_subdirectory(libtomcrypt)
add_subdirectory(libtommath)
add_subdirectory(LuaJIT-2.1.0-beta3)
add_subdirectory(muFFT)
add_subdirectory(glew-1.5.8)
add_subdirectory(pcre)
add_subdirectory(zlib)
add_subdirectory(mad-0.15.1b)

## Vorbis depends on Vorbis
add_subdirectory(newogg)
add_subdirectory(newvorbis)
target_link_libraries(vorbis ogg)

if(MSVC) # Only required on Windows
    add_subdirectory(ffmpeg)
endif()

## IDE Folder Organization
list(APPEND EXT_TARGETS
    SQLiteCpp sqlite3
    discord-rpc muFFT
    tomcrypt libtommath
    zlib libmad pcre
    ogg vorbis glfw
    libcurl)

list(APPEND LUA_TARGETS luajit lua minilua buildvm)

## Sort all extern libraries to a IDE folder/filter
set_property(TARGET ${EXT_TARGETS} PROPERTY FOLDER "External Libraries")
set_property(TARGET ${LUA_TARGETS} PROPERTY FOLDER "External Libraries/Lua 5.1")
