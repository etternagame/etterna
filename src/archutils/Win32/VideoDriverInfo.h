/* GetVideoDriverInfo - Get information about Win32 video drivers. */

#ifndef VIDEO_DRIVER_INFO_H
#define VIDEO_DRIVER_INFO_H

struct VideoDriverInfo
{
	RString sProvider;
	RString sDescription;
	RString sVersion;
	RString sDate;
	RString sDeviceID;
};

RString
GetPrimaryVideoName();
bool
GetVideoDriverInfo(int iCardno, VideoDriverInfo& info);
RString
GetPrimaryVideoDriverName();

#endif
