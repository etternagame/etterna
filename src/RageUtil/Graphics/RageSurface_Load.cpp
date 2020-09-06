#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "RageUtil/File/RageFile.h"
#include "Core/Services/Locator.hpp"
#include "RageSurface_Load.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include "RageUtil/Utils/RageUtil.h"
#include "RageSurface.h"

#include <set>

RageSurfaceUtils::OpenResult
RageSurface_stb_Load(const std::string& sPath,
					 RageSurface*& ret,
					 bool bHeaderOnly,
					 std::string& error)
{
	RageFile f;
	if (!f.Open(sPath)) {
		error = f.GetError();
		return RageSurfaceUtils::OPEN_FATAL_ERROR;
	}

	int x, y, n;
	const auto doot = stbi_load(f.GetPath().c_str(), &x, &y, &n, 4);
	if (doot == nullptr) {
		return RageSurfaceUtils::OPEN_FATAL_ERROR;
	}
	if (bHeaderOnly) {
		ret = CreateSurfaceFrom(x, y, 32, 0, 0, 0, 0, nullptr, x * 4);
		stbi_image_free(doot);
		return RageSurfaceUtils::OPEN_OK;
	} else {
		ret = CreateSurfaceFrom(x,
								y,
								32,
								Swap32BE(0xFF000000),
								Swap32BE(0x00FF0000),
								Swap32BE(0x0000FF00),
								Swap32BE(0x000000FF),
								doot,
								x * 4);
	}

	if (ret == nullptr) {
		stbi_image_free(doot);
		return RageSurfaceUtils::OPEN_UNKNOWN_FILE_FORMAT; // XXX
	}
	ret->stb_loadpoint = true;
	return RageSurfaceUtils::OPEN_OK;
}
static RageSurface*
TryOpenFile(const std::string& sPath,
			bool bHeaderOnly,
			std::string& error,
			const std::string& format,
			bool& bKeepTrying)
{
	RageSurface* ret = nullptr;
	RageSurfaceUtils::OpenResult result;
	result = RageSurface_stb_Load(sPath, ret, bHeaderOnly, error);

	if (result == RageSurfaceUtils::OPEN_OK) {
		ASSERT(ret != nullptr);
		return ret;
	}

	//Locator::getLogger()->trace("Format {} failed: {}", format.c_str(), error.c_str());
	return nullptr;
}

RageSurface*
RageSurfaceUtils::LoadFile(const std::string& sPath,
						   std::string& error,
						   bool bHeaderOnly)
{
	{
		RageFile TestOpen;
		if (!TestOpen.Open(sPath)) {
			error = TestOpen.GetError();
			return nullptr;
		}
	}

	std::set<std::string> FileTypes;
	auto const& exts = ActorUtil::GetTypeExtensionList(FT_Bitmap);
	for (const auto& ext : exts) {
		FileTypes.insert(ext);
	}

	auto format = GetExtension(sPath);
	MakeLower(format);

	auto bKeepTrying = true;

	/* If the extension matches a format, try that first. */
	if (FileTypes.find(format) != FileTypes.end()) {
		const auto ret =
		  TryOpenFile(sPath, bHeaderOnly, error, format, bKeepTrying);
		if (ret)
			return ret;
		FileTypes.erase(format);
	}

	for (auto it = FileTypes.begin(); bKeepTrying && it != FileTypes.end();
		 ++it) {
		const auto ret =
		  TryOpenFile(sPath, bHeaderOnly, error, *it, bKeepTrying);
		if (ret) {
            Locator::getLogger()->info("Graphic file", sPath, "is really %s", it->c_str());
			return ret;
		}
	}

	return nullptr;
}
