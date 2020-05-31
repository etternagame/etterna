
list(APPEND VORBIS_SRC
  "lib/analysis.c"
  "lib/bitrate.c"
  "lib/block.c"
  "lib/codebook.c"
  "lib/envelope.c"
  "lib/floor0.c"
  "lib/floor1.c"
  "lib/info.c"
  "lib/lookup.c"
  "lib/lpc.c"
  "lib/lsp.c"
  "lib/mapping0.c"
  "lib/mdct.c"
  "lib/psy.c"
  "lib/registry.c"
  "lib/res0.c"
  "lib/sharedbook.c"
  "lib/smallft.c"
  "lib/synthesis.c"
  "lib/vorbisenc.c"
  "lib/window.c"

  "lib/vorbisfile.c"
)

list(APPEND VORBIS_HPP
  "lib/backends.h"
  "lib/bitrate.h"
  "lib/codebook.h"
  "include/vorbis/codec.h"
  "lib/codec_internal.h"
  "lib/envelope.h"
  "lib/modes/floor_all.h"
  "lib/books/floor/floor_books.h"
  "lib/highlevel.h"
  "lib/lookup.h"
  "lib/lookup_data.h"
  "lib/lpc.h"
  "lib/lsp.h"
  "lib/masking.h"
  "lib/mdct.h"
  "lib/misc.h"
  "lib/os.h"
  "lib/psy.h"
  "lib/modes/psych_11.h"
  "lib/modes/psych_16.h"
  "lib/modes/psych_44.h"
  "lib/modes/psych_8.h"
  "lib/registry.h"
  "lib/books/coupled/res_books_stereo.h"
  "lib/books/uncoupled/res_books_uncoupled.h"
  "lib/modes/residue_16.h"
  "lib/modes/residue_44.h"
  "lib/modes/residue_44u.h"
  "lib/modes/residue_8.h"
  "lib/scales.h"
  "lib/modes/setup_11.h"
  "lib/modes/setup_16.h"
  "lib/modes/setup_22.h"
  "lib/modes/setup_32.h"
  "lib/modes/setup_44.h"
  "lib/modes/setup_44u.h"
  "lib/modes/setup_8.h"
  "lib/modes/setup_X.h"
  "lib/window.h"

  "include/vorbis/vorbisfile.h")

source_group("Source Files" FILES ${VORBIS_SRC})
source_group("Header Files" FILES ${VORBIS_HPP})

add_library("vorbis" STATIC ${VORBIS_SRC})

# disable_project_warnings("vorbis")

list(APPEND VORBIS_INCLUDE_DIRS "lib" "include")

target_include_directories("vorbis" PUBLIC ${VORBIS_INCLUDE_DIRS})

target_link_libraries(vorbis ogg)


################# VORBISFILE #########################

# list(APPEND VORBISFILE_SRC
#   "lib/vorbisfile.c"
# )

# list(APPEND VORBISFILE_HPP "include/vorbis/vorbisfile.h"s)

# source_group("Source Files" FILES ${VORBISFILE_SRC})
# source_group("Header Files" FILES ${VORBISFILE_HPP})

# add_library("vorbisfile" ${VORBISFILE_SRC} )
# set_property(TARGET "vorbisfile" PROPERTY FOLDER "External Libraries")

# # disable_project_warnings("vorbisfile")

# target_include_directories("vorbisfile" PUBLIC include)

# target_link_libraries(vorbisfile ogg)
