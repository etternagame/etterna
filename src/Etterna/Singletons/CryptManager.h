#ifndef CryptManager_H
#define CryptManager_H

class RageFileBasic;
struct lua_State;

const RString SIGNATURE_APPEND = ".sig";

class CryptManager
{
  public:
	CryptManager();
	~CryptManager();

	static void GenerateGlobalKeys();
	static void GenerateRSAKey(unsigned int keyLength,
							   RString& sPrivKey,
							   RString& sPubKey);
	static void GenerateRSAKeyToFile(unsigned int keyLength,
									 const RString& privFilename,
									 const RString& pubFilename);
	static void SignFileToFile(const RString& sPath,
							   RString sSignatureFile = "");
	static bool Sign(const RString& sPath,
					 RString& sSignatureOut,
					 const RString& sPrivateKey);
	static bool VerifyFileWithFile(const RString& sPath,
								   const RString& sSignatureFile = "");
	static bool VerifyFileWithFile(const RString& sPath,
								   RString sSignatureFile,
								   const RString& sPublicKeyFile);
	static bool Verify(RageFileBasic& file,
					   const RString& sSignature,
					   const RString& sPublicKey);

	static void GetRandomBytes(void* pData, int iBytes);
	static RString GenerateRandomUUID();

	static RString GetMD5ForFile(const RString& fn);	   // in binary
	static RString GetMD5ForString(const RString& sData);  // in binary
	static RString GetSHA1ForString(const RString& sData); // in binary
	static RString GetSHA1ForFile(const RString& fn);	  // in binary

	static RString GetPublicKeyFileName();

	// Lua
	void PushSelf(lua_State* L);
};

extern CryptManager*
  CRYPTMAN; // global and accessible from anywhere in our program

#endif
