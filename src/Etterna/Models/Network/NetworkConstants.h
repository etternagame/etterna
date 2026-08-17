#pragma once

#include <string>
#include <map>

// [SMLClientCommands name]
enum NSCommand
{
	NSCPing = 0,
	NSCPingR,	  //  1 [SMLC_PingR]
	NSCHello,	  //  2 [SMLC_Hello]
	NSCGSR,		  //  3 [SMLC_GameStart]
	NSCGON,		  //  4 [SMLC_GameOver]
	NSCGSU,		  //  5 [SMLC_GameStatusUpdate]
	NSCSU,		  //  6 [SMLC_StyleUpdate]
	NSCCM,		  //  7 [SMLC_Chat]
	NSCRSG,		  //  8 [SMLC_RequestStart]
	NSCUUL,		  //  9 [SMLC_Reserved1]
	NSCSMS,		  // 10 [SMLC_MusicSelect]
	NSCUPOpts,	  // 11 [SMLC_PlayerOpts]
	NSCSMOnline,  // 12 [SMLC_SMO]
	NSCFormatted, // 13 [SMLC_RESERVED1]
	NSCAttack,	  // 14 [SMLC_RESERVED2]
	XML,		  // 15 [SMLC_RESERVED3]
	FLU,		  // 16 [SMLC_FriendListUpdate]
	NUM_NS_COMMANDS
};

enum SMOStepType
{
	SMOST_UNUSED = 0,
	SMOST_HITMINE,
	SMOST_AVOIDMINE,
	SMOST_MISS,
	SMOST_W5,
	SMOST_W4,
	SMOST_W3,
	SMOST_W2,
	SMOST_W1,
	SMOST_LETGO,
	SMOST_HELD
	/*
	,SMOST_CHECKPOINTMISS,
	SMOST_CHECKPOINTHIT
	 */
};

enum ETTServerMessageTypes
{
	ettps_hello = 0,
	ettps_ping,
	ettps_recievechat,
	ettps_loginresponse,
	ettps_roomlist,
	ettps_lobbyuserlist,
	ettps_lobbyuserlistupdate,
	ettps_recievescore,
	ettps_mpleaderboardupdate,
	ettps_createroomresponse,
	ettps_enterroomresponse,
	ettps_selectchart,
	ettps_startchart,
	ettps_deleteroom,
	ettps_newroom,
	ettps_updateroom,
	ettps_roomuserlist,
	ettps_chartrequest,
	ettps_roompacklist,
	ettps_gameplay_replay_update,
	ettps_end
};

enum ETTClientMessageTypes
{
	ettpc_login = 0,
	ettpc_ping,
	ettpc_sendchat,
	ettpc_sendscore,
	ettpc_mpleaderboardupdate,
	ettpc_createroom,
	ettpc_enterroom,
	ettpc_leaveroom,
	ettpc_selectchart,
	ettpc_startchart,
	ettpc_gameover,
	ettpc_haschart,
	ettpc_missingchart,
	ettpc_startingchart,
	ettpc_notstartingchart,
	ettpc_openoptions,
	ettpc_closeoptions,
	ettpc_openeval,
	ettpc_closeeval,
	ettpc_logout,
	ettpc_hello,
	ettpc_gameplay_judgment,
	ettpc_replay_input,
	ettpc_replay_miss,
	ettpc_replay_holddrop,
	ettpc_replay_minehit,
	ettpc_end
};

namespace NetworkConstants {

	extern std::map<ETTClientMessageTypes, std::string> ettClientMessageMap;
	extern std::map<std::string, ETTServerMessageTypes> ettServerMessageMap;

}
