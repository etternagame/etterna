#ifndef ACTOR_H
#define ACTOR_H

#include "Etterna/Models/Misc/EnumHelper.h"
#include "Etterna/Models/Lua/LuaReference.h"
#include "RageUtil/Misc/RageTypes.h"
#include "Etterna/Singletons/MessageManager.h"
#include "Tween.h"

#include <map>

class XNode;
struct lua_State;
class LuaClass;

using apActorCommands = std::shared_ptr<LuaReference>;

/** @brief The background layer. */
#define DRAW_ORDER_BEFORE_EVERYTHING (-200)
/** @brief The underlay layer. */
#define DRAW_ORDER_UNDERLAY (-100)
/** @brief The decorations layer. */
#define DRAW_ORDER_DECORATIONS 0
/** @brief The overlay layer.
 *
 * Normal screen elements go here. */
#define DRAW_ORDER_OVERLAY (+100)
/** @brief The transitions layer. */
#define DRAW_ORDER_TRANSITIONS (+200)
/** @brief The over everything layer. */
#define DRAW_ORDER_AFTER_EVERYTHING (+300)

/** @brief The different horizontal alignments. */
enum HorizAlign
{
	HorizAlign_Left,   /**< Align to the left. */
	HorizAlign_Center, /**< Align to the center. */
	HorizAlign_Right,  /**< Align to the right. */
	NUM_HorizAlign,	   /**< The number of horizontal alignments. */
	HorizAlign_Invalid
};
LuaDeclareType(HorizAlign);

/** @brief The different vertical alignments. */
enum VertAlign
{
	VertAlign_Top,	  /**< Align to the top. */
	VertAlign_Middle, /**< Align to the middle. */
	VertAlign_Bottom, /**< Align to the bottom. */
	NUM_VertAlign,	  /**< The number of vertical alignments. */
	VertAlign_Invalid
};
LuaDeclareType(VertAlign);

/** @brief The left horizontal alignment constant. */
#define align_left 0.0f
/** @brief The center horizontal alignment constant. */
#define align_center 0.5f
/** @brief The right horizontal alignment constant. */
#define align_right 1.0f
/** @brief The top vertical alignment constant. */
#define align_top 0.0f
/** @brief The middle vertical alignment constant. */
#define align_middle 0.5f
/** @brief The bottom vertical alignment constant. */
#define align_bottom 1.0f

// This is the number of colors in Actor::diffuse.  Actor has multiple
// diffuse colors so that each edge can be a different color, and the actor
// is drawn with a gradient between them.
// I doubt I actually found all the places that touch diffuse and rely on the
// number of diffuse colors, so change this at your own risk. -Kyz
#define NUM_DIFFUSE_COLORS 4

/** @brief Base class for all objects that appear on the screen. */
class Actor : public MessageSubscriber
{
  public:
	/** @brief Set up the Actor with its initial settings. */
	Actor();
	/**
	 * @brief Copy a new Actor to the old one.
	 * @param cpy the new Actor to use in place of this one. */
	Actor(const Actor& cpy);
	~Actor() override;
	Actor& operator=(const Actor& x);
	[[nodiscard]] virtual auto Copy() const -> Actor*;
	virtual void InitState();
	virtual void LoadFromNode(const XNode* pNode);

	static void SetBGMTime(float fTime,
						   float fBeat,
						   float fTimeNoOffset,
						   float fBeatNoOffset);
	static void SetPlayerBGMBeat(float fBeat, float fBeatNoOffset);

	/**
	 * @brief The list of the different effects.
	 *
	 * todo: split out into diffuse effects and translation effects, or
	 * create an effect stack instead. -aj */
	enum Effect
	{
		no_effect,
		diffuse_blink,
		diffuse_shift,
		diffuse_ramp,
		glow_blink,
		glow_shift,
		glow_ramp,
		rainbow,
		wag,
		bounce,
		bob,
		pulse,
		spin,
		vibrate
	};

	/** @brief Various values an Actor's effect can be tied to. */
	enum EffectClock
	{
		CLOCK_TIMER,
		CLOCK_TIMER_GLOBAL,
		CLOCK_BGM_TIME,
		CLOCK_BGM_BEAT,
		CLOCK_BGM_TIME_NO_OFFSET,
		CLOCK_BGM_BEAT_NO_OFFSET,
		CLOCK_BGM_BEAT_PLAYER1,
		CLOCK_BGM_BEAT_PLAYER2,
		CLOCK_LIGHT_1 = 1000,
		CLOCK_LIGHT_LAST = 1100,
		NUM_CLOCKS
	};

