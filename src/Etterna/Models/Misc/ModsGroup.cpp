#include "Etterna/Globals/global.h"
#include "EnumHelper.h"
#include "Etterna/Singletons/LuaManager.h"
#include "ModsGroup.h"

static const char* ModsLevelNames[] = {
	"Preferred",
	"Stage",
	"Song",
	"Current",
};
XToString(ModsLevel);
LuaXType(ModsLevel);
