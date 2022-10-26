list(APPEND HD_PATTERN_MODS
    "Balance.h"
    "Chaos.h"
    "CJOHAnchor.h"
    "OHJ.h"
    "CJOHJ.h"
    "OHT.h"
    "VOHT.h"
    "Minijack.h"
    "Roll.h"
    "RollJS.h"
    "RunningMan.h"
    "WideRangeAnchor.h"
    "WideRangeBalance.h"
    "WideRangeJumptrill.h"
    "WideRangeJJ.h"
    "WideRangeRoll.h")

target_sources(Etterna PUBLIC ${HD_PATTERN_MODS})