	/**
	 * @brief The present state for the Tween.
	 */
	struct TweenState
	{
		void Init();
		static void MakeWeightedAverage(TweenState& average_out,
										const TweenState& ts1,
										const TweenState& ts2,
										float fPercentBetween);
		auto operator==(const TweenState& other) const -> bool;
		auto operator!=(const TweenState& other) const -> bool
		{
			return !operator==(other);
		}

		// start and end position for tweening
		RageVector3 pos;
		RageVector3 rotation;
		RageVector4 quat;
		RageVector3 scale;
		float fSkewX{}, fSkewY{};
		/**
		 * @brief The amount of cropping involved.
		 *
		 * If 0, there is no cropping. If 1, it's fully cropped. */
		RectF crop;
		/**
		 * @brief The amount of fading involved.
		 *
		 * If 0, there is no fade. If 1, it's fully faded. */
		RectF fade;
		/**
		 * @brief Four values making up the diffuse in this TweenState.
		 *
		 * 0 = UpperLeft, 1 = UpperRight, 2 = LowerLeft, 3 = LowerRight */
		RageColor diffuse[NUM_DIFFUSE_COLORS];
		/** @brief The glow color for this TweenState. */
		RageColor glow;
		/** @brief A magical value that nobody really knows the use for. ;) */
		float aux{};
	};

	// PartiallyOpaque broken out of Draw for reuse and clarity.
	[[nodiscard]] auto PartiallyOpaque() const -> bool;
	auto IsOver(float mx, float my) -> bool;

	auto GetFakeParentOrParent() -> Actor*; // fake parent > parent -mina
	auto GetTrueX() -> float; // recursive with parent (for mouseovers) -mina
	auto GetTrueY() -> float; // same
	auto GetTrueZ() -> float;
	auto GetTrueRotationX() -> float;
	auto GetTrueRotationY() -> float;
	auto GetTrueRotationZ() -> float; // same
	auto GetTrueZoom() -> float;	  // same
	auto GetTrueZoomX() -> float;
	auto GetTrueZoomY() -> float;
	auto GetTrueZoomZ() -> float;
	auto IsVisible() -> bool; // same but for gating updates on things that may
							  // not explicitly set visible = false -mina

	/**
	 * @brief Calls multiple functions for drawing the Actors.
	 *
	 * It calls the following in order:
	 * -# EarlyAbortDraw
	 * -# BeginDraw
	 * -# DrawPrimitives
	 * -# EndDraw
	 */
	void Draw();
	/**
	 * @brief Allow the Actor to be aborted early.
	 *
	 * Subclasses may wish to overwrite this to allow for
	 * aborted actors.
	 * @return false, as by default Actors shouldn't be aborted on drawing. */
	[[nodiscard]] virtual auto EarlyAbortDraw() const -> bool { return false; }
	/** @brief Calculate values that may be needed  for drawing. */
	virtual void PreDraw();
	/** @brief Reset internal diffuse and glow. */
	virtual void PostDraw();
	/** @brief Start the drawing and push the transform on the world matrix
	 * stack. */
	virtual void BeginDraw();
	/**
	 * @brief Set the global rendering states of this Actor.
	 *
	 * This should be called at the beginning of an Actor's DrawPrimitives()
	 * call. */
	virtual void SetGlobalRenderStates();
	/**
	 * @brief Set the texture rendering states of this Actor.
	 *
	 * This should be called after setting a texture for the Actor. */
	virtual void SetTextureRenderStates();
	/**
	 * @brief Draw the primitives of the Actor.
	 *
	 * Derivative classes should override this function. */
	virtual void DrawPrimitives(){};
	/** @brief Pop the transform from the world matrix stack. */
	virtual void EndDraw();

	// TODO(Sam): make Update non virtual and change all classes to override
	// UpdateInternal instead.
	virtual void Update(
	  float fDeltaTime); // this can short circuit UpdateInternal
	virtual void UpdateInternal(float fDeltaTime); // override this
	void UpdateTweening(float fDeltaTime);
	void CalcPercentThroughTween();
	// These next functions should all be overridden by a derived class that has
	// its own tweening states to handle.
	virtual void SetCurrentTweenStart() {}
	virtual void EraseHeadTween() {}
	virtual void UpdatePercentThroughTween(float PercentThroughTween) {}

