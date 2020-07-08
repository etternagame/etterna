#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/Actor.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "AutoActor.h"
#include "Etterna/Singletons/ThemeManager.h"

void
AutoActor::Unload()
{
	if (m_pActor != nullptr) {
		delete m_pActor;
	}
	m_pActor = nullptr;
}

AutoActor::AutoActor(const AutoActor& cpy)
{
	if (cpy.m_pActor == nullptr)
		m_pActor = nullptr;
	else
		m_pActor = cpy.m_pActor->Copy();
}

AutoActor&
AutoActor::operator=(const AutoActor& cpy)
{
	Unload();

	if (cpy.m_pActor == nullptr)
		m_pActor = nullptr;
	else
		m_pActor = cpy.m_pActor->Copy();
	return *this;
}

void
AutoActor::Load(Actor* pActor)
{
	Unload();
	m_pActor = pActor;
}

void
AutoActor::Load(const std::string& sPath)
{
	Unload();
	m_pActor = ActorUtil::MakeActor(sPath);

	// If a Condition is false, MakeActor will return NULL.
	if (m_pActor == nullptr)
		m_pActor = new Actor;
}

void
AutoActor::LoadB(const std::string& sMetricsGroup, const std::string& sElement)
{
	ThemeManager::PathInfo pi;
	const auto b =
	  THEME->GetPathInfo(pi, EC_BGANIMATIONS, sMetricsGroup, sElement);
	ASSERT(b);
	LuaThreadVariable var1("MatchingMetricsGroup", pi.sMatchingMetricsGroup);
	LuaThreadVariable var2("MatchingElement", pi.sMatchingElement);
	Load(pi.sResolvedPath);
}

void
AutoActor::LoadActorFromNode(const XNode* pNode, Actor* pParent)
{
	Unload();

	m_pActor = ActorUtil::LoadFromNode(pNode, pParent);
}

void
AutoActor::LoadAndSetName(const std::string& sScreenName,
						  const std::string& sActorName)
{
	Load(THEME->GetPathG(sScreenName, sActorName));
	m_pActor->SetName(sActorName);
	ActorUtil::LoadAllCommands(*m_pActor, sScreenName);
}
