/* FontManager - Interface for loading and releasing fonts. */

#ifndef FONTMANAGER_H
#define FONTMANAGER_H

class Font;
struct Game;

class FontManager
{
  public:
	FontManager();
	~FontManager();

	Font* LoadFont(const RString& sFontOrTextureFilePath, RString sChars = "");
	Font* CopyFont(Font* pFont);
	void UnloadFont(Font* fp);
	// void PruneFonts();
};

extern FontManager* FONT; // global and accessible from anywhere in our program

#endif