	[[nodiscard]] auto get_tween_uses_effect_delta() const -> bool
	{
		return m_tween_uses_effect_delta;
	}
	void set_tween_uses_effect_delta(bool t) { m_tween_uses_effect_delta = t; }

	/**
	 * @brief Retrieve the Actor's name.
	 * @return the Actor's name. */
	[[nodiscard]] auto GetName() const -> const std::string& { return m_sName; }
	/**
	 * @brief Set the Actor's name to a new one.
	 * @param sName the new name for the Actor. */
	virtual void SetName(const std::string& sName) { m_sName = sName; }
	/**
	 * @brief Give this Actor a new parent.
	 * @param pParent the new parent Actor. */
	void SetParent(Actor* pParent);
	/**
	 * @brief Retrieve the Actor's parent.
	 * @return the Actor's parent. */
	[[nodiscard]] auto GetParent() const -> Actor* { return m_pParent; }
	/**
	 * @brief Retrieve the Actor's lineage.
	 * @return the Actor's lineage. */
	[[nodiscard]] auto GetLineage() const -> std::string;

	void SetFakeParent(Actor* mailman) { m_FakeParent = mailman; }
	[[nodiscard]] auto GetFakeParent() const -> Actor* { return m_FakeParent; }

	void AddWrapperState();
	void RemoveWrapperState(size_t i);
	auto GetWrapperState(size_t i) -> Actor*;
	[[nodiscard]] auto GetNumWrapperStates() const -> size_t
	{
		return m_WrapperStates.size();
	}

	/**
	 * @brief Retrieve the Actor's x position.
	 * @return the Actor's x position. */
	[[nodiscard]] auto GetX() const -> float { return m_current.pos.x; };
	/**
	 * @brief Retrieve the Actor's y position.
	 * @return the Actor's y position. */
	[[nodiscard]] auto GetY() const -> float { return m_current.pos.y; };
	/**
	 * @brief Retrieve the Actor's z position.
	 * @return the Actor's z position. */
	[[nodiscard]] auto GetZ() const -> float { return m_current.pos.z; };
	[[nodiscard]] auto GetDestX() const -> float
	{
		return DestTweenState().pos.x;
	};
	[[nodiscard]] auto GetDestY() const -> float
	{
		return DestTweenState().pos.y;
	};
	[[nodiscard]] auto GetDestZ() const -> float
	{
		return DestTweenState().pos.z;
	};
	void SetX(float x) { DestTweenState().pos.x = x; };
	void SetY(float y) { DestTweenState().pos.y = y; };
	void SetZ(float z) { DestTweenState().pos.z = z; };
	void SetXY(float x, float y)
	{
		DestTweenState().pos.x = x;
		DestTweenState().pos.y = y;
	};
	/**
	 * @brief Add to the x position of this Actor.
	 * @param x the amount to add to the Actor's x position. */
	void AddX(float x) { SetX(GetDestX() + x); }
	/**
	 * @brief Add to the y position of this Actor.
	 * @param y the amount to add to the Actor's y position. */
	void AddY(float y) { SetY(GetDestY() + y); }
	/**
	 * @brief Add to the z position of this Actor.
	 * @param z the amount to add to the Actor's z position. */
	void AddZ(float z) { SetZ(GetDestZ() + z); }

	// height and width vary depending on zoom
	[[nodiscard]] auto GetUnzoomedWidth() const -> float { return m_size.x; }
	[[nodiscard]] auto GetUnzoomedHeight() const -> float { return m_size.y; }

	[[nodiscard]] auto GetZoomedWidth() const -> float
	{
		return m_size.x * m_baseScale.x * DestTweenState().scale.x;
	}

	[[nodiscard]] auto GetZoomedHeight() const -> float
	{
		return m_size.y * m_baseScale.y * DestTweenState().scale.y;
	}
	void SetWidth(float width) { m_size.x = width; }
	void SetHeight(float height) { m_size.y = height; }

