/* GetVideoDriverInfo - Get information about Win32 video drivers. */

#ifndef VIDEO_DRIVER_INFO_H
#define VIDEO_DRIVER_INFO_H

struct VideoDriverInfo
{
	std::string sProvider;
	std::string sDescription;
	std::string sVersion;
	std::string sDate;
	std::string sDeviceID;
};

std::string
GetPrimaryVideoName();
bool
GetVideoDriverInfo(int iCardno, VideoDriverInfo& info);
std::string
GetPrimaryVideoDriverName();

#endif
