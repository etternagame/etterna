#ifndef TEXTURE_FONT_H
#define TEXTURE_FONT_H

#include <vector>
#include <map>
using namespace std;

struct FontPageDescription
{
	CString name;
	vector<wchar_t> chars;
};

struct FontPage
{
	FontPage();
	~FontPage();

	void Create(unsigned width, unsigned height);

	HBITMAP m_hPage;

	/* Width and height of this page: */
	//	int Width, Height;

	int m_iFrameWidth, m_iFrameHeight;
	int m_iNumFramesX, m_iNumFramesY;
};

/* Create a bitmap font with the given parameters. */
class TextureFont
{
  public:
	TextureFont();
	~TextureFont();

	vector<FontPageDescription> m_PagesToGenerate;
	void FormatFontPage(int iPage, HDC hDC);
	void FormatFontPages();
	void Save(CString sPath,
			  CString sBitmapAppendBeforeExtension,
			  bool bSaveMetrics,
			  bool bSaveBitmaps,
			  bool bExportStrokeTemplates);

	map<wchar_t, HBITMAP> m_Characters;

	/* Font generation properties: */
	bool m_bBold;			 /* whether font is bold */
	bool m_bItalic;			 /* whether font is italic */
	bool m_bAntiAlias;		 /* antialiasing type */
	float m_fFontSizePixels; /* font size in pixels */
	CString m_sFamily;		 /* font family */
	int m_iPadding;			 /* empty padding between characters */

	/* Derived properties: */
	int m_iCharDescent, m_iCharLeftOverlap, m_iCharRightOverlap,
	  m_iCharBaseline, m_iCharTop, m_iCharVertSpacing;

	RECT m_BoundingRect;

	vector<FontPage*> m_apPages;

	CString m_sError, m_sWarnings;

  private:
	int GetTopPadding() const;

	/* Bounds of each character, according to MeasureCharacterRanges. */
	map<wchar_t, RECT> m_RealBounds;

	map<wchar_t, ABC> m_ABC;
	void FormatCharacter(wchar_t c, HDC hDC);
};

#endif