	// Base values
	[[nodiscard]] auto GetBaseZoomX() const -> float { return m_baseScale.x; }
	void SetBaseZoomX(float zoom) { m_baseScale.x = zoom; }
	[[nodiscard]] auto GetBaseZoomY() const -> float { return m_baseScale.y; }
	void SetBaseZoomY(float zoom) { m_baseScale.y = zoom; }
	[[nodiscard]] auto GetBaseZoomZ() const -> float { return m_baseScale.z; }
	void SetBaseZoomZ(float zoom) { m_baseScale.z = zoom; }
	void SetBaseZoom(float zoom)
	{
		m_baseScale = RageVector3(zoom, zoom, zoom);
	}
	void SetBaseRotationX(float rot) { m_baseRotation.x = rot; }
	void SetBaseRotationY(float rot) { m_baseRotation.y = rot; }
	void SetBaseRotationZ(float rot) { m_baseRotation.z = rot; }
	void SetBaseRotation(const RageVector3& rot) { m_baseRotation = rot; }
	virtual void SetBaseAlpha(float fAlpha) { m_fBaseAlpha = fAlpha; }
	void SetInternalDiffuse(const RageColor& c) { m_internalDiffuse = c; }
	void SetInternalGlow(const RageColor& c) { m_internalGlow = c; }

	/**
	 * @brief Retrieve the general zoom factor, using the x coordinate of the
	 * Actor.
	 *
	 * Note that this is not accurate in some cases.
	 * @return the zoom factor for the x coordinate of the Actor. */
	[[nodiscard]] auto GetZoom() const -> float
	{
		return DestTweenState().scale.x;
	}
	/**
	 * @brief Retrieve the zoom factor for the x coordinate of the Actor.
	 * @return the zoom factor for the x coordinate of the Actor. */
	[[nodiscard]] auto GetZoomX() const -> float
	{
		return DestTweenState().scale.x;
	}
	/**
	 * @brief Retrieve the zoom factor for the y coordinate of the Actor.
	 * @return the zoom factor for the y coordinate of the Actor. */
	[[nodiscard]] auto GetZoomY() const -> float
	{
		return DestTweenState().scale.y;
	}
	/**
	 * @brief Retrieve the zoom factor for the z coordinate of the Actor.
	 * @return the zoom factor for the z coordinate of the Actor. */
	[[nodiscard]] auto GetZoomZ() const -> float
	{
		return DestTweenState().scale.z;
	}
	/**
	 * @brief Set the zoom factor for all dimensions of the Actor.
	 * @param zoom the zoom factor for all dimensions. */
	void SetZoom(float zoom)
	{
		DestTweenState().scale.x = zoom;
		DestTweenState().scale.y = zoom;
		DestTweenState().scale.z = zoom;
	}
	/**
	 * @brief Set the zoom factor for the x dimension of the Actor.
	 * @param zoom the zoom factor for the x dimension. */
	void SetZoomX(float zoom) { DestTweenState().scale.x = zoom; }
	/**
	 * @brief Set the zoom factor for the y dimension of the Actor.
	 * @param zoom the zoom factor for the y dimension. */
	void SetZoomY(float zoom) { DestTweenState().scale.y = zoom; }
	/**
	 * @brief Set the zoom factor for the z dimension of the Actor.
	 * @param zoom the zoom factor for the z dimension. */
	void SetZoomZ(float zoom) { DestTweenState().scale.z = zoom; }
	void ZoomTo(float fX, float fY)
	{
		ZoomToWidth(fX);
		ZoomToHeight(fY);
	}
	void ZoomToWidth(float zoom) { SetZoomX(zoom / GetUnzoomedWidth()); }
	void ZoomToHeight(float zoom) { SetZoomY(zoom / GetUnzoomedHeight()); }

	[[nodiscard]] auto GetRotationX() const -> float
	{
		return DestTweenState().rotation.x;
	}
	[[nodiscard]] auto GetRotationY() const -> float
	{
		return DestTweenState().rotation.y;
	}
	[[nodiscard]] auto GetRotationZ() const -> float
	{
		return DestTweenState().rotation.z;
	}
	void SetRotationX(float rot) { DestTweenState().rotation.x = rot; }
	void SetRotationY(float rot) { DestTweenState().rotation.y = rot; }
	void SetRotationZ(float rot) { DestTweenState().rotation.z = rot; }
	// added in StepNXA, now available in sm-ssc:
	void AddRotationX(float rot) { DestTweenState().rotation.x += rot; };
	void AddRotationY(float rot) { DestTweenState().rotation.y += rot; };
	void AddRotationZ(float rot) { DestTweenState().rotation.z += rot; };
	// and these were normally in SM:
	void AddRotationH(float rot);
	void AddRotationP(float rot);
	void AddRotationR(float rot);

	void SetSkewX(float fAmount) { DestTweenState().fSkewX = fAmount; }

	[[nodiscard]] auto GetSkewX(float /* fAmount */) const -> float
	{
		return DestTweenState().fSkewX;
	}
	void SetSkewY(float fAmount) { DestTweenState().fSkewY = fAmount; }

