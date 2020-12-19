#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "GraphDisplay.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Misc/RageMath.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Misc/StageStats.h"
#include "Etterna/Singletons/ThemeManager.h"

//#define DIVIDE_LINE_WIDTH
// THEME->GetMetricI(m_sName,"TexturedBottomHalf")
REGISTER_ACTOR_CLASS(GraphDisplay);

enum
{
	VALUE_RESOLUTION = 100
};

class GraphLine : public Actor
{
  public:
	enum
	{
		iSubdivisions = 4
	};
	enum
	{
		iCircleVertices = iSubdivisions + 2
	};
	void DrawPrimitives() override
	{
		Actor::SetGlobalRenderStates(); // set Actor-specified render states

		// Must call this after setting the texture or else texture
		// parameters have no effect.
		Actor::SetTextureRenderStates();

		for (unsigned i = 0; i < m_Quads.size(); ++i)
			m_Quads[i].c = this->m_pTempState->diffuse[0];
		for (unsigned i = 0; i < m_pCircles.size(); ++i)
			m_pCircles[i].c = this->m_pTempState->diffuse[0];

		DISPLAY->DrawQuads(&m_Quads[0], m_Quads.size());

		const int iFans = m_pCircles.size() / iCircleVertices;
		for (int i = 0; i < iFans; ++i)
			DISPLAY->DrawFan(&m_pCircles[0] + iCircleVertices * i,
							 iCircleVertices);
	}

	static void MakeCircle(const RageSpriteVertex& v,
						   RageSpriteVertex* pVerts,
						   int iSubdivisions,
						   float fRadius)
	{
		pVerts[0] = v;

		for (int i = 0; i < iSubdivisions + 1; ++i) {
			const float fRotation =
			  static_cast<float>(i) / iSubdivisions * 2 * PI;
			const float fX = RageFastCos(fRotation) * fRadius;
			const float fY = -RageFastSin(fRotation) * fRadius;
			pVerts[1 + i] = v;
			pVerts[1 + i].p.x += fX;
			pVerts[1 + i].p.y += fY;
		}
	}

	void Set(const RageSpriteVertex* m_LineStrip, int iSize)
	{
		m_pCircles.resize(iSize * iCircleVertices);

		for (int i = 0; i < iSize; ++i) {
			MakeCircle(m_LineStrip[i],
					   &m_pCircles[0] + iCircleVertices * i,
					   iSubdivisions,
					   1);
		}

		const int iNumLines = iSize - 1;
		m_Quads.resize(iNumLines * 4);
		for (int i = 0; i < iNumLines; ++i) {
			const RageSpriteVertex& p1 = m_LineStrip[i];
			const RageSpriteVertex& p2 = m_LineStrip[i + 1];

			const float opp = p2.p.x - p1.p.x;
			const float adj = p2.p.y - p1.p.y;
			const float hyp = powf(opp * opp + adj * adj, 0.5f);

			const float lsin = opp / hyp;
			const float lcos = adj / hyp;

			RageSpriteVertex* v = &m_Quads[i * 4];
			v[0] = v[1] = p1;
			v[2] = v[3] = p2;

			const int iLineWidth = 2;
			const float ydist = lsin * iLineWidth / 2;
			const float xdist = lcos * iLineWidth / 2;

			v[0].p.x += xdist;
			v[0].p.y -= ydist;
			v[1].p.x -= xdist;
			v[1].p.y += ydist;
			v[2].p.x -= xdist;
			v[2].p.y += ydist;
			v[3].p.x += xdist;
			v[3].p.y -= ydist;
		}
	}

	GraphLine* Copy() const override;

  private:
	vector<RageSpriteVertex> m_Quads;
	vector<RageSpriteVertex> m_pCircles;
};
REGISTER_ACTOR_CLASS(GraphLine);

class GraphBody : public Actor
{
  public:
	explicit GraphBody(const std::string& sFile)
	{
		m_pTexture = TEXTUREMAN->LoadTexture(sFile);

		for (int i = 0; i < 2 * VALUE_RESOLUTION; ++i) {
			m_Slices[i].c = RageColor(1, 1, 1, 1);
			m_Slices[i].t = RageVector2(0, 0);
		}
	}
	~GraphBody() override
	{
		TEXTUREMAN->UnloadTexture(m_pTexture);
		m_pTexture = nullptr;
	}

