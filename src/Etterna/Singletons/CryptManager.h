#ifndef CryptManager_H
#define CryptManager_H

class RageFileBasic;
struct lua_State;

const std::string SIGNATURE_APPEND = ".sig";

class CryptManager
{
  public:
	CryptManager();
	~CryptManager();

	static void GetRandomBytes(void* pData, int iBytes);

	static auto GetMD5ForString(const std::string& sData)
	  -> std::string; // in binary
	static auto GetMD5ForFile(const std::string& fn)
	  -> std::string; // in binary
	static auto GetSHA1ForString(const std::string& sData)
	  -> std::string; // in binary
	static auto GetSHA1ForFile(const std::string& fn)
	  -> std::string; // in binary
	static auto GetSHA256ForString(const std::string& sData) -> std::string;
	static auto GetSHA256ForFile(const std::string& fn) -> std::string;
	// Lua
	void PushSelf(lua_State* L);
};

extern CryptManager*
  CRYPTMAN; // global and accessible from anywhere in our program

#endif
