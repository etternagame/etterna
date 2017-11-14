/** @brief Font - stores a font, used by BitmapText. */

#ifndef FONT_H
#define FONT_H

#include "RageTextureID.h"
#include "RageUtil.h"
#include "RageTypes.h"
#include <map>

class FontPage;
class RageTexture;
class IniFile;

/** @brief The textures used by the font. */
struct FontPageTextures
{
	/** @brief The primary texture drawn underneath Main. */
	RageTexture *m_pTextureMain;
	/** @brief an optional texture drawn underneath Main.
	 *
	 * This can help to acheive complicated layer styles. */
	RageTexture *m_pTextureStroke;

	/** @brief Set up the initial textures. */
	FontPageTextures(): m_pTextureMain(NULL), m_pTextureStroke(NULL) {}
};

/** @brief The components of a glyph (not technically a character). */
struct glyph
{
	/** @brief the FontPage that is needed. */
	FontPage *m_pPage;
	/** @brief the textures for the glyph. */
	FontPageTextures m_FontPageTextures;
	FontPageTextures *GetFontPageTextures() const { return const_cast<FontPageTextures *>(&m_FontPageTextures); }

	/** @brief Number of pixels to advance horizontally after drawing this character. */
	int m_iHadvance{0};

	/** @brief Width of the actual rendered character. */
	float m_fWidth{0};
	/** @brief Height of the actual rendered character. */
	float m_fHeight{0};

	/** @brief Number of pixels to offset this character when rendering. */
	float m_fHshift{0}; // , m_fVshift;

	/** @brief Texture coordinate rect. */
	RectF m_TexRect;
	
	/** @brief Set up the glyph with default values. */
	glyph() : m_pPage(NULL), m_FontPageTextures(),  m_TexRect() {}
};

/** @brief The settings used for the FontPage. */
struct FontPageSettings
{
	RString m_sTexturePath;

	int m_iDrawExtraPixelsLeft{0},
		m_iDrawExtraPixelsRight{0},
		m_iAddToAllWidths{0},
		m_iLineSpacing{-1},
		m_iTop{-1},
		m_iBaseline{-1},
		m_iDefaultWidth{-1},
		m_iAdvanceExtraPixels{1};
	float m_fScaleAllWidthsBy{1};
	RString m_sTextureHints;

	map<wchar_t,int> CharToGlyphNo;
	// If a value is missing, the width of the texture frame is used.
	map<int,int> m_mapGlyphWidths;

	/** @brief The initial settings for the FontPage. */
	FontPageSettings(): m_sTexturePath(""),
		m_sTextureHints("default"),
		CharToGlyphNo(),
		m_mapGlyphWidths()
	{ }

	/**
	 * @brief Map a range from a character map to glyphs.
	 * @param sMapping the intended mapping.
	 * @param iMapOffset the number of maps to offset.
	 * @param iGlyphOffset the number of glyphs to offset.
	 * @param iCount the range to map. If -1, the range is the entire map.
	 * @return the empty string on success, or an error message on failure. */
	RString MapRange( const RString &sMapping, int iMapOffset, int iGlyphOffset, int iCount );
};

class FontPage
{
public:
	FontPage();
	~FontPage();

	void Load( const FontPageSettings &cfg );

	// Page-global properties.
	int m_iHeight{0};
	int m_iLineSpacing{0};
	float m_fVshift{0};
	int GetCenter() const { return m_iHeight/2; }

	// Remember these only for GetLineWidthInSourcePixels.
	int m_iDrawExtraPixelsLeft{0},
	m_iDrawExtraPixelsRight{0};

	FontPageTextures m_FontPageTextures;

	// XXX: remove?
	RString m_sTexturePath;

	/** @brief All glyphs in this list will point to m_pTexture. */
	vector<glyph> m_aGlyphs;

	map<wchar_t,int> m_iCharToGlyphNo;

private:
	void SetExtraPixels( int iDrawExtraPixelsLeft, int DrawExtraPixelsRight );
	void SetTextureCoords( const vector<int> &aiWidths, int iAdvanceExtraPixels );
};

class Font
{
public:
	int m_iRefCount{1};
	RString path;

	Font();
	~Font();

	const glyph &GetGlyph( wchar_t c ) const;

	int GetLineWidthInSourcePixels( const wstring &szLine ) const;
	int GetLineHeightInSourcePixels( const wstring &szLine ) const;
	int GetGlyphsThatFit(const wstring& line, int* width) const;

	bool FontCompleteForString( const wstring &str ) const;

	/**
	 * @brief Add a FontPage to this font.
	 * @param fp the FontPage to be added.
	 */
	void AddPage(FontPage *fp);

	/**
	 * @brief Steal all of a font's pages.
	 * @param f the font whose pages we are stealing. */
	void MergeFont(Font &f);

	void Load(const RString &sFontOrTextureFilePath, const RString &sChars);
	void Unload();
	void Reload();

	// Load font-wide settings.
	void CapsOnly();

	int GetHeight() const { return m_pDefault->m_iHeight; }
	int GetCenter() const { return m_pDefault->GetCenter(); }
	int GetLineSpacing() const { return m_pDefault->m_iLineSpacing; }

	void SetDefaultGlyph( FontPage *pPage );

	bool IsRightToLeft() const { return m_bRightToLeft; };
	const RageColor &GetDefaultStrokeColor() const { return m_DefaultStrokeColor; };

private:
	/** @brief List of pages and fonts that we use (and are responsible for freeing). */
	vector<FontPage *> m_apPages;

	/**
	 * @brief This is the primary fontpage of this font.
	 * 
	 * The font-wide height, center, etc. is pulled from it. 
	 * (This is one of pages[].) */
	FontPage *m_pDefault;

	/** @brief Map from characters to glyphs. */
	map<wchar_t,glyph*> m_iCharToGlyph;
	/** @brief Each glyph is part of one of the pages[]. */
	glyph *m_iCharToGlyphCache[128];

	/**
	 * @brief True for Hebrew, Arabic, Urdu fonts.
	 *
	 * This will also change the way glyphs from the default FontPage are rendered. 
	 * There may be a better way to handle this. */
	bool m_bRightToLeft{false};

	RageColor m_DefaultStrokeColor;

	/** @brief We keep this around only for reloading. */
	RString m_sChars;

	void LoadFontPageSettings( FontPageSettings &cfg, IniFile &ini, const RString &sTexturePath, const RString &PageName, const RString &sChars );
	static void GetFontPaths( const RString &sFontOrTextureFilePath, vector<RString> &sTexturePaths );
	RString GetPageNameFromFileName( const RString &sFilename );
	
	Font(const Font& rhs);
	Font& operator=(const Font& rhs);
};

/**
 * @brief Last private-use Unicode character:
 * 
 * This is in the header to reduce file dependencies. */
const wchar_t FONT_DEFAULT_GLYPH = 0xF8FF;

#endif

/**
 * @file
 * @author Glenn Maynard, Chris Danford (c) 2001-2004
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