	void DrawPrimitives() override
	{
		Actor::SetGlobalRenderStates(); // set Actor-specified render states

		DISPLAY->SetTexture(TextureUnit_1, m_pTexture->GetTexHandle());

		// Must call this after setting the texture or else texture
		// parameters have no effect.
		Actor::SetTextureRenderStates();

		DISPLAY->SetTextureMode(TextureUnit_1, TextureMode_Modulate);
		DISPLAY->DrawQuadStrip(m_Slices, ARRAYLEN(m_Slices));
	}

	std::shared_ptr<RageTexture> m_pTexture;
	RageSpriteVertex m_Slices[2 * VALUE_RESOLUTION];
};

GraphDisplay::GraphDisplay()
{
	m_pGraphLine = nullptr;
	m_pGraphBody = nullptr;
}

GraphDisplay::~GraphDisplay()
{
	FOREACH(Actor*, m_vpSongBoundaries, p)
	SAFE_DELETE(*p);
	m_vpSongBoundaries.clear();
	SAFE_DELETE(m_pGraphLine);
	SAFE_DELETE(m_pGraphBody);
}

void
GraphDisplay::Set(const StageStats& ss, const PlayerStageStats& pss)
{
	/* Multiple song support
	// Show song boundaries
	float fSec = 0;
	FOREACH_CONST(Song*, ss.m_vpPossibleSongs, song)
	{
		if (song == ss.m_vpPossibleSongs.end() - 1)
			continue;
		fSec += (*song)->GetStepsSeconds();

		Actor* p = m_sprSongBoundary->Copy();
		m_vpSongBoundaries.push_back(p);
		const float fX = SCALE(fSec,
							   0,
							   fTotalStepSeconds,
							   m_quadVertices.left,
							   m_quadVertices.right);
		p->SetX(fX);
		this->AddChild(p);
	}
	*/
	SetWithoutStageStats(pss, ss.GetTotalPossibleStepsSeconds());
}

void
GraphDisplay::SetWithoutStageStats(const PlayerStageStats& pss, const float fTotalStepSeconds)
{
	m_Values.resize(VALUE_RESOLUTION);
	pss.GetWifeRecord(&m_Values[0], VALUE_RESOLUTION, fTotalStepSeconds);
	for (unsigned i = 0; i < ARRAYLEN(m_Values); i++)
		CLAMP(m_Values[i], 0.f, 1.f);

	UpdateVerts();

	if (!pss.m_bFailed) {
		// Search for the min life record to show "Just Barely!"
		float fMinLifeSoFar = 1.0f;
		int iMinLifeSoFarAt = 0;

		for (int i = 0; i < VALUE_RESOLUTION; ++i) {
			const float fLife = m_Values[i];
			if (fLife < fMinLifeSoFar) {
				fMinLifeSoFar = fLife;
				iMinLifeSoFarAt = i;
			}
		}

		if (fMinLifeSoFar > 0.0f && fMinLifeSoFar < 0.1f) {
			const float fX = SCALE(static_cast<float>(iMinLifeSoFarAt),
								   0.0f,
								   static_cast<float>(VALUE_RESOLUTION - 1),
								   m_quadVertices.left,
								   m_quadVertices.right);
			m_sprBarely->SetX(fX);
		} else {
			m_sprBarely->SetVisible(false);
		}
		this->AddChild(m_sprBarely);
	}
}

void
GraphDisplay::Load(const std::string& sMetricsGroup)
{
	m_size.x =
	  static_cast<float>(THEME->GetMetricI(sMetricsGroup, "BodyWidth"));
	m_size.y =
	  static_cast<float>(THEME->GetMetricI(sMetricsGroup, "BodyHeight"));

	m_sprBacking.Load(THEME->GetPathG(sMetricsGroup, "Backing"));
	m_sprBacking->ZoomToWidth(m_size.x);
	m_sprBacking->ZoomToHeight(m_size.y);
	this->AddChild(m_sprBacking);

	m_pGraphBody = new GraphBody(THEME->GetPathG(sMetricsGroup, "Body"));
	this->AddChild(m_pGraphBody);

	m_pGraphLine = new GraphLine;
	m_pGraphLine->SetName("Line");
	ActorUtil::LoadAllCommands(m_pGraphLine, sMetricsGroup);
	this->AddChild(m_pGraphLine);

	m_sprSongBoundary.Load(THEME->GetPathG(sMetricsGroup, "SongBoundary"));

	m_sprBarely.Load(THEME->GetPathG(sMetricsGroup, "Barely"));
}

void
GraphDisplay::UpdateVerts()
{
	m_quadVertices.left = -m_size.x / 2;
	m_quadVertices.right = m_size.x / 2;
	m_quadVertices.top = -m_size.y / 2;
	m_quadVertices.bottom = m_size.y / 2;

	RageSpriteVertex LineStrip[VALUE_RESOLUTION];
	for (int i = 0; i < VALUE_RESOLUTION; ++i) {
		const float fX = SCALE(static_cast<float>(i),
							   0.0f,
							   static_cast<float>(VALUE_RESOLUTION - 1),
							   m_quadVertices.left,
							   m_quadVertices.right);
		const float fY = SCALE(
		  m_Values[i], 0.0f, 1.0f, m_quadVertices.bottom, m_quadVertices.top);

		m_pGraphBody->m_Slices[i * 2 + 0].p = RageVector3(fX, fY, 0);
		m_pGraphBody->m_Slices[i * 2 + 1].p =
		  RageVector3(fX, m_quadVertices.bottom, 0);

		const RectF* pRect = m_pGraphBody->m_pTexture->GetTextureCoordRect(0);

		const float fU = SCALE(fX,
							   m_quadVertices.left,
							   m_quadVertices.right,
							   pRect->left,
							   pRect->right);
		const float fV = SCALE(fY,
							   m_quadVertices.top,
							   m_quadVertices.bottom,
							   pRect->top,
							   pRect->bottom);
		m_pGraphBody->m_Slices[i * 2 + 0].t = RageVector2(fU, fV);
		m_pGraphBody->m_Slices[i * 2 + 1].t = RageVector2(fU, pRect->bottom);

		LineStrip[i].p = RageVector3(fX, fY, 0);
		LineStrip[i].c = RageColor(1, 1, 1, 1);
		LineStrip[i].t = RageVector2(0, 0);
	}

	m_pGraphLine->Set(LineStrip, VALUE_RESOLUTION);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the GraphDisplay. */
class LunaGraphDisplay : public Luna<GraphDisplay>
{
  public:
	static int Load(T* p, lua_State* L)
	{
		p->Load(SArg(1));
		COMMON_RETURN_SELF;
	}
	static int Set(T* p, lua_State* L)
	{
		StageStats* pStageStats = Luna<StageStats>::check(L, 1);
		PlayerStageStats* pPlayerStageStats =
		  Luna<PlayerStageStats>::check(L, 2);
		if (pStageStats == nullptr) {
			luaL_error(L, "The StageStats passed to GraphDisplay:Set are nil.");
		}
		if (pPlayerStageStats == nullptr) {
			luaL_error(
			  L, "The PlayerStageStats passed to GraphDisplay:Set are nil.");
		}
		p->Set(*pStageStats, *pPlayerStageStats);
		COMMON_RETURN_SELF;
	}
	static int SetWithoutStageStats(T* p, lua_State* L)
	{
		auto* pss = Luna<PlayerStageStats>::check(L, 1);
		const float flen = FArg(2);
		p->SetWithoutStageStats(*pss, flen);
		COMMON_RETURN_SELF;
	}

	LunaGraphDisplay()
	{
		ADD_METHOD(Load);
		ADD_METHOD(Set);
		ADD_METHOD(SetWithoutStageStats);
	}
};

LUA_REGISTER_DERIVED_CLASS(GraphDisplay, ActorFrame)
// lua end
