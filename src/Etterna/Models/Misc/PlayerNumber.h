/* PlayerNumber - A simple type representing a player. */

#ifndef PlayerNumber_H
#define PlayerNumber_H

#include "EnumHelper.h"

// Player number stuff
enum PlayerNumber
{
	PLAYER_1 = 0,
	NUM_PlayerNumber, // leave this at the end
	PlayerNumber_Invalid
};
const PlayerNumber NUM_PLAYERS = NUM_PlayerNumber;
const PlayerNumber PLAYER_INVALID = PlayerNumber_Invalid;
auto
PlayerNumberToString(PlayerNumber pn) -> const std::string&;
auto
PlayerNumberToLocalizedString(PlayerNumber pn) -> const std::string&;
LuaDeclareType(PlayerNumber);
/** @brief A foreach loop to handle the different players. */
#define FOREACH_PlayerNumber(pn) FOREACH_ENUM(PlayerNumber, pn)

const PlayerNumber OPPOSITE_PLAYER[NUM_PLAYERS] = { PLAYER_1 };

enum MultiPlayer
{
	MultiPlayer_P1 = 0,
	MultiPlayer_P2,
	MultiPlayer_P3,
	MultiPlayer_P4,
	MultiPlayer_P5,
	MultiPlayer_P6,
	MultiPlayer_P7,
	MultiPlayer_P8,
	MultiPlayer_P9,
	MultiPlayer_P10,
	MultiPlayer_P11,
	MultiPlayer_P12,
	MultiPlayer_P13,
	MultiPlayer_P14,
	MultiPlayer_P15,
	MultiPlayer_P16,
	MultiPlayer_P17,
	MultiPlayer_P18,
	MultiPlayer_P19,
	MultiPlayer_P20,
	MultiPlayer_P21,
	MultiPlayer_P22,
	MultiPlayer_P23,
	MultiPlayer_P24,
	MultiPlayer_P25,
	MultiPlayer_P26,
	MultiPlayer_P27,
	MultiPlayer_P28,
	MultiPlayer_P29,
	MultiPlayer_P30,
	MultiPlayer_P31,
	MultiPlayer_P32,
	NUM_MultiPlayer, // leave this at the end
	MultiPlayer_Invalid
};
auto
MultiPlayerToString(MultiPlayer mp) -> const std::string&;
auto
MultiPlayerToLocalizedString(MultiPlayer mp) -> const std::string&;
LuaDeclareType(MultiPlayer);
/** @brief A foreach loop to handle the different Players in MultiPlayer. */
#define FOREACH_MultiPlayer(pn) FOREACH_ENUM(MultiPlayer, pn)

#endif
