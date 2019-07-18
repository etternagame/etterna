#ifndef BITMAP_TEXT_H
#define BITMAP_TEXT_H

#include "Actor.h"
#include <map>

class RageTexture;
class Font;
struct FontPageTextures;
/** @brief An actor that holds a Font and draws text to the screen. */
class BitmapText : public Actor
{
  public:
	BitmapText();
	BitmapText(const BitmapText& cpy);
	BitmapText& operator=(const BitmapText& cpy);
	~BitmapText() override;

	void LoadFromNode(const XNode* pNode) override;
	BitmapText* Copy() const override;

	struct BMT_TweenState
	{
		// We'd be better off not adding strokes to things we can't control
		// themewise (ScreenDebugOverlay for example). -Midiman
		BMT_TweenState()
		  : m_stroke_color(RageColor(0, 0, 0, 0))
		{
		}
		static void MakeWeightedAverage(BMT_TweenState& out,
										BMT_TweenState const& from,
										BMT_TweenState const& to,
										float between);
		bool operator==(BMT_TweenState const& other) const;
		bool operator!=(BMT_TweenState const& other) const
		{
			return !operator==(other);
		}
		void SetStrokeColor(RageColor const& c) { m_stroke_color = c; }
		RageColor const& GetStrokeColor() { return m_stroke_color; }

	  private:
		RageColor m_stroke_color;
	};

	BMT_TweenState& BMT_DestTweenState()
	{
		if (BMT_Tweens.empty()) {
			return BMT_current;
		}

		return BMT_Tweens.back();
	}
	BMT_TweenState const& BMT_DestTweenState() const
	{
		return const_cast<BitmapText*>(this)->BMT_DestTweenState();
	}

	void SetCurrentTweenStart() override;
	void EraseHeadTween() override;
	void UpdatePercentThroughTween(float between) override;
	void BeginTweening(float time, ITween* interp) override;
	// This function exists because the compiler tried to connect a call of
	// "BeginTweening(1.2f)" to the function above. -Kyz
	virtual void BeginTweening(float time, TweenType tt = TWEEN_LINEAR)
	{
		Actor::BeginTweening(time, tt);
	}
	void StopTweening() override;
	void FinishTweening() override;

	bool LoadFromFont(const RString& sFontName);
	bool LoadFromTextureAndChars(const RString& sTexturePath,
								 const RString& sChars);
	virtual void SetText(const RString& sText,
						 const RString& sAlternateText = "",
						 int iWrapWidthPixels = -1);
	void SetVertSpacing(int iSpacing);
	void SetMaxWidth(float fMaxWidth);
	void SetMaxHeight(float fMaxHeight);
	void SetMaxDimUseZoom(bool use);
	void SetWrapWidthPixels(int iWrapWidthPixels);
	void CropLineToWidth(size_t l, int width);
	void CropToWidth(int width);

	bool EarlyAbortDraw() const override;
	void DrawPrimitives() override;

	void SetUppercase(bool b);
	void SetRainbowScroll(bool b) { m_bRainbowScroll = b; }
	void SetJitter(bool b) { m_bJitter = b; }
	void SetDistortion(float f);
	void UnSetDistortion();
	void set_mult_attrs_with_diffuse(bool m);
	bool get_mult_attrs_with_diffuse();

	void SetHorizAlign(float f) override;

	void SetStrokeColor(const RageColor& c)
	{
		BMT_DestTweenState().SetStrokeColor(c);
	}
	RageColor const& GetStrokeColor()
	{
		return BMT_DestTweenState().GetStrokeColor();
	}
	void SetCurrStrokeColor(const RageColor& c)
	{
		BMT_current.SetStrokeColor(c);
	}
	RageColor const& GetCurrStrokeColor()
	{
		return BMT_current.GetStrokeColor();
	}

	void SetTextGlowMode(TextGlowMode tgm) { m_TextGlowMode = tgm; }

	void GetLines(vector<wstring>& wTextLines) const
	{
		wTextLines = m_wTextLines;
	}
	const vector<wstring>& GetLines() const { return m_wTextLines; }

	RString GetText() const { return m_sText; }
	// Return true if the string 's' will use an alternate string, if available.
	bool StringWillUseAlternate(const RString& sText,
								const RString& sAlternateText) const;

	struct Attribute
	{
		Attribute()
		  : glow()
		{
		}
		int length{ -1 };
		RageColor diffuse[NUM_DIFFUSE_COLORS];
		RageColor glow;

		void FromStack(lua_State* L, int iPos);
	};

	Attribute GetDefaultAttribute() const;
	void AddAttribute(size_t iPos, const Attribute& attr);
	void ClearAttributes();

	// Commands
	void PushSelf(lua_State* L) override;

	vector<RageSpriteVertex> m_aVertices;

  protected:
	Font* m_pFont;
	bool m_bUppercase;
	RString m_sText;
	vector<wstring> m_wTextLines;
	vector<int> m_iLineWidths; // in source pixels
	int m_iWrapWidthPixels;	// -1 = no wrap
	float m_fMaxWidth;		   // 0 = no max
	float m_fMaxHeight;		   // 0 = no max
	bool m_MaxDimensionUsesZoom;
	bool m_bRainbowScroll;
	bool m_bJitter;
	bool m_bUsingDistortion;
	bool m_mult_attrs_with_diffuse;
	float m_fDistortion;
	int m_iVertSpacing;

	vector<FontPageTextures*> m_vpFontPageTextures;
	map<size_t, Attribute> m_mAttributes;
	bool m_bHasGlowAttribute;

	TextGlowMode m_TextGlowMode;

	// recalculate the items in SetText()
	void BuildChars();
	void DrawChars(bool bUseStrokeTexture);
	void UpdateBaseZoom();

  private:
	void SetTextInternal();
	vector<BMT_TweenState> BMT_Tweens;
	BMT_TweenState BMT_current;
	BMT_TweenState BMT_start;
};

// With the addition of Attributes to BitmapText, this class may very well be
// redundant. (Leave it in for now, though.) -aj
class ColorBitmapText : public BitmapText
{
  public:
	ColorBitmapText* Copy() const override;
	void SetText(const RString& sText,
				 const RString& sAlternateText = "",
				 int iWrapWidthPixels = -1) override;
	void ResetText();
	void DrawPrimitives() override;
	int lines = 0;
	void SetMaxLines(int iNumLines, int iDirection, unsigned int& scroll);
	void SetMaxLines(int iLines, bool bCutBottom = true); // if bCutBottom =
														  // false then, it will
														  // crop the top
	void SimpleAddLine(const RString& sAddition, int iWidthPixels);
	void SetMaxLines(int iNumLines, int iDirection);
	void PushSelf(lua_State* L) override;

  protected:
	struct ColorChange
	{
		RageColor c; // Color to change to
		int l;		 // Change Location
	};
	vector<ColorChange> m_vColors;
};

#endif
