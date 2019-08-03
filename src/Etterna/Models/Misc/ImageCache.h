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

	RageTextureID LoadCachedImage(const std::string& sImageDir,
								  const std::string& sImagePath);
	void CacheImage(const std::string& sImageDir,
					const std::string& sImagePath);
	void LoadImage(const std::string& sImageDir, const std::string& sImagePath);

	void Demand(const std::string& sImageDir);
	void Undemand(const std::string& sImageDir);

	void OutputStats() const;

  private:
	static std::string GetImageCachePath(const std::string& sImageDir,
										 const std::string& sImagePath);
	void UnloadAllImages();
	void CacheImageInternal(const std::string& sImageDir,
							const std::string& sImagePath);

	IniFile ImageData;
};

extern ImageCache*
  IMAGECACHE; // global and accessible from anywhere in our program

#endif
