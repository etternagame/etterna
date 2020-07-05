/* RageLog - Manages logging. */

#ifndef RAGE_LOG_H
#define RAGE_LOG_H

#include <config.hpp>

class RageLog
{
  public:
	RageLog();
	~RageLog();

	void Trace(const char* fmt, ...) PRINTF(2, 3);
	void Warn(const char* fmt, ...) PRINTF(2, 3);
	void Info(const char* fmt, ...) PRINTF(2, 3);
	// Time is purely for writing profiling time data to the time log. -Kyz
	void Time(const char* fmt, ...) PRINTF(2, 3);
	void UserLog(const std::string& sType,
				 const std::string& sElement,
				 const char* fmt,
				 ...) PRINTF(4, 5);
	void Flush();

	void MapLog(const std::string& key, const char* fmt, ...) PRINTF(3, 4);
	void UnmapLog(const std::string& key);

	static const char* GetAdditionalLog();
	static const char* GetInfo();
	/* Returns NULL if past the last recent log. */
	static const char* GetRecentLog(int n);

	void SetShowLogOutput(bool show); // enable or disable logging to stdout
	void SetLogToDisk(bool b);		  // enable or disable logging to file
	void SetInfoToDisk(bool b);	   // enable or disable logging info.txt to file
	void SetUserLogToDisk(bool b); // enable or disable logging user.txt to file
	void SetFlushing(bool b);	   // enable or disable flushing

  private:
	bool m_bLogToDisk{ false };
	bool m_bInfoToDisk{ false };
	bool m_bUserLogToDisk{ false };
	bool m_bFlush{ false };
	bool m_bShowLogOutput{ false };
	void Write(int, const std::string& str);
	void UpdateMappedLog();
	void AddToInfo(const std::string& buf);
	void AddToRecentLogs(const std::string& buf);
};

extern RageLog* LOG; // global and accessible from anywhere in our program
#endif
