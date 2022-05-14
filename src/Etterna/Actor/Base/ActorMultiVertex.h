/** @brief ActorMultiVertex - An actor with mutiple vertices. Can be used to
 * create shapes that quads can't. */

#ifndef ACTOR_MULTI_VERTEX_H
#define ACTOR_MULTI_VERTEX_H

#include "Actor.h"
#include "Etterna/Models/Misc/CubicSpline.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Graphics/RageTextureID.h"

enum DrawMode
{
	DrawMode_Quads = 0,
	DrawMode_QuadStrip,
	DrawMode_Fan,
	DrawMode_Strip,
	DrawMode_Triangles,
	DrawMode_LineStrip,
	DrawMode_SymmetricQuadStrip,
	NUM_DrawMode,
	DrawMode_Invalid
};

const std::string&
DrawModeToString(DrawMode cat);
const std::string&
DrawModeToLocalizedString(DrawMode cat);
LuaDeclareType(DrawMode);

class RageTexture;

class ActorMultiVertex : public Actor
{
  public:
	static const size_t num_vert_splines = 4;
	ActorMultiVertex();
	ActorMultiVertex(const ActorMultiVertex& cpy);
	~ActorMultiVertex() override;

	void LoadFromNode(const XNode* Node) override;
	[[nodiscard]] ActorMultiVertex* Copy() const override;

	struct AMV_TweenState
	{
		AMV_TweenState() = default;
		static void MakeWeightedAverage(AMV_TweenState& average_out,
										const AMV_TweenState& ts1,
										const AMV_TweenState& ts2,
										float percent_between);
		bool operator==(const AMV_TweenState& other) const;
		bool operator!=(const AMV_TweenState& other) const
		{
			return !operator==(other);
		}

		void SetDrawState(DrawMode dm, int first, int num);
		[[nodiscard]] int GetSafeNumToDraw(DrawMode dm, int num) const;

		std::vector<RageSpriteVertex> vertices;
		std::vector<size_t> quad_states;

		DrawMode _DrawMode{ DrawMode_Invalid };
		int FirstToDraw{ 0 };
		int NumToDraw{ -1 };

		// needed for DrawMode_LineStrip
		float line_width{ 1.0f };
	};

	AMV_TweenState& AMV_DestTweenState()
	{
		if (AMV_Tweens.empty()) {
			return AMV_current;
		}

		return AMV_Tweens.back();
	}

	[[nodiscard]] const AMV_TweenState& AMV_DestTweenState() const
	{
		return const_cast<ActorMultiVertex*>(this)->AMV_DestTweenState();
	}

	void EnableAnimation(bool bEnable) override;
	void Update(float fDelta) override;
	[[nodiscard]] bool EarlyAbortDraw() const override;
	void DrawPrimitives() override;
	virtual void DrawInternal(const AMV_TweenState* TS);

	void SetCurrentTweenStart() override;
	void EraseHeadTween() override;
	void UpdatePercentThroughTween(float PercentThroughTween) override;
	void BeginTweening(float time, ITween* pInterp) override;

	void StopTweening() override;
	void FinishTweening() override;

	void SetTexture(RageTexture* Texture);
	[[nodiscard]] RageTexture* GetTexture() const { return _Texture; };
	void LoadFromTexture(const RageTextureID& ID);

	void UnloadTexture();
	void SetNumVertices(size_t n);

	void AddVertex();
	void AddVertices(int Add);

	void SetEffectMode(EffectMode em) { _EffectMode = em; }
	void SetTextureMode(TextureMode tm) { _TextureMode = tm; }
	void SetLineWidth(float width) { AMV_DestTweenState().line_width = width; }

	void SetDrawState(DrawMode dm, int first, int num)
	{
		AMV_DestTweenState().SetDrawState(dm, first, num);
	}

	[[nodiscard]] DrawMode GetDestDrawMode() const
	{
		return AMV_DestTweenState()._DrawMode;
	}
	[[nodiscard]] int GetDestFirstToDraw() const
	{
		return AMV_DestTweenState().FirstToDraw;
	}
	[[nodiscard]] int GetDestNumToDraw() const
	{
		return AMV_DestTweenState().NumToDraw;
	}
	[[nodiscard]] DrawMode GetCurrDrawMode() const
	{
		return AMV_current._DrawMode;
	}
	[[nodiscard]] int GetCurrFirstToDraw() const
	{
		return AMV_current.FirstToDraw;
	}
	[[nodiscard]] int GetCurrNumToDraw() const { return AMV_current.NumToDraw; }
	size_t GetNumVertices() { return AMV_DestTweenState().vertices.size(); }

	void SetVertexPos(int index, float x, float y, float z);
	void SetVertexColor(int index, const RageColor& c);
	void SetVertexCoords(int index, float TexCoordX, float TexCoordY);

	inline void SetVertsFromSplinesInternal(size_t num_splines,
											size_t start_vert);
	void SetVertsFromSplines();
	CubicSplineN* GetSpline(size_t i);

	struct State
	{
		RectF rect;
		float delay;
	};

	[[nodiscard]] int GetNumStates() const override { return _states.size(); }
	void AddState(const State& new_state) { _states.push_back(new_state); }
	void RemoveState(size_t i)
	{
		ASSERT(i < _states.size());
		_states.erase(_states.begin() + i);
	}

	[[nodiscard]] size_t GetState() const { return _cur_state; }
	State& GetStateData(size_t i)
	{
		ASSERT(i < _states.size());
		return _states[i];
	}
	void SetStateData(size_t i, const State& s)
	{
		ASSERT(i < _states.size());
		_states[i] = s;
	}
	void SetStateProperties(const std::vector<State>& new_states)
	{
		_states = new_states;
		SetState(0);
	}
	void SetState(int i) override;
	void SetAllStateDelays(float delay);
	[[nodiscard]] float GetAnimationLengthSeconds() const override;
	void SetSecondsIntoAnimation(float seconds) override;
	void UpdateAnimationState(bool force_update = false);

	[[nodiscard]] size_t GetNumQuadStates() const
	{
		return AMV_DestTweenState().quad_states.size();
	}
	void AddQuadState(size_t s)
	{
		AMV_DestTweenState().quad_states.push_back(s);
	}
	void RemoveQuadState(size_t i)
	{
		AMV_DestTweenState().quad_states.erase(
		  AMV_DestTweenState().quad_states.begin() + i);
	}
	size_t GetQuadState(size_t i)
	{
		return AMV_DestTweenState().quad_states[i];
	}
	void SetQuadState(size_t i, size_t s)
	{
		AMV_DestTweenState().quad_states[i] = s;
	}
	bool _use_animation_state;
	bool _decode_movie;

	void PushSelf(lua_State* L) override;

  private:
	RageTexture* _Texture;

	std::vector<RageSpriteVertex> _Vertices;
	std::vector<AMV_TweenState> AMV_Tweens;
	AMV_TweenState AMV_current;
	AMV_TweenState AMV_start;

	// required to handle diffuse and glow
	AMV_TweenState* AMV_TempState;

	EffectMode _EffectMode;
	TextureMode _TextureMode;

	// Four splines for controlling vert positions, because quads drawmode
	// requires four. -Kyz
	std::vector<CubicSplineN> _splines;

	bool _skip_next_update;
	float _secs_into_state;
	size_t _cur_state;
	std::vector<State> _states;
};

#endif // ACTOR_MULTI_VERTEX_H