	[[nodiscard]] auto GetSkewY(float /* fAmount */) const -> float
	{
		return DestTweenState().fSkewY;
	}

	[[nodiscard]] auto GetCropLeft() const -> float
	{
		return DestTweenState().crop.left;
	}
	[[nodiscard]] auto GetCropTop() const -> float
	{
		return DestTweenState().crop.top;
	}
	[[nodiscard]] auto GetCropRight() const -> float
	{
		return DestTweenState().crop.right;
	}
	[[nodiscard]] auto GetCropBottom() const -> float
	{
		return DestTweenState().crop.bottom;
	}
	void SetCropLeft(float percent) { DestTweenState().crop.left = percent; }
	void SetCropTop(float percent) { DestTweenState().crop.top = percent; }
	void SetCropRight(float percent) { DestTweenState().crop.right = percent; }
	void SetCropBottom(float percent)
	{
		DestTweenState().crop.bottom = percent;
	}

	void SetFadeLeft(float percent) { DestTweenState().fade.left = percent; }
	void SetFadeTop(float percent) { DestTweenState().fade.top = percent; }
	void SetFadeRight(float percent) { DestTweenState().fade.right = percent; }
	void SetFadeBottom(float percent)
	{
		DestTweenState().fade.bottom = percent;
	}

	void SetGlobalDiffuseColor(const RageColor& c);

	virtual void SetDiffuse(const RageColor& c)
	{
		for (auto& i : DestTweenState().diffuse) {
			i = c;
		}
	};
	virtual void SetDiffuseAlpha(float f)
	{
		for (auto i = 0; i < NUM_DIFFUSE_COLORS; ++i) {
			auto c = GetDiffuses(i);
			c.a = f;
			SetDiffuses(i, c);
		}
	}

	[[nodiscard]] auto GetCurrentDiffuseAlpha() const -> float
	{
		return m_current.diffuse[0].a;
	}
	void SetDiffuseColor(const RageColor& c);
	void SetDiffuses(int i, const RageColor& c)
	{
		DestTweenState().diffuse[i] = c;
	};
	void SetDiffuseUpperLeft(const RageColor& c)
	{
		DestTweenState().diffuse[0] = c;
	};
	void SetDiffuseUpperRight(const RageColor& c)
	{
		DestTweenState().diffuse[1] = c;
	};
	void SetDiffuseLowerLeft(const RageColor& c)
	{
		DestTweenState().diffuse[2] = c;
	};
	void SetDiffuseLowerRight(const RageColor& c)
	{
		DestTweenState().diffuse[3] = c;
	};
	void SetDiffuseTopEdge(const RageColor& c)
	{
		DestTweenState().diffuse[0] = DestTweenState().diffuse[1] = c;
	};
	void SetDiffuseRightEdge(const RageColor& c)
	{
		DestTweenState().diffuse[1] = DestTweenState().diffuse[3] = c;
	};
	void SetDiffuseBottomEdge(const RageColor& c)
	{
		DestTweenState().diffuse[2] = DestTweenState().diffuse[3] = c;
	};
	void SetDiffuseLeftEdge(const RageColor& c)
	{
		DestTweenState().diffuse[0] = DestTweenState().diffuse[2] = c;
	};
	[[nodiscard]] auto GetDiffuse() const -> RageColor
	{
		return DestTweenState().diffuse[0];
	};
	[[nodiscard]] auto GetDiffuses(int i) const -> RageColor
	{
		return DestTweenState().diffuse[i];
	};
	[[nodiscard]] auto GetDiffuseAlpha() const -> float
	{
		return DestTweenState().diffuse[0].a;
	};
	void SetGlow(const RageColor& c) { DestTweenState().glow = c; };
	[[nodiscard]] auto GetGlow() const -> RageColor
	{
		return DestTweenState().glow;
	};

	void SetAux(float f) { DestTweenState().aux = f; }
	[[nodiscard]] auto GetAux() const -> float { return m_current.aux; }

	virtual void BeginTweening(float time, ITween* pInterp);
	virtual void BeginTweening(float time, TweenType tt = TWEEN_LINEAR);
	virtual void StopTweening();
	void Sleep(float time);
	void QueueCommand(const std::string& sCommandName);
	void QueueMessage(const std::string& sMessageName);
	virtual void FinishTweening();
	virtual void HurryTweening(float factor);
	// Let ActorFrame and BGAnimation override
	[[nodiscard]] virtual auto GetTweenTimeLeft() const
	  -> float; // Amount of time until all tweens have stopped
	auto DestTweenState()
	  -> TweenState& // where Actor will end when its tween finish
	{
		if (m_Tweens.empty()) { // not tweening
			return m_current;
		}
		{
			return m_Tweens.back()->state;
		}
	}

