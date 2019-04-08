# Prep options that are needed for each platform.

# This option quiets warnings that are a part of external projects.
option(WITH_EXTERNAL_WARNINGS "Build with warnings for all components, not just Etterna." OFF)

# This option is not yet working, but will likely default to ON in the future.
option(WITH_LTO "Build with Link Time Optimization (LTO)/Whole Program Optimization." OFF)

# This option handles if we use SSE2 processing.
option(WITH_SSE2 "Build with SSE2 Optimizations." ON)

# Turn this on to set this to a specific release mode.
option(WITH_FULL_RELEASE "Build as a proper, full release." OFF)

# Turn this option on to log every segment added or removed.
option(WITH_LOGGING_TIMING_DATA "Build with logging all Add and Erase Segment calls." OFF)

if(NOT MSVC)
  # Turn this option off to disable using FFMEPG.
  option(WITH_FFMPEG "Build with FFMPEG." ON)
  # Change this number to utilize a different number of jobs for building FFMPEG.
  option(WITH_FFMPEG_JOBS "Build FFMPEG with this many jobs." 2)
else()
  # Turn this option on to enable using the Texture Font Generator.
  option(WITH_TEXTURE_GENERATOR "Build with the Texture Font Generator. Ensure the MFC library is installed." OFF)
  # Turn this option off to use dynamic linking instead of static linking.
  option(WITH_STATIC_LINKING "Build Etterna with static linking." ON)
endif()

if(LINUX)
    # Builder beware: later versions of ffmpeg may break!
    option(WITH_SYSTEM_FFMPEG "Build with the system's FFMPEG, disabled build with bundled's FFMPEG" OFF)
    option(WITH_CRYSTALHD_DISABLED "Build FFMPEG without Crystal HD support." OFF)
    option(WITH_GLES2 "Build with OpenGL ES 2.0 Support." ON)
    option(WITH_GTK2 "Build with GTK2 Support." ON)
endif()
