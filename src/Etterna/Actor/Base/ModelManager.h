#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "RageUtil/Graphics/RageModelGeometry.h"

#include <map>

struct ModelManagerPrefs
{
	bool m_bDelayedUnload;

	ModelManagerPrefs() { m_bDelayedUnload = false; }
	ModelManagerPrefs(bool bDelayedUnload)
	{
		m_bDelayedUnload = bDelayedUnload;
	}

	bool operator!=(const ModelManagerPrefs& rhs) const
	{
		return m_bDelayedUnload != rhs.m_bDelayedUnload;
	}
};
/**
 * @brief Class for loading and releasing textures.
 *
 * Funnily enough, the original documentation claimed this was an Interface. */
class ModelManager
{
  public:
	ModelManager();
	~ModelManager();

	RageModelGeometry* LoadMilkshapeAscii(const std::string& sFile,
										  bool bNeedNormals);
	void UnloadModel(RageModelGeometry* m);
	//	void ReloadAll();

	/**
	 * @brief Set up new preferences.
	 * @param prefs the new preferences to set up.
	 * @return true if the display needs to be reset, false otherwise. */
	bool SetPrefs(const ModelManagerPrefs& prefs);
	[[nodiscard]] const ModelManagerPrefs& GetPrefs() const { return m_Prefs; }

  protected:
	std::map<std::string, RageModelGeometry*> m_mapFileToGeometry;

	ModelManagerPrefs m_Prefs;
};

extern ModelManager*
  MODELMAN; // global and accessible from anywhere in our program

#endif
