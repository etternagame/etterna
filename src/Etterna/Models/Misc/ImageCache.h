#ifndef IMAGE_CACHE_H
#define IMAGE_CACHE_H

#include "Etterna/FileTypes/IniFile.h"

#include "RageUtil/Graphics/RageTexture.h"

class LoadingWindow;
/** @brief Maintains a cache of reduced-quality banners. */
class ImageCache
{
  public:
	ImageCache();
	~ImageCache();
	void ReadFromDisk();

	auto LoadCachedImage(const std::string& sImageDir,
						 const std::string& sImagePath) -> RageTextureID;
	void CacheImage(const std::string& sImageDir,
					const std::string& sImagePath);
	void LoadImage(const std::string& sImageDir, const std::string& sImagePath);

	void Demand(const std::string& sImageDir);
	void Undemand(const std::string& sImageDir);

	void OutputStats() const;

  private:
	static auto GetImageCachePath(const std::string& sImageDir,
								  const std::string& sImagePath) -> std::string;
	void UnloadAllImages();
	void CacheImageInternal(const std::string& sImageDir,
							const std::string& sImagePath);

	IniFile ImageData;
};

extern ImageCache*
  IMAGECACHE; // global and accessible from anywhere in our program

#endif
