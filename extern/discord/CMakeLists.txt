

add_library(discord INTERFACE)

# Include for Social SDK headers
target_include_directories(discord INTERFACE include)

# Platform-specific Social SDK library paths
if(WIN32)
    set(DISCORD_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/release/discord_partner_sdk.lib")
    #set(DISCORD_SHARED_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/release/discord_partner_sdk.dll")
elseif(APPLE)
    set(DISCORD_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libdiscord_partner_sdk.dylib")
    #set(DISCORD_SHARED_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libdiscord_partner_sdk.dylib")
else() # Linux
    set(DISCORD_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libdiscord_partner_sdk.so")
    #set(DISCORD_SHARED_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libdiscord_partner_sdk.so")
endif()

message(DISCORD_LIB_PATH="${DISCORD_LIB_PATH}")

# Link the Social SDK library
target_link_libraries(discord INTERFACE ${DISCORD_LIB_PATH})

# Set the runtime search path (RPATH) for Linux and macOS
if(UNIX)
    # Use RPATH when building
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    # Set the RPATH to use the lib directory relative to the executable
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    if(APPLE)
        set(CMAKE_INSTALL_RPATH "@executable_path")
    endif()
endif()