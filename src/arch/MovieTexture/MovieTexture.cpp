#include "Etterna/Globals/global.h"
#include "MovieTexture.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/File/RageFile.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "arch/arch_default.h"

void
ForceToAscii(std::string& str)
{
	for (char& i : str)
		if (i < 0x20 || i > 0x7E)
			i = '?';
}

bool
RageMovieTexture::GetFourCC(const std::string& fn,
							std::string& handler,
							std::string& type)
{
	std::string ignore, ext;
	splitpath(fn, ignore, ignore, ext);
	if (!CompareNoCase(ext, ".mpg") || !CompareNoCase(ext, ".mpeg") ||
		!CompareNoCase(ext, ".mpv") || !CompareNoCase(ext, ".mpe")) {
		handler = type = "MPEG";
		return true;
	}
	if (!CompareNoCase(ext, ".ogv")) {
		handler = type = "Ogg";
		return true;
	}

	// Not very pretty but should do all the same error checking without
	// iostream
#define HANDLE_ERROR(x)                                                        \
	{                                                                          \
		Locator::getLogger()->warn("Error reading {}: {}", fn.c_str(), x);                      \
		handler = type = "";                                                   \
		return false;                                                          \
	}

	RageFile file;
	if (!file.Open(fn))
		HANDLE_ERROR("Could not open file.");
	if (!file.Seek(0x70))
		HANDLE_ERROR("Could not seek.");
	type = "    ";
	if (file.Read((char*)type.c_str(), 4) != 4)
		HANDLE_ERROR("Could not read.");
	ForceToAscii(type);

	if (file.Seek(0xBC) != 0xBC)
		HANDLE_ERROR("Could not seek.");
	handler = "    ";
	if (file.Read((char*)handler.c_str(), 4) != 4)
		HANDLE_ERROR("Could not read.");
	ForceToAscii(handler);

	return true;
#undef HANDLE_ERROR
}

DriverList RageMovieTextureDriver::m_pDriverList;

// Helper for MakeRageMovieTexture()
static void
DumpAVIDebugInfo(const std::string& fn)
{
	std::string type, handler;
	if (!RageMovieTexture::GetFourCC(fn, handler, type))
		return;

	Locator::getLogger()->trace("Movie {} has handler '{}', type '{}'",
			   fn.c_str(),
			   handler.c_str(),
			   type.c_str());
}

static Preference<std::string> g_sMovieDrivers("MovieDrivers",
											   ""); // "" == default
/* Try drivers in order of preference until we find one that works. */
static LocalizedString MOVIE_DRIVERS_EMPTY("Arch",
										   "Movie Drivers cannot be empty.");
static LocalizedString COULDNT_CREATE_MOVIE_DRIVER(
  "Arch",
  "Couldn't create a movie driver.");
std::shared_ptr<RageMovieTexture>
RageMovieTexture::Create(const RageTextureID& ID)
{
	DumpAVIDebugInfo(ID.filename);

	std::string sDrivers = g_sMovieDrivers;
	if (sDrivers.empty())
		sDrivers = DEFAULT_MOVIE_DRIVER_LIST;

	vector<std::string> DriversToTry;
	split(sDrivers, ",", DriversToTry, true);

	if (DriversToTry.empty())
		RageException::Throw("%s", MOVIE_DRIVERS_EMPTY.GetValue().c_str());

	std::shared_ptr<RageMovieTexture> ret;

	for (auto& Driver : DriversToTry) {
		Locator::getLogger()->trace("Initializing driver: {}", Driver);
		RageDriver* pDriverBase =
		  RageMovieTextureDriver::m_pDriverList.Create(Driver);

		if (pDriverBase == nullptr) {
			Locator::getLogger()->trace("Unknown movie driver name: {}", Driver);
			continue;
		}

		RageMovieTextureDriver* pDriver =
		  dynamic_cast<RageMovieTextureDriver*>(pDriverBase);
		ASSERT(pDriver != nullptr);

		std::string sError;
		ret = pDriver->Create(ID, sError);
		delete pDriver;

		if (ret == nullptr) {
			Locator::getLogger()->trace("Couldn't load driver {}: {}", Driver, sError.c_str());
			ret.reset();
			continue;
		}
		Locator::getLogger()->trace("Created movie texture \"{}\" with driver \"{}\"",
				   ID.filename.c_str(),
				   Driver.c_str());
		break;
	}
	if (!ret)
		RageException::Throw("%s",
							 COULDNT_CREATE_MOVIE_DRIVER.GetValue().c_str());

	return ret;
}
