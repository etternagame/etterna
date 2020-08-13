#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Singletons/GameManager.h"
#include "Style.h"
#include "StyleUtil.h"
#include "Etterna/FileTypes/XmlFile.h"

void
StyleID::FromStyle(const Style* p)
{
	if (p) {
		sGame = GAMEMAN->GetGameForStyle(p)->m_szName;
		sStyle = p->m_szName;
	} else {
		sGame = "";
		sStyle = "";
	}
}

const Style*
StyleID::ToStyle() const
{
	auto pGame = GAMEMAN->StringToGame(sGame);
	if (pGame == nullptr)
		return nullptr;

	return GAMEMAN->GameAndStringToStyle(pGame, sStyle);
}

XNode*
StyleID::CreateNode() const
{
	auto pNode = new XNode("Style");

	pNode->AppendAttr("Game", sGame);
	pNode->AppendAttr("Style", sStyle);

	return pNode;
}

void
StyleID::LoadFromNode(const XNode* pNode)
{
	Unset();
	ASSERT(pNode->GetName() == "Style");

	sGame = "";
	pNode->GetAttrValue("Game", sGame);

	sStyle = "";
	pNode->GetAttrValue("Style", sStyle);
}

bool
StyleID::IsValid() const
{
	return !sGame.empty() && !sStyle.empty();
}

bool
StyleID::operator<(const StyleID& rhs) const
{
#define COMP(a)                                                                \
	if ((a) < rhs.a)                                                           \
		return true;                                                           \
	if ((a) > rhs.a)                                                           \
		return false;
	COMP(sGame);
	COMP(sStyle);
#undef COMP
	return false;
}
