list(APPEND HA_SEQUENCERS
    "FlamSequencing.h"
    "ThingSequencing.h"
    "TrillSequencing.h")

target_sources(Etterna PUBLIC ${HA_SEQUENCERS})