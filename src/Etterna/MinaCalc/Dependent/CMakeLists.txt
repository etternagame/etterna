add_subdirectory(HD_PatternMods)
add_subdirectory(HD_Sequencers)

list(APPEND HD_GENERAL_SRC
    "HD_BasicSequencing.h"
    "HD_MetaSequencing.h"
    "IntervalHandInfo.h"
    "MetaHandInfo.h"
    "MetaIntervalHandInfo.h")

target_sources(Etterna PUBLIC ${HD_GENERAL_SRC})