	[[nodiscard]] auto DestTweenState() const -> const TweenState&
	{
		return const_cast<Actor*>(this)->DestTweenState();
	}

	/** @brief How do we handle stretching the Actor? */
	enum StretchType
	{
		fit_inside, /**< Have the Actor fit inside its parent, using the smaller
					   zoom. */
		cover /**< Have the Actor cover its parent, using the larger zoom. */
	};

	void ScaleToCover(const RectF& rect) { ScaleTo(rect, cover); }
	void ScaleToFitInside(const RectF& rect) { ScaleTo(rect, fit_inside); };
	void ScaleTo(const RectF& rect, StretchType st);

	void StretchTo(const RectF& rect);

	// Alignment settings.  These need to be virtual for BitmapText
	virtual void SetHorizAlign(float f) { m_fHorizAlign = f; }
	virtual void SetVertAlign(float f) { m_fVertAlign = f; }
	void SetHorizAlign(HorizAlign ha)
	{
		SetHorizAlign((ha == HorizAlign_Left)
						? 0.0F
						: (ha == HorizAlign_Center) ? 0.5F : +1.0F);
	}
	void SetVertAlign(VertAlign va)
	{
		SetVertAlign((va == VertAlign_Top)
					   ? 0.0F
					   : (va == VertAlign_Middle) ? 0.5F : +1.0F);
	}
	virtual auto GetHorizAlign() -> float { return m_fHorizAlign; }
	virtual auto GetVertAlign() -> float { return m_fVertAlign; }

	// effects
#if defined(SSC_FUTURES)
	void StopEffects();
	Effect GetEffect(int i) const { return m_Effects[i]; }
#else
	void StopEffect() { m_Effect = no_effect; }
	[[nodiscard]] auto GetEffect() const -> Effect { return m_Effect; }
#endif
	[[nodiscard]] auto GetSecsIntoEffect() const -> float
	{
		return m_fSecsIntoEffect;
	}
	[[nodiscard]] auto GetEffectDelta() const -> float
	{
		return m_fEffectDelta;
	}

	// todo: account for SSC_FUTURES by adding an effect as an arg to each one
	// -aj
	void SetEffectColor1(const RageColor& c) { m_effectColor1 = c; }
	void SetEffectColor2(const RageColor& c) { m_effectColor2 = c; }
	void RecalcEffectPeriod();
	void SetEffectPeriod(float fTime);
	[[nodiscard]] auto GetEffectPeriod() const -> float
	{
		return m_effect_period;
	}
	auto SetEffectTiming(float ramp_toh,
						 float at_half,
						 float ramp_tof,
						 float at_zero,
						 float at_full,
						 std::string& err) -> bool;
	auto SetEffectHoldAtFull(float haf, std::string& err) -> bool;
	void SetEffectOffset(float fTime) { m_fEffectOffset = fTime; }
	void SetEffectClock(EffectClock c) { m_EffectClock = c; }
	void SetEffectClockString(const std::string& s); // convenience

	void SetEffectMagnitude(const RageVector3& vec)
	{
		m_vEffectMagnitude = vec;
	}

	[[nodiscard]] auto GetEffectMagnitude() const -> RageVector3
	{
		return m_vEffectMagnitude;
	}

	void ResetEffectTimeIfDifferent(Effect new_effect);
	void SetEffectDiffuseBlink(float fEffectPeriodSeconds,
							   const RageColor& c1,
							   const RageColor& c2);
	void SetEffectDiffuseShift(float fEffectPeriodSeconds,
							   const RageColor& c1,
							   const RageColor& c2);
	void SetEffectDiffuseRamp(float fEffectPeriodSeconds,
							  const RageColor& c1,
							  const RageColor& c2);
	void SetEffectGlowBlink(float fEffectPeriodSeconds,
							const RageColor& c1,
							const RageColor& c2);
	void SetEffectGlowShift(float fEffectPeriodSeconds,
							const RageColor& c1,
							const RageColor& c2);
	void SetEffectGlowRamp(float fEffectPeriodSeconds,
						   const RageColor& c1,
						   const RageColor& c2);
	void SetEffectRainbow(float fEffectPeriodSeconds);
	void SetEffectWag(float fPeriod, const RageVector3& vect);
	void SetEffectBounce(float fPeriod, const RageVector3& vect);
	void SetEffectBob(float fPeriod, const RageVector3& vect);
	void SetEffectPulse(float fPeriod, float fMinZoom, float fMaxZoom);
	void SetEffectSpin(const RageVector3& vect);
	void SetEffectVibrate(const RageVector3& vect);

