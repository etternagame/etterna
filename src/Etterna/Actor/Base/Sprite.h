#ifndef SPRITE_H
#define SPRITE_H

#include "Actor.h"
#include "RageUtil/Graphics/RageTextureID.h"

void
TexCoordArrayFromRect(float fImageCoords[8], const RectF& rect);

class RageTexture;
/** @brief A bitmap Actor that animates and moves around. */
class Sprite : public Actor
{
  public:
	/** @brief The Sprite's present state. */
	struct State
	{
		RectF rect;
		/** @brief The number of "seconds to show". */
		float fDelay;
	};

	Sprite();
	Sprite(const Sprite& cpy);
	~Sprite() override;

	// See explanation in source.
	static auto NewBlankSprite() -> Sprite*;

	void InitState() override;

	void LoadFromNode(const XNode* pNode) override;
	[[nodiscard]] auto Copy() const -> Sprite* override;

	[[nodiscard]] auto EarlyAbortDraw() const -> bool override;
	void DrawPrimitives() override;
	void Update(float fDeltaTime) override;

	void
	UpdateAnimationState(); // take m_fSecondsIntoState, and move to a new state

	// Adjust texture properties for song backgrounds.
	static auto SongBGTexture(RageTextureID ID) -> RageTextureID;

	// Adjust texture properties for song banners.
	static auto SongBannerTexture(RageTextureID ID) -> RageTextureID;

	virtual void Load(const RageTextureID& ID);
	void SetTexture(std::shared_ptr<RageTexture> pTexture);

	void UnloadTexture();
	[[nodiscard]] auto GetTexture() const -> std::shared_ptr<RageTexture>
	{
		return m_pTexture;
	};

	void EnableAnimation(bool bEnable) override;

	[[nodiscard]] auto GetNumStates() const -> int override;
	void SetState(int iNewState) override;
	[[nodiscard]] auto GetState() const -> int { return m_iCurState; }

	[[nodiscard]] auto GetAnimationLengthSeconds() const -> float override
	{
		return m_animation_length_seconds;
	}
	virtual void RecalcAnimationLengthSeconds();
	void SetSecondsIntoAnimation(float fSeconds) override;
	void SetStateProperties(const std::vector<State>& new_states)
	{
		m_States = new_states;
		RecalcAnimationLengthSeconds();
		SetState(0);
	}

	[[nodiscard]] auto GetTexturePath() const -> std::string;

	void SetCustomTextureRect(const RectF& new_texcoord_frect);
	void SetCustomTextureCoords(float fTexCoords[8]);
	void SetCustomImageRect(RectF rectImageCoords); // in image pixel space
	void SetCustomImageCoords(float fImageCoords[8]);
	void SetCustomPosCoords(float fPosCoords[8]);
	[[nodiscard]] auto GetCurrentTextureCoordRect() const -> const RectF*;
	[[nodiscard]] auto GetTextureCoordRectForState(int iState) const
	  -> const RectF*;
	void StopUsingCustomCoords();
	void StopUsingCustomPosCoords();
	void GetActiveTextureCoords(float fTexCoordsOut[8]) const;
	void StretchTexCoords(float fX, float fY);
	void AddImageCoords(float fX, float fY); // in image pixel space
	void SetEffectMode(EffectMode em) { m_EffectMode = em; }

	void LoadFromCached(const std::string& sDir, const std::string& sPath);
	void SetTexCoordVelocity(float fVelX, float fVelY);
	/**
	 * @brief Scale the Sprite while maintaining the aspect ratio.
	 *
	 * It has to fit within and become clipped to the given parameters.
	 * @param fWidth the new width.
	 * @param fHeight the new height. */
	void ScaleToClipped(float fWidth, float fHeight);
	void CropTo(float fWidth, float fHeight);

	// Commands
	void PushSelf(lua_State* L) override;

	void SetAllStateDelays(float fDelay);

	bool m_DecodeMovie;

	bool m_use_effect_clock_for_texcoords;

  protected:
	void LoadFromTexture(const RageTextureID& ID);

  private:
	void LoadStatesFromTexture();

	void DrawTexture(const TweenState* state);

	std::shared_ptr<RageTexture> m_pTexture;

	std::vector<State> m_States;
	int m_iCurState;
	/** @brief The number of seconds that have elapsed since we switched to this
	 * frame. */
	float m_fSecsIntoState;
	float m_animation_length_seconds;

	EffectMode m_EffectMode;
	bool m_bUsingCustomTexCoords;
	bool m_bUsingCustomPosCoords;
	bool m_bSkipNextUpdate;
	/**
	 * @brief Set up the coordinates for the texture.
	 *
	 * The first two parameters are the (x, y) coordinates for the top left.
	 * The remaining six are for the (x, y) coordinates for bottom left,
	 * bottom right, and top right respectively. */
	float m_CustomTexCoords[8];
	/**
	 * @brief Set up the coordinates for the position.
	 *
	 * These are offsets for the quad the sprite will be drawn to.
	 * The first two are the (x, y) offsets for the top left.
	 * The remaining six are for the (x, y) coordinates for bottom left,
	 * bottom right, and top right respectively.
	 * These are offsets instead of a replacement for m_size to avoid
	 * complicating the cropping code. */
	float m_CustomPosCoords[8];

	// Remembered clipped dimensions are applied on Load().
	// -1 means no remembered dimensions;
	float m_fRememberedClipWidth, m_fRememberedClipHeight;

	float m_fTexCoordVelocityX;
	float m_fTexCoordVelocityY;
};

#endif
