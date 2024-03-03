list(APPEND HA_PATTERN_MODS
    "CJ.h"
    "CJDensity.h"
    "FlamJam.h"
    "HS.h"
    "HSDensity.h"
    "JS.h"
    "Stream.h"
    "TheThingFinder.h"
    "GenericStream.h"
    "GenericChordstream.h"
    "GenericBracketing.h")

target_sources(Etterna PUBLIC ${HA_PATTERN_MODS})