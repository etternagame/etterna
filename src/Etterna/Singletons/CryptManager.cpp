#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/CryptHelpers.h"
#include "CryptManager.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "LuaManager.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"

#include "tomcrypt.h"

CryptManager* CRYPTMAN =
  nullptr; // global and accessible from anywhere in our program

static bool
HashFile(RageFileBasic& f, unsigned char buf_hash[20], int iHash)
{
	hash_state hash;
	int iRet = hash_descriptor[iHash].init(&hash);
	ASSERT_M(iRet == CRYPT_OK, error_to_string(iRet));

	std::string s;
	while (!f.AtEOF()) {
		s.erase();
		if (f.Read(s, 1024 * 4) == -1) {
			LOG->Warn("Error reading %s: %s",
					  f.GetDisplayPath().c_str(),
					  f.GetError().c_str());
			hash_descriptor[iHash].done(&hash, buf_hash);
			return false;
		}

		iRet = hash_descriptor[iHash].process(
		  &hash, (const unsigned char*)s.data(), s.size());
		ASSERT_M(iRet == CRYPT_OK, error_to_string(iRet));
	}

	iRet = hash_descriptor[iHash].done(&hash, buf_hash);
	ASSERT_M(iRet == CRYPT_OK, error_to_string(iRet));

	return true;
}

#if defined(DISABLE_CRYPTO)
CryptManager::CryptManager() {}
CryptManager::~CryptManager() {}

void
CryptManager::GetRandomBytes(void* pData, int iBytes)
{
	uint8_t* pBuf = (uint8_t*)pData;
	while (iBytes--)
		*pBuf++ = (uint8_t)RandomInt(256);
}

#else

/*
 openssl genrsa -out testing -outform DER
 openssl rsa -in testing -out testing2 -outform DER
 openssl rsa -in testing -out testing2 -pubout -outform DER
 openssl pkcs8 -inform DER -outform DER -nocrypt -in private.rsa -out
 private.der
*/

static PRNGWrapper* g_pPRNG = nullptr;

CryptManager::CryptManager()
{
	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "CRYPTMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}

	g_pPRNG = new PRNGWrapper(&yarrow_desc);
}

CryptManager::~CryptManager()
{
	SAFE_DELETE(g_pPRNG);
	// Unregister with Lua.
	LUA->UnsetGlobal("CRYPTMAN");
}

void
CryptManager::GetRandomBytes(void* pData, int iBytes)
{
	int iRet = prng_descriptor[g_pPRNG->m_iPRNG].read(
	  reinterpret_cast<unsigned char*>(pData), iBytes, &g_pPRNG->m_PRNG);
	ASSERT(iRet == iBytes);
}
#endif

std::string
CryptManager::GetMD5ForFile(const std::string& fn)
{
	RageFile file;
	if (!file.Open(fn, RageFile::READ)) {
		LOG->Warn("GetMD5: Failed to open file '%s'", fn.c_str());
		return std::string();
	}
	int iHash = register_hash(&md5_desc);
	ASSERT(iHash >= 0);

	unsigned char digest[16];
	HashFile(file, digest, iHash);

	return std::string((const char*)digest, sizeof(digest));
}

std::string
CryptManager::GetMD5ForString(const std::string& sData)
{
	unsigned char digest[16];

	int iHash = register_hash(&md5_desc);

	hash_state hash;
	hash_descriptor[iHash].init(&hash);
	hash_descriptor[iHash].process(
	  &hash, (const unsigned char*)sData.data(), sData.size());
	hash_descriptor[iHash].done(&hash, digest);

	return std::string((const char*)digest, sizeof(digest));
}

std::string
CryptManager::GetSHA1ForString(const std::string& sData)
{
	unsigned char digest[20];

	int iHash = register_hash(&sha1_desc);

	hash_state hash;
	hash_descriptor[iHash].init(&hash);
	hash_descriptor[iHash].process(
	  &hash, (const unsigned char*)sData.data(), sData.size());
	hash_descriptor[iHash].done(&hash, digest);

	return std::string((const char*)digest, sizeof(digest));
}

std::string
CryptManager::GetSHA1ForFile(const std::string& fn)
{
	RageFile file;
	if (!file.Open(fn, RageFile::READ)) {
		LOG->Warn("GetSHA1: Failed to open file '%s'", fn.c_str());
		return std::string();
	}
	int iHash = register_hash(&sha1_desc);
	ASSERT(iHash >= 0);

	unsigned char digest[20];
	HashFile(file, digest, iHash);

	return std::string((const char*)digest, sizeof(digest));
}

/* Generate a version 4 random UUID. */
std::string
CryptManager::GenerateRandomUUID()
{
	uint32_t buf[4];
	CryptManager::GetRandomBytes(buf, sizeof(buf));

	buf[1] &= 0xFFFF0FFF;
	buf[1] |= 0x00004000;
	buf[2] &= 0x0FFFFFFF;
	buf[2] |= 0xA0000000;

	return ssprintf("%08x-%04x-%04x-%04x-%04x%08x",
					buf[0],
					buf[1] >> 16,
					buf[1] & 0xFFFF,
					buf[2] >> 16,
					buf[2] & 0xFFFF,
					buf[3]);
}

// lua start

/** @brief Allow Lua to have access to the CryptManager. */
class LunaCryptManager : public Luna<CryptManager>
{
  public:
	static int MD5String(T* p, lua_State* L)
	{
		std::string md5out;
		md5out = p->GetMD5ForString(SArg(1));
		lua_pushlstring(L, md5out.c_str(), md5out.size());
		return 1;
	}
	static int MD5File(T* p, lua_State* L)
	{
		std::string md5fout;
		md5fout = p->GetMD5ForFile(SArg(1));
		lua_pushlstring(L, md5fout.c_str(), md5fout.size());
		return 1;
	}
	static int SHA1String(T* p, lua_State* L)
	{
		std::string sha1out;
		sha1out = p->GetSHA1ForString(SArg(1));
		lua_pushlstring(L, sha1out.c_str(), sha1out.size());
		return 1;
	}
	static int SHA1File(T* p, lua_State* L)
	{
		std::string sha1fout;
		sha1fout = p->GetSHA1ForFile(SArg(1));
		lua_pushlstring(L, sha1fout.c_str(), sha1fout.size());
		return 1;
	}
	static int GenerateRandomUUID(T* p, lua_State* L)
	{
		std::string uuidOut;
		uuidOut = p->GenerateRandomUUID();
		lua_pushlstring(L, uuidOut.c_str(), uuidOut.size());
		return 1;
	}

	LunaCryptManager()
	{
		ADD_METHOD(MD5String);
		ADD_METHOD(MD5File);
		ADD_METHOD(SHA1String);
		ADD_METHOD(SHA1File);
		ADD_METHOD(GenerateRandomUUID);
	}
};

LUA_REGISTER_CLASS(CryptManager)

// lua end
