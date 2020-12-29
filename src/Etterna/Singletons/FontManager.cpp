#include "Etterna/Globals/global.h"
#include "Etterna/Models/Fonts/Font.h"
#include "FontManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"

#include <map>

FontManager* FONT =
  nullptr; // global and accessible from anywhere in our program

// map from file name to a texture holder
typedef std::pair<std::string, std::string> FontName;
static std::map<FontName, Font*> g_mapPathToFont;

FontManager::FontManager() = default;

FontManager::~FontManager()
{
	for (std::map<FontName, Font*>::iterator i = g_mapPathToFont.begin();
		 i != g_mapPathToFont.end();
		 ++i) {
		const FontName& fn = i->first;
		Font* pFont = i->second;
		if (pFont->m_iRefCount > 0) {
			Locator::getLogger()->trace("FONT LEAK: '{}', RefCount = {}.", fn.first, pFont->m_iRefCount);
		}
		delete pFont;
	}
}

Font*
FontManager::LoadFont(const std::string& sFontOrTextureFilePath,
					  std::string sChars)
{
	Font* pFont;
	/* Convert the path to lowercase so that we don't load duplicates. Really,
	 * this does not solve the duplicate problem. We could have two copies of
	 * the same bitmap if there are equivalent but different paths
	 * (e.g. "graphics\blah.png" and "..\stepmania\graphics\blah.png" ). */

	Locator::getLogger()->trace("FontManager::LoadFont({}).", sFontOrTextureFilePath.c_str());
	const FontName NewName(sFontOrTextureFilePath, sChars);
	std::map<FontName, Font*>::iterator p = g_mapPathToFont.find(NewName);
	if (p != g_mapPathToFont.end()) {
		pFont = p->second;
		pFont->m_iRefCount++;
		return pFont;
	}

	auto* f = new Font;
	f->Load(sFontOrTextureFilePath, sChars);
	g_mapPathToFont[NewName] = f;
	return f;
}

Font*
FontManager::CopyFont(Font* pFont)
{
	++pFont->m_iRefCount;
	return pFont;
}

void
FontManager::UnloadFont(Font* fp)
{
	Locator::getLogger()->trace("FontManager::UnloadFont({}).", fp->path.c_str());

	for (std::map<FontName, Font*>::iterator i = g_mapPathToFont.begin();
		 i != g_mapPathToFont.end();
		 ++i) {
		if (i->second != fp)
			continue;

		ASSERT_M(fp->m_iRefCount > 0,
				 "Attempting to unload a font with zero ref count!");

		i->second->m_iRefCount--;

		if (fp->m_iRefCount == 0) {
			delete i->second;		  // free the texture
			g_mapPathToFont.erase(i); // and remove the key in the map
		}
		return;
	}

	FAIL_M(ssprintf("Unloaded an unknown font (%p)", fp));
}
