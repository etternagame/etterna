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

	static void GenerateGlobalKeys();
	static void GenerateRSAKey(unsigned int keyLength,
							   std::string& sPrivKey,
							   std::string& sPubKey);
	static void GenerateRSAKeyToFile(unsigned int keyLength,
									 const std::string& privFilename,
									 const std::string& pubFilename);
	static void SignFileToFile(const std::string& sPath,
							   std::string sSignatureFile = "");
	static auto Sign(const std::string& sPath,
					 std::string& sSignatureOut,
					 const std::string& sPrivateKey) -> bool;
	static auto VerifyFileWithFile(const std::string& sPath,
								   const std::string& sSignatureFile = "")
	  -> bool;
	static auto VerifyFileWithFile(const std::string& sPath,
								   std::string sSignatureFile,
								   const std::string& sPublicKeyFile) -> bool;
	static auto Verify(RageFileBasic& file,
					   const std::string& sSignature,
					   const std::string& sPublicKey) -> bool;

	static void GetRandomBytes(void* pData, int iBytes);
	static auto GenerateRandomUUID() -> std::string;

	static auto GetMD5ForFile(const std::string& fn)
	  -> std::string; // in binary
	static auto GetMD5ForString(const std::string& sData)
	  -> std::string; // in binary
	static auto GetSHA1ForString(const std::string& sData)
	  -> std::string; // in binary
	static auto GetSHA1ForFile(const std::string& fn)
	  -> std::string; // in binary
	static auto GetSHA256ForString(const std::string& sData) -> std::string;

	static auto GetPublicKeyFileName() -> std::string;

	// Lua
	void PushSelf(lua_State* L);
};

extern CryptManager*
  CRYPTMAN; // global and accessible from anywhere in our program

#endif
