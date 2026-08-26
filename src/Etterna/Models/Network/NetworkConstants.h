#pragma once

#include <string>
#include <map>

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
	ettps_spectating_update,
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
