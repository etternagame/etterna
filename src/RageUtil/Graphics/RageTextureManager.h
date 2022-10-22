/* RageTextureManager - Interface for loading textures. */

#ifndef RAGE_TEXTURE_MANAGER_H
#define RAGE_TEXTURE_MANAGER_H

#include "RageSurface.h"
#include "RageTexture.h"

struct RageTextureManagerPrefs
{
	int m_iTextureColorDepth{ 16 };
	int m_iMovieColorDepth{ 16 };
	bool m_bDelayedDelete{ false };
	int m_iMaxTextureResolution{ 1024 };
	bool m_bHighResolutionTextures{ true };
	bool m_bMipMaps{ false };

	RageTextureManagerPrefs() = default;
	RageTextureManagerPrefs(int iTextureColorDepth,
							int iMovieColorDepth,
							bool bDelayedDelete,
							int iMaxTextureResolution,
							bool bHighResolutionTextures,
							bool bMipMaps)
	  : m_iTextureColorDepth(iTextureColorDepth)
	  , m_iMovieColorDepth(iMovieColorDepth)
	  , m_bDelayedDelete(bDelayedDelete)
	  , m_iMaxTextureResolution(iMaxTextureResolution)
	  , m_bHighResolutionTextures(bHighResolutionTextures)
	  , m_bMipMaps(bMipMaps)
	{
	}

	bool operator!=(const RageTextureManagerPrefs& rhs) const
	{
		return m_iTextureColorDepth != rhs.m_iTextureColorDepth ||
			   m_iMovieColorDepth != rhs.m_iMovieColorDepth ||
			   m_bDelayedDelete != rhs.m_bDelayedDelete ||
			   m_iMaxTextureResolution != rhs.m_iMaxTextureResolution ||
			   m_bHighResolutionTextures != rhs.m_bHighResolutionTextures ||
			   m_bMipMaps != rhs.m_bMipMaps;
	}
};

class RageTextureManager
{
  public:
	RageTextureManager();
	~RageTextureManager();
	void Update(float fDeltaTime);

	RageTexture* LoadTexture(const RageTextureID& ID);
	RageTexture* CopyTexture(
	  RageTexture* pCopy); // returns a ref to the same texture, not a deep copy
	bool IsTextureRegistered(RageTextureID ID) const;
	void RegisterTexture(RageTextureID ID, RageTexture* p);
	void VolatileTexture(const RageTextureID& ID);
	void UnloadTexture(RageTexture* t);
	void ReloadAll();

	void RegisterTextureForUpdating(const RageTextureID& id, RageTexture* tex);

	bool SetPrefs(RageTextureManagerPrefs prefs);
	RageTextureManagerPrefs GetPrefs() { return m_Prefs; };

	RageTextureID::TexPolicy GetDefaultTexturePolicy() const
	{
		return m_TexturePolicy;
	}
	void SetDefaultTexturePolicy(const RageTextureID::TexPolicy& p)
	{
		m_TexturePolicy = p;
	}

	// call this between Screens
	void DeleteCachedTextures() { GarbageCollect(screen_changed); }

	// call this on switch theme
	void DoDelayedDelete() { GarbageCollect(delayed_delete); }

	void InvalidateTextures();

	void AdjustTextureID(RageTextureID& ID) const;
	void DiagnosticOutput() const;

	void DisableOddDimensionWarning() { m_iNoWarnAboutOddDimensions++; }
	void EnableOddDimensionWarning() { m_iNoWarnAboutOddDimensions--; }
	bool GetOddDimensionWarning() const
	{
		return m_iNoWarnAboutOddDimensions == 0;
	}

	RageTextureID GetDefaultTextureID();
	RageTextureID GetScreenTextureID();
	RageSurface* GetScreenSurface();
	RageTextureManagerPrefs m_Prefs;
  private:
	void DeleteTexture(RageTexture* t);
	enum GCType
	{
		screen_changed,
		delayed_delete
	};
	void GarbageCollect(GCType type);
	RageTexture* LoadTextureInternal(RageTextureID ID);

	int m_iNoWarnAboutOddDimensions{ 0 };
	RageTextureID::TexPolicy m_TexturePolicy{ RageTextureID::TEX_DEFAULT };
};

extern RageTextureManager*
  TEXTUREMAN; // global and accessible from anywhere in our program

#endif
