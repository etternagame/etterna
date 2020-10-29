#include "Etterna/Globals/global.h"
#include "CryptManager.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "LuaManager.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/File/RageFileManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"

#include "openssl/rand.h"
#include "openssl/sha.h"
#include "openssl/md5.h"

#include <functional>

CryptManager* CRYPTMAN =
  nullptr; // global and accessible from anywhere in our program

///@brief Read from a file, running the lamda `hash` on every read block
static bool
HashFile(std::string fn,
		 std::function<void(const unsigned char* data, size_t length)> hash)
{
	RageFile file;
	if (!file.Open(fn, RageFile::READ)) {
		Locator::getLogger()->warn("GetMD5ForFile: Failed to open file '{}'", fn.c_str());
		return false;
	}

	std::string s;
	while (!file.AtEOF()) {
		s.erase();
		if (file.Read(s, 1024 * 4) == -1) {
			Locator::getLogger()->warn("Error reading {}: {}", file.GetDisplayPath(), file.GetError());
			return false;
		}
		hash(reinterpret_cast<const unsigned char*>(s.data()), s.size());
	}
	return true;
}

/*
 openssl genrsa -out testing -outform DER
 openssl rsa -in testing -out testing2 -outform DER
 openssl rsa -in testing -out testing2 -pubout -outform DER
 openssl pkcs8 -inform DER -outform DER -nocrypt -in private.rsa -out
 private.der
*/

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
}

CryptManager::~CryptManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("CRYPTMAN");
}

void
CryptManager::GetRandomBytes(void* pData, int iBytes)
{
	int retval = RAND_bytes((unsigned char*)pData, iBytes);
	if (retval != 1) {
		Locator::getLogger()->warn("The error {} occured in RAND_bytes", retval);
	}
}

std::string
CryptManager::GetMD5ForFile(const std::string& fn)
{
	MD5_CTX* hash = new MD5_CTX;
	MD5_Init(hash);
	auto update = [&hash](const unsigned char* data, size_t length) {
		MD5_Update(hash, data, length);
	};
	if (!HashFile(fn, update)) {
		Locator::getLogger()->warn("An error occurred when calculating MD5 of \n{}", fn.c_str());
		return std::string();
	}
	unsigned char digest[MD5_DIGEST_LENGTH];
	MD5_Final(digest, hash);
	return std::string(reinterpret_cast<const char*>(digest), sizeof(digest));
}

std::string
CryptManager::GetMD5ForString(const std::string& sData)
{
	unsigned char digest[MD5_DIGEST_LENGTH];
	const unsigned char* data =
	  reinterpret_cast<const unsigned char*>(sData.data());
	MD5(data, sData.size(), digest);
	return std::string(reinterpret_cast<const char*>(digest), sizeof(digest));
}

std::string
CryptManager::GetSHA1ForString(const std::string& sData)
{
	unsigned char digest[SHA_DIGEST_LENGTH];
	const unsigned char* data =
	  reinterpret_cast<const unsigned char*>(sData.data());
	SHA1(data, sData.size(), digest);
	return std::string(reinterpret_cast<const char*>(digest), sizeof(digest));
}

std::string
CryptManager::GetSHA1ForFile(const std::string& fn)
{
	SHA_CTX* hash = new SHA_CTX;
	SHA1_Init(hash);
	auto update = [&hash](const unsigned char* data, size_t length) {
		SHA1_Update(hash, data, length);
	};
	if (!HashFile(fn, update)) {
		Locator::getLogger()->warn("An error occurred when calculating SHA1 of \n{}",
				  fn.c_str());
		return std::string();
	}
	unsigned char digest[SHA_DIGEST_LENGTH];
	SHA1_Final(digest, hash);
	return std::string(reinterpret_cast<const char*>(digest), sizeof(digest));
}

std::string
CryptManager::GetSHA256ForString(const std::string& sData)
{
	unsigned char digest[SHA256_DIGEST_LENGTH];
	const unsigned char* data =
	  reinterpret_cast<const unsigned char*>(sData.data());
	SHA256(data, sData.size(), digest);
	return std::string(reinterpret_cast<const char*>(digest), sizeof(digest));
}

std::string
CryptManager::GetSHA256ForFile(const std::string& fn)
{
	SHA256_CTX* hash = new SHA256_CTX;
	SHA256_Init(hash);
	auto update = [&hash](const unsigned char* data, size_t length) {
		SHA256_Update(hash, data, length);
	};
	if (!HashFile(fn, update)) {
		Locator::getLogger()->warn("An error occurred when calculating SHA256 of \n{}",
				  fn.c_str());
		return std::string();
	}
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256_Final(digest, hash);
	return std::string(reinterpret_cast<const char*>(digest), sizeof(digest));
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
	static int SHA256String(T* p, lua_State* L)
	{
		std::string sha256out;
		sha256out = p->GetSHA256ForString(SArg(1));
		lua_pushlstring(L, sha256out.c_str(), sha256out.size());
		return 1;
	}
	static int SHA256File(T* p, lua_State* L)
	{
		std::string sha256fout;
		sha256fout = p->GetSHA256ForFile(SArg(1));
		lua_pushlstring(L, sha256fout.c_str(), sha256fout.size());
		return 1;
	}
	LunaCryptManager()
	{
		ADD_METHOD(MD5String);
		ADD_METHOD(MD5File);
		ADD_METHOD(SHA1String);
		ADD_METHOD(SHA1File);
		ADD_METHOD(SHA256String);
		ADD_METHOD(SHA256File);
	}
};

LUA_REGISTER_CLASS(CryptManager)

// lua end
