# Disable all warnings for this directory
# Here are possible CMAKE_CXX_COMPILER_ID values: https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    add_compile_options(/W0)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL AppleClang)
    set_directory_properties(PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_INHIBIT_ALL_WARNINGS YES)
else() # All remaining compilers
    add_compile_options(-w)
endif()

# Extern Specific Variables, and the related add_subdirectory
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE) # Disable SQLiteCpp static analysis
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)  # Disable SQLiteCpp linting
add_subdirectory(SQLiteCpp)

set(CLANG_FORMAT_CMD OFF CACHE BOOL "" FORCE)       # Disable discord clang-format
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)         # Disable discord examples
set(USE_STATIC_CRT ON CACHE BOOL "" FORCE)          # Have discord-rpc link statically
add_subdirectory(discord-rpc)

# Universally required
add_subdirectory(rapidjson)
add_subdirectory(asio-1.12.2)
add_subdirectory(websocketpp)
add_subdirectory(LuaJIT-2.1.0-beta3)
add_subdirectory(libtomcrypt)
add_subdirectory(libtommath)
add_subdirectory(fftw-3.3.8)
add_subdirectory(glew-1.5.8)
add_subdirectory(pcre)
add_subdirectory(stb)
add_subdirectory(zlib)
add_subdirectory(curl)
add_subdirectory(mad-0.15.1b)
add_subdirectory(newogg)
add_subdirectory(newvorbis)

if(MSVC) # Only required on Windows
    add_subdirectory(ffmpeg)
endif()

# Organize Files in IDE
list(APPEND ET_EXT_TARGETS
    luajit lua minilua buildvm
    discord-rpc
    SQLiteCpp sqlite3
    tomcrypt libtommath
    fftw3f glfw pcre
    zlib libmad
    ogg vorbis)

set_property(TARGET ${ET_EXT_TARGETS} PROPERTY FOLDER "External Libraries") # Sort all extern libraries to a IDE folder/filter
