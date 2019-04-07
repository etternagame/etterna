# Disable all warnings for this directory
# Here are possible CMAKE_CXX_COMPILER_ID values: https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html

if(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
    add_compile_options(/W0)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL AppleClang)
    set_directory_properties(PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_INHIBIT_ALL_WARNINGS YES)
else() # All remaining compilers
    add_compile_options(-w)
endif()

# Disable install from being used in subdirectories
# The install command is used to decide what gets included in the distribution 
# executable/tar.gz/DMG, do defining this macro stops externs from getting included.
# The macro is only valid in this subdirectory and below
macro(install) 
endmacro()

# Universally required
add_subdirectory(LuaJIT-2.1.0-beta3)
add_subdirectory(discord-rpc)
add_subdirectory(SQLiteCpp)
add_subdirectory(json)
add_subdirectory(jsoncpp)
add_subdirectory(libtomcrypt)
add_subdirectory(libtommath)
add_subdirectory(fftw-3.3.8)
add_subdirectory(MinaCalc)
add_subdirectory(glew-1.5.8)
add_subdirectory(pcre)
add_subdirectory(stb)
add_subdirectory(libuv)
add_subdirectory(zlib)
add_subdirectory(uWebSocket)
add_subdirectory(libcurl)
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
    jsoncpp uv uv_a
    tomcrypt libtommath
    fftw3f glfw pcre
    zlib uWS libmad
    ogg vorbis)

set_property(TARGET ${ET_EXT_TARGETS} PROPERTY FOLDER "External Libraries") # Sort all extern libraries to a IDE folder/filter
