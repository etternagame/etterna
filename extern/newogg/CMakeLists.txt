
list(APPEND OGG_SRC
  "src/bitwise.c"
  "src/framing.c")

list(APPEND OGG_HPP
  "include/ogg/config_types.h"
  "include/ogg/ogg.h"
  "include/ogg/os_types.h")

source_group("Source Files" FILES ${OGG_SRC})
source_group("Header Files" FILES ${OGG_HPP})

add_library("ogg" STATIC ${OGG_SRC} ${OGG_HPP} ${OGG_DAT})

# set_property(TARGET "ogg" PROPERTY FOLDER "External Libraries")

# disable_project_warnings(ogg)
target_include_directories(ogg PUBLIC include)

configure_file("config.ogg.types.in.h" "include/ogg/config_types.h")
