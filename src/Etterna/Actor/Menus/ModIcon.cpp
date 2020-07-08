#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "ModIcon.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ThemeManager.h"

ModIcon::ModIcon() = default;

ModIcon::ModIcon(const ModIcon& cpy)
  : ActorFrame(cpy)
  , m_text(cpy.m_text)
  , m_sprFilled(cpy.m_sprFilled)
  , m_sprEmpty(cpy.m_sprEmpty)
  , CROP_TEXT_TO_WIDTH(cpy.CROP_TEXT_TO_WIDTH)
  , STOP_WORDS(cpy.STOP_WORDS)
  , m_vStopWords(cpy.m_vStopWords)
{
	this->RemoveAllChildren();
	this->AddChild(m_sprFilled);
	this->AddChild(m_sprEmpty);
	this->AddChild(&m_text);
}

void
ModIcon::Load(const std::string& sMetricsGroup)
{
	m_sprFilled.Load(THEME->GetPathG(sMetricsGroup, "Filled"));
	m_sprFilled->SetName("Filled");
	ActorUtil::LoadAllCommands(m_sprFilled, sMetricsGroup);
	this->AddChild(m_sprFilled);

	m_sprEmpty.Load(THEME->GetPathG(sMetricsGroup, "Empty"));
	m_sprEmpty->SetName("Empty");
	ActorUtil::LoadAllCommands(m_sprEmpty, sMetricsGroup);
	this->AddChild(m_sprEmpty);

	m_text.LoadFromFont(THEME->GetPathF(sMetricsGroup, "Text"));
	m_text.SetName("Text");
	ActorUtil::LoadAllCommandsAndSetXYAndOnCommand(m_text, sMetricsGroup);
	this->AddChild(&m_text);

	CROP_TEXT_TO_WIDTH.Load(sMetricsGroup, "CropTextToWidth");

	// stop words
	STOP_WORDS.Load(sMetricsGroup, "StopWords");
	split(STOP_WORDS, ",", m_vStopWords);

	Set("");
}

void
ModIcon::Set(const std::string& _sText)
{
	auto sText = _sText;

	for (auto& m_vStopWord : m_vStopWords)
		if (EqualsNoCase(sText, m_vStopWord))
			sText = "";

	s_replace(sText, (" "), "\n");

	const auto bVacant = (sText.empty());
	m_sprFilled->SetVisible(!bVacant);
	m_sprEmpty->SetVisible(bVacant);

	m_text.SetText(sText);
	m_text.CropToWidth(CROP_TEXT_TO_WIDTH);
}