	// other properties
	/**
	 * @brief Determine if the Actor is visible at this time.
	 * @return true if it's visible, false otherwise. */
	[[nodiscard]] auto GetVisible() const -> bool { return m_bVisible; }
	void SetVisible(bool b) { m_bVisible = b; }
	void SetShadowLength(float fLength)
	{
		m_fShadowLengthX = fLength;
		m_fShadowLengthY = fLength;
	}
	void SetShadowLengthX(float fLengthX) { m_fShadowLengthX = fLengthX; }
	void SetShadowLengthY(float fLengthY) { m_fShadowLengthY = fLengthY; }
	void SetShadowColor(const RageColor& c) { m_ShadowColor = c; }
	// TODO(Sam): Implement hibernate as a tween type?
	void SetDrawOrder(int iOrder) { m_iDrawOrder = iOrder; }
	[[nodiscard]] auto GetDrawOrder() const -> int { return m_iDrawOrder; }

	virtual void EnableAnimation(bool b)
	{
		m_bIsAnimating = b;
	} // Sprite needs to overload this
	void StartAnimating() { this->EnableAnimation(true); }
	void StopAnimating() { this->EnableAnimation(false); }

	// render states
	void SetBlendMode(BlendMode mode) { m_BlendMode = mode; }
	void SetTextureTranslate(float x, float y)
	{
		m_texTranslate.x = x;
		m_texTranslate.y = y;
	}
	void SetTextureWrapping(bool b) { m_bTextureWrapping = b; }
	void SetTextureFiltering(bool b) { m_bTextureFiltering = b; }
	void SetClearZBuffer(bool b) { m_bClearZBuffer = b; }
	void SetUseZBuffer(bool b)
	{
		SetZTestMode(b ? ZTEST_WRITE_ON_PASS : ZTEST_OFF);
		SetZWrite(b);
	}
	virtual void SetZTestMode(ZTestMode mode) { m_ZTestMode = mode; }
	virtual void SetZWrite(bool b) { m_bZWrite = b; }
	void SetZBias(float f) { m_fZBias = f; }
	virtual void SetCullMode(CullMode mode) { m_CullMode = mode; }

	// Lua
	virtual void PushSelf(lua_State* L);
	virtual void PushContext(lua_State* L);

	// Named commands
	void AddCommand(const std::string& sCmdName,
					const apActorCommands& apac,
					bool warn = true);
	[[nodiscard]] auto HasCommand(const std::string& sCmdName) const -> bool;
	[[nodiscard]] auto GetCommand(const std::string& sCommandName) const
	  -> const apActorCommands*;
	void PlayCommand(const std::string& sCommandName)
	{
		HandleMessage(Message(sCommandName));
	} // convenience
	void PlayCommandNoRecurse(const Message& msg);

	// Commands by reference
	virtual void RunCommands(const LuaReference& cmds,
							 const LuaReference* pParamTable = nullptr);

	virtual void RunCommands(const apActorCommands& cmds,
							 const LuaReference* pParamTable = nullptr)
	{
		this->RunCommands(*cmds, pParamTable);
	} // convenience
	virtual void RunCommandsRecursively(
	  const LuaReference& cmds,
	  const LuaReference* pParamTable = nullptr)
	{
		RunCommands(cmds, pParamTable);
	}
	// If we're a leaf, then execute this command.
	virtual void RunCommandsOnLeaves(const LuaReference& cmds,
									 const LuaReference* pParamTable = nullptr)
	{
		RunCommands(cmds, pParamTable);
	}

	// Messages
	void HandleMessage(const Message& msg) override;

	// Animation
	[[nodiscard]] virtual auto GetNumStates() const -> int { return 1; }
	virtual void SetState(int /* iNewState */) {}
	[[nodiscard]] virtual auto GetAnimationLengthSeconds() const -> float
	{
		return 0;
	}
	virtual void SetSecondsIntoAnimation(float /*unused*/) {}
	virtual void SetUpdateRate(float /*unused*/) {}
	virtual auto GetUpdateRate() -> float { return 1.0F; }

	std::unique_ptr<LuaClass> m_pLuaInstance;

