#include "Etterna/Globals/global.h"
#include "LocalizedString.h"
#include "Etterna/Singletons/LuaManager.h"
#include "PlayerNumber.h"

static const char* PlayerNumberNames[] = { "P1" };
XToString(PlayerNumber);
XToLocalizedString(PlayerNumber);
LuaFunction(PlayerNumberToString, PlayerNumberToString(PLAYER_1));
LuaXType(PlayerNumber);
LuaFunction(PlayerNumberToLocalizedString,
			PlayerNumberToLocalizedString(PLAYER_1));

static const char* MultiPlayerNames[] = {
	"P1",  "P2",  "P3",  "P4",  "P5",  "P6",  "P7",  "P8",  "P9",  "P10", "P11",
	"P12", "P13", "P14", "P15", "P16", "P17", "P18", "P19", "P20", "P21", "P22",
	"P23", "P24", "P25", "P26", "P27", "P28", "P29", "P30", "P31", "P32",
};
XToString(MultiPlayer);
XToLocalizedString(MultiPlayer);
LuaFunction(MultiPlayerToString,
			MultiPlayerToString(Enum::Check<MultiPlayer>(L, 1)));
LuaFunction(MultiPlayerToLocalizedString,
			MultiPlayerToLocalizedString(Enum::Check<MultiPlayer>(L, 1)));
LuaXType(MultiPlayer);
