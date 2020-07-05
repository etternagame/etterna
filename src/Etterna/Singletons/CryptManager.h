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
	static bool Sign(const std::string& sPath,
					 std::string& sSignatureOut,
					 const std::string& sPrivateKey);
	static bool VerifyFileWithFile(const std::string& sPath,
								   const std::string& sSignatureFile = "");
	static bool VerifyFileWithFile(const std::string& sPath,
								   std::string sSignatureFile,
								   const std::string& sPublicKeyFile);
	static bool Verify(RageFileBasic& file,
					   const std::string& sSignature,
					   const std::string& sPublicKey);

	static void GetRandomBytes(void* pData, int iBytes);
	static std::string GenerateRandomUUID();

	static std::string GetMD5ForFile(const std::string& fn);	   // in binary
	static std::string GetMD5ForString(const std::string& sData);  // in binary
	static std::string GetSHA1ForString(const std::string& sData); // in binary
	static std::string GetSHA1ForFile(const std::string& fn);	   // in binary
	static std::string GetSHA256ForString(const std::string& sData);

	static std::string GetPublicKeyFileName();

	// Lua
	void PushSelf(lua_State* L);
};

extern CryptManager*
  CRYPTMAN; // global and accessible from anywhere in our program

#endif
