add_subdirectory(Agnostic)
add_subdirectory(Dependent)

list(APPEND GENERAL_CALC_SRC
    "MinaCalc.h"
    "MinaCalc.cpp"
    "MinaCalcHelpers.h"
    "CalcWindow.h"
    "PatternModHelpers.h"
    "SequencedBaseDiffCalc.h"
    "SequencingHelpers.h"
    "Ulbu.h"
    "UlbuAcolytes.h")

target_sources(Etterna PUBLIC ${GENERAL_CALC_SRC})