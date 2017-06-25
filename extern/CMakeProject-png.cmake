file(GLOB pnglib_sources libpng/*.c)
set(PNG_SRC ${pnglib_sources})

file(GLOB pnglib_headers libpng/*.h)
set(PNG_HPP ${pnglib_headers})

source_group("" FILES ${PNG_SRC})
source_group("" FILES ${PNG_HPP})

add_library("png" STATIC ${PNG_SRC} ${PNG_HPP})

set_property(TARGET "png" PROPERTY FOLDER "External Libraries")
set_target_properties("png" PROPERTIES LINKER_LANGUAGE C)

# include_directories(src)

if(MSVC)
  sm_add_compile_definition("png" _CRT_SECURE_NO_WARNINGS)
endif(MSVC)

disable_project_warnings("png")

target_include_directories("png" PUBLIC "zlib")

