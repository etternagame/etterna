#ifndef BITMAP_TEXT_H
#define BITMAP_TEXT_H

#include "Actor.h"

class RageTexture;
class Font;
struct FontPageTextures;
/** @brief An actor that holds a Font and draws text to the screen. */
class BitmapText : public Actor
{
  public:
	BitmapText();
	BitmapText(const BitmapText& cpy);
	auto operator=(const BitmapText& cpy) -> BitmapText&;
	~BitmapText() override;

	void LoadFromNode(const XNode* pNode) override;
	[[nodiscard]] auto Copy() const -> BitmapText* override;

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
		auto operator==(BMT_TweenState const& other) const -> bool;
		auto operator!=(BMT_TweenState const& other) const -> bool
		{
			return !operator==(other);
		}
		void SetStrokeColor(RageColor const& c) { m_stroke_color = c; }
		[[nodiscard]] auto GetStrokeColor() const -> RageColor const&
		{
			return m_stroke_color;
		}

	  private:
		RageColor m_stroke_color;
	};

	auto BMT_DestTweenState() -> BMT_TweenState&
	{
		if (BMT_Tweens.empty()) {
			return BMT_current;
		}

		return BMT_Tweens.back();
	}

	[[nodiscard]] auto BMT_DestTweenState() const -> BMT_TweenState const&
	{
		return const_cast<BitmapText*>(this)->BMT_DestTweenState();
	}

	void SetCurrentTweenStart() override;
	void EraseHeadTween() override;
	void UpdatePercentThroughTween(float between) override;
	void BeginTweening(float time, ITween* interp) override;
	// This function exists because the compiler tried to connect a call of
	// "BeginTweening(1.2f)" to the function above. -Kyz
	virtual void BeginTweening(float time, TweenType tt = TWEEN_LINEAR) override
	{
		Actor::BeginTweening(time, tt);
	}
	void StopTweening() override;
	void FinishTweening() override;

	auto LoadFromFont(const std::string& sFontName) -> bool;
	auto LoadFromTextureAndChars(const std::string& sTexturePath,
								 const std::string& sChars) -> bool;
	virtual void SetText(const std::string& sText,
						 const std::string& sAlternateText = "",
						 int iWrapWidthPixels = -1);
	void SetVertSpacing(int iSpacing);
	void SetMaxWidth(float fMaxWidth);
	void SetMaxHeight(float fMaxHeight);
	void SetMaxDimUseZoom(bool use);
	void SetWrapWidthPixels(int iWrapWidthPixels);
	void CropLineToWidth(size_t l, int width);
	void CropToWidth(int width);

	[[nodiscard]] auto EarlyAbortDraw() const -> bool override;
	void DrawPrimitives() override;

	void SetUppercase(bool b);
	void SetRainbowScroll(bool b) { m_bRainbowScroll = b; }
	void SetJitter(bool b) { m_bJitter = b; }
	void SetDistortion(float f);
	void UnSetDistortion();
	void set_mult_attrs_with_diffuse(bool m);
	[[nodiscard]] auto get_mult_attrs_with_diffuse() const -> bool;

	void SetHorizAlign(float f) override;

	void SetStrokeColor(const RageColor& c)
	{
		BMT_DestTweenState().SetStrokeColor(c);
	}
	auto GetStrokeColor() -> RageColor const&
	{
		return BMT_DestTweenState().GetStrokeColor();
	}
	void SetCurrStrokeColor(const RageColor& c)
	{
		BMT_current.SetStrokeColor(c);
	}
	auto GetCurrStrokeColor() -> RageColor const&
	{
		return BMT_current.GetStrokeColor();
	}

	void SetTextGlowMode(TextGlowMode tgm) { m_TextGlowMode = tgm; }

	void GetLines(std::vector<std::wstring>& wTextLines) const
	{
		wTextLines = m_wTextLines;
	}

	[[nodiscard]] auto GetLines() const -> const std::vector<std::wstring>&
	{
		return m_wTextLines;
	}

	[[nodiscard]] auto GetText() const -> std::string { return m_sText; }
	// Return true if the string 's' will use an alternate string, if available.
	[[nodiscard]] auto StringWillUseAlternate(
	  const std::string& sText,
	  const std::string& sAlternateText) const -> bool;

	struct Attribute
	{
		int length{ -1 };
		RageColor diffuse[NUM_DIFFUSE_COLORS];
		RageColor glow;

		void FromStack(lua_State* L, int iPos);
	};

	[[nodiscard]] auto GetDefaultAttribute() const -> Attribute;
	void AddAttribute(size_t iPos, const Attribute& attr);
	void ClearAttributes();

	// Commands
	void PushSelf(lua_State* L) override;

	std::vector<RageSpriteVertex> m_aVertices;

  protected:
	Font* m_pFont;
	bool m_bUppercase;
	std::string m_sText;
	std::vector<std::wstring> m_wTextLines;
	std::vector<int> m_iLineWidths; // in source pixels
	int m_iWrapWidthPixels;			// -1 = no wrap
	float m_fMaxWidth;				// 0 = no max
	float m_fMaxHeight;				// 0 = no max
	bool m_MaxDimensionUsesZoom;
	bool m_bRainbowScroll;
	bool m_bJitter;
	bool m_bUsingDistortion;
	bool m_mult_attrs_with_diffuse;
	float m_fDistortion;
	int m_iVertSpacing;

	std::vector<FontPageTextures*> m_vpFontPageTextures;
	std::map<size_t, Attribute> m_mAttributes;
	bool m_bHasGlowAttribute;

	TextGlowMode m_TextGlowMode;

	// recalculate the items in SetText()
	void BuildChars();
	void DrawChars(bool bUseStrokeTexture);
	void UpdateBaseZoom();

  private:
	void SetTextInternal();
	std::vector<BMT_TweenState> BMT_Tweens;
	BMT_TweenState BMT_current;
	BMT_TweenState BMT_start;
};

// With the addition of Attributes to BitmapText, this class may very well be
// redundant. (Leave it in for now, though.) -aj
class ColorBitmapText : public BitmapText
{
  public:
	[[nodiscard]] auto Copy() const -> ColorBitmapText* override;
	void SetText(const std::string& sText,
				 const std::string& sAlternateText = "",
				 int iWrapWidthPixels = -1) override;
	void ResetText();
	void DrawPrimitives() override;
	int lines = 0;
	void SetMaxLines(int iNumLines, int iDirection, unsigned int& scroll);
	void SetMaxLines(int iLines, bool bCutBottom = true); // if bCutBottom =
														  // false then, it will
														  // crop the top
	void SimpleAddLine(const std::string& sAddition, int iWidthPixels);
	void SetMaxLines(int iNumLines, int iDirection);
	void PushSelf(lua_State* L) override;

  protected:
	struct ColorChange
	{
		RageColor c; // Color to change to
		int l{};	 // Change Location
	};
	std::vector<ColorChange> m_vColors;
};

#endif