  protected:
	/** @brief the name of the Actor. */
	std::string m_sName;
	/** @brief the current parent of this Actor if it exists. */
	Actor* m_pParent;
	// m_FakeParent exists to provide a way to render the actor inside another's
	// state without making that actor the parent.  It's like having multiple
	// parents. -Kyz
	Actor* m_FakeParent;
	// WrapperStates provides a way to wrap the actor inside ActorFrames,
	// applicable to any actor, not just ones the theme creates.
	std::vector<Actor*> m_WrapperStates;

	/** @brief Some general information about the Tween. */
	struct TweenInfo
	{
		// counters for tweening
		TweenInfo();
		~TweenInfo();
		TweenInfo(const TweenInfo& cpy);
		auto operator=(const TweenInfo& rhs) -> TweenInfo&;

		ITween* m_pTween;
		/** @brief How far into the tween are we? */
		float m_fTimeLeftInTween{};
		/** @brief The number of seconds between Start and End positions/zooms.
		 */
		float m_fTweenTime{};
		/** @brief The command to execute when this TweenState goes into effect.
		 */
		std::string m_sCommandName;
	};

	RageVector3 m_baseRotation;
	RageVector3 m_baseScale;
	float m_fBaseAlpha{};
	RageColor m_internalDiffuse;
	RageColor m_internalGlow;

	RageVector2 m_size;
	TweenState m_current;
	TweenState m_start;
	struct TweenStateAndInfo
	{
		TweenState state;
		TweenInfo info;
	};
	std::vector<TweenStateAndInfo*> m_Tweens;

	/** @brief Temporary variables that are filled just before drawing */
	TweenState* m_pTempState{};

	// Stuff for alignment
	/** @brief The particular horizontal alignment.
	 *
	 * Use the defined constant values for best effect. */
	float m_fHorizAlign{};
	/** @brief The particular vertical alignment.
	 *
	 * Use the defined constant values for best effect. */
	float m_fVertAlign{};

	// Stuff for effects
#if defined(SSC_FUTURES) // be able to stack effects
	std::vector<Effect> m_Effects;
#else // compatibility
	Effect m_Effect;
#endif
	float m_fSecsIntoEffect{};
	float m_fEffectDelta{};

	// units depend on m_EffectClock
	float m_effect_ramp_to_half{};
	float m_effect_hold_at_half{};
	float m_effect_ramp_to_full{};
	float m_effect_hold_at_full{};
	float m_effect_hold_at_zero{};
	float m_fEffectOffset{};
	// Anything changing ramp_up, hold_at_half, ramp_down, or hold_at_zero must
	// also update the period so the period is only calculated when changed.
	// -Kyz
	float m_effect_period{};
	EffectClock m_EffectClock;
	bool m_tween_uses_effect_delta;

	/* This can be used in lieu of the fDeltaTime parameter to Update() to
	 * follow the effect clock.  Actor::Update must be called first. */
	[[nodiscard]] auto GetEffectDeltaTime() const -> float
	{
		return m_fEffectDelta;
	}

	// todo: account for SSC_FUTURES by having these be vectors too -aj
	RageColor m_effectColor1;
	RageColor m_effectColor2;
	RageVector3 m_vEffectMagnitude;

	// other properties
	bool m_bVisible{};
	bool m_bIsAnimating{};
	float m_fShadowLengthX{};
	float m_fShadowLengthY{};
	RageColor m_ShadowColor;
	/** @brief The draw order priority.
	 *
	 * The lower this number is, the sooner it is drawn. */
	int m_iDrawOrder{};

	// render states
	BlendMode m_BlendMode;
	ZTestMode m_ZTestMode;
	CullMode m_CullMode;
	RageVector2 m_texTranslate;
	bool m_bTextureWrapping{};
	bool m_bTextureFiltering{};
	bool m_bClearZBuffer{};
	bool m_bZWrite{};
	/**
	 * @brief The amount of bias.
	 *
	 * If 0, there is no bias. If 1, there is a full bias. */
	float m_fZBias{};

	// global state
	static float g_fCurrentBGMTime, g_fCurrentBGMBeat;
	static float g_fCurrentBGMTimeNoOffset, g_fCurrentBGMBeatNoOffset;
	static std::vector<float> g_vfCurrentBGMBeatPlayer;
	static std::vector<float> g_vfCurrentBGMBeatPlayerNoOffset;

  private:
	// commands
	std::map<std::string, apActorCommands> m_mapNameToCommands;
};

#endif
