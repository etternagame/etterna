add_subdirectory(HA_PatternMods)
add_subdirectory(HA_Sequencers)

list(APPEND HA_GENERAL_SRC
    "HA_Sequencing.h"
    "IntervalInfo.h"
    "MetaIntervalInfo.h"
    "MetaRowInfo.h")

target_sources(Etterna PUBLIC ${HA_GENERAL_SRC})