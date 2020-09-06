#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Singletons/MessageManager.h"
#include "Preference.h"
#include "Core/Services/Locator.hpp"
#include "SubscriptionManager.h"
#include "Etterna/FileTypes/XmlFile.h"

static SubscriptionManager<IPreference> m_Subscribers;

IPreference::IPreference(const std::string& sName)
  : m_sName(sName)
  , m_bIsStatic(false)
{
	m_Subscribers.Subscribe(this);
}

IPreference::~IPreference()
{
	m_Subscribers.Unsubscribe(this);
}

IPreference*
IPreference::GetPreferenceByName(const std::string& sName)
{
	for (auto& p : *m_Subscribers.m_pSubscribers) {
		if (!CompareNoCase(p->GetName(), sName))
			return p;
	}

	return nullptr;
}

void
IPreference::LoadAllDefaults()
{
	for (auto& p : *m_Subscribers.m_pSubscribers) {
		p->LoadDefault();
	}
}

void
IPreference::ReadAllPrefsFromNode(const XNode* pNode, bool bIsStatic)
{
	ASSERT(pNode != NULL);
	for (auto& p : *m_Subscribers.m_pSubscribers) {
		p->ReadFrom(pNode, bIsStatic);
	}
}

void
IPreference::SavePrefsToNode(XNode* pNode)
{
	for (auto& p : *m_Subscribers.m_pSubscribers) {
		p->WriteTo(pNode);
	}
}

void
IPreference::ReadAllDefaultsFromNode(const XNode* pNode)
{
	if (pNode == nullptr)
		return;
	for (auto& p : *m_Subscribers.m_pSubscribers) {
		p->ReadDefaultFrom(pNode);
	}
}

void
IPreference::PushValue(lua_State* L) const
{
	Locator::getLogger()->trace("The preference value \"{}\" is of a type not supported by Lua", m_sName.c_str());

	lua_pushnil(L);
}

void
IPreference::SetFromStack(lua_State* L)
{
    Locator::getLogger()->trace("The preference value \"{}\" is of a type not supported by Lua",
		  m_sName.c_str());

	lua_pop(L, 1);
}

void
IPreference::ReadFrom(const XNode* pNode, bool bIsStatic)
{
	std::string sVal;
	if (pNode->GetAttrValue(m_sName, sVal)) {
		FromString(sVal);
		m_bIsStatic = bIsStatic;
	}
}

void
IPreference::WriteTo(XNode* pNode) const
{
	if (!m_bIsStatic)
		pNode->AppendAttr(m_sName, ToString());
}

/* Load our value from the node, and make it the new default. */
void
IPreference::ReadDefaultFrom(const XNode* pNode)
{
	std::string sVal;
	if (!pNode->GetAttrValue(m_sName, sVal))
		return;
	SetDefaultFromString(sVal);
}

void
BroadcastPreferenceChanged(const std::string& sPreferenceName)
{
	if (MESSAGEMAN)
		MESSAGEMAN->Broadcast(sPreferenceName + "Changed");
}
