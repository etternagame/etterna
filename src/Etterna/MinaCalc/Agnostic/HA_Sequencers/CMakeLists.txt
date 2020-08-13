list(APPEND HA_SEQUENCERS
    "FlamSequencing.h"
    "ThingSequencing.h")

target_sources(Etterna PUBLIC ${HA_SEQUENCERS})