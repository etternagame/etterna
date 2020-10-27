list(APPEND HD_SEQUENCERS
    "GenericSequencing.h"
    "OHJSequencing.h"
    "RMSequencing.h")

target_sources(Etterna PUBLIC ${HD_SEQUENCERS})