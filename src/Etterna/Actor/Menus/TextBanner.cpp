#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "TextBanner.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/FileTypes/XmlFile.h"

REGISTER_ACTOR_CLASS(TextBanner);

void
TextBanner::LoadFromNode(const XNode* pNode)
{
	m_bInitted = true;

	ActorFrame::LoadFromNode(pNode);
}

void
TextBanner::Load(const std::string& sMetricsGroup)
{
	m_bInitted = true;

	m_textTitle.SetName("Title");
	m_textTitle.LoadFromFont(THEME->GetPathF(sMetricsGroup, "text"));
	this->AddChild(&m_textTitle);

	m_textSubTitle.SetName("Subtitle");
	m_textSubTitle.LoadFromFont(THEME->GetPathF(sMetricsGroup, "text"));
	this->AddChild(&m_textSubTitle);

	m_textArtist.SetName("Artist");
	m_textArtist.LoadFromFont(THEME->GetPathF(sMetricsGroup, "text"));
	this->AddChild(&m_textArtist);

	AddCommand("AfterSet", THEME->GetMetricA(sMetricsGroup, "AfterSetCommand"));
	m_sArtistPrependString =
	  THEME->GetMetric(sMetricsGroup, "ArtistPrependString");

	ActorUtil::LoadAllCommandsAndOnCommand(m_textTitle, sMetricsGroup);
	ActorUtil::LoadAllCommandsAndOnCommand(m_textSubTitle, sMetricsGroup);
	ActorUtil::LoadAllCommandsAndOnCommand(m_textArtist, sMetricsGroup);
}

TextBanner::TextBanner()
{
	m_bInitted = false;
}

TextBanner::TextBanner(const TextBanner& cpy)
  : ActorFrame(cpy)
  , m_bInitted(cpy.m_bInitted)
  , m_textTitle(cpy.m_textTitle)
  , m_textSubTitle(cpy.m_textSubTitle)
  , m_textArtist(cpy.m_textArtist)
  , m_sArtistPrependString(cpy.m_sArtistPrependString)
{
	this->AddChild(&m_textTitle);
	this->AddChild(&m_textSubTitle);
	this->AddChild(&m_textArtist);
}

void
TextBanner::SetFromString(const std::string& sDisplayTitle,
						  const std::string& sTranslitTitle,
						  const std::string& sDisplaySubTitle,
						  const std::string& sTranslitSubTitle,
						  const std::string& sDisplayArtist,
						  const std::string& sTranslitArtist)
{
	ASSERT(m_bInitted);

	m_textTitle.SetText(sDisplayTitle, sTranslitTitle);
	m_textSubTitle.SetText(sDisplaySubTitle, sTranslitSubTitle);
	m_textArtist.SetText(sDisplayArtist, sTranslitArtist);

	Message msg("AfterSet");
	this->PlayCommandNoRecurse(msg);
}

void
TextBanner::SetFromSong(const Song* pSong)
{
	if (pSong == nullptr) {
		SetFromString("", "", "", "", "", "");
		return;
	}
	SetFromString(pSong->GetDisplayMainTitle(),
				  pSong->GetTranslitMainTitle(),
				  pSong->GetDisplaySubTitle(),
				  pSong->GetTranslitSubTitle(),
				  m_sArtistPrependString + pSong->GetDisplayArtist(),
				  m_sArtistPrependString + pSong->GetTranslitArtist());
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the TextBanner. */
class LunaTextBanner : public Luna<TextBanner>
{
  public:
	static int Load(T* p, lua_State* L)
	{
		p->Load(SArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetFromSong(T* p, lua_State* L)
	{
		Song* pSong = Luna<Song>::check(L, 1);
		p->SetFromSong(pSong);
		COMMON_RETURN_SELF;
	}
	LunaTextBanner()
	{
		ADD_METHOD(Load);
		ADD_METHOD(SetFromSong);
	}
};

LUA_REGISTER_DERIVED_CLASS(TextBanner, ActorFrame)
// lua end
