list(APPEND HD_PATTERN_MODS
    "Balance.h"
    "Chaos.h"
    "OHJ.h"
    "CJOHJ.h"
    "OHT.h"
    "VOHT.h"
    "Roll.h"
    "RunningMan.h"
    "WideRangeAnchor.h"
    "WideRangeBalance.h"
    "WideRangeJumptrill.h"
    "WideRangeRoll.h")

target_sources(Etterna PUBLIC ${HD_PATTERN_MODS})