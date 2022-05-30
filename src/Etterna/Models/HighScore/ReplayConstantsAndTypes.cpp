#include "ReplayConstantsAndTypes.h"
#include "Etterna/Globals/global.h"

static const char* ReplayTypeNames[] = {
	"V0",
	"V1",
	"V2",
	"Input",
};
XToString(ReplayType);
LuaXType(ReplayType);
