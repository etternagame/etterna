#include "Etterna/Globals/global.h"
#include "Foreach.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Singletons/MessageManager.h"
#include "Preference.h"
#include "RageUtil/Misc/RageLog.h"
#include "SubscriptionManager.h"
#include "Etterna/FileTypes/XmlFile.h"

static SubscriptionManager<IPreference> m_Subscribers;

IPreference::IPreference(const RString& sName)
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
IPreference::GetPreferenceByName(const RString& sName)
{
	FOREACHS(IPreference*, *m_Subscribers.m_pSubscribers, p)
	{
		if (!(*p)->GetName().CompareNoCase(sName))
			return *p;
	}

	return NULL;
}

void
IPreference::LoadAllDefaults()
{
	FOREACHS_CONST(IPreference*, *m_Subscribers.m_pSubscribers, p)
	(*p)->LoadDefault();
}

void
IPreference::ReadAllPrefsFromNode(const XNode* pNode, bool bIsStatic)
{
	ASSERT(pNode != NULL);
	FOREACHS_CONST(IPreference*, *m_Subscribers.m_pSubscribers, p)
	(*p)->ReadFrom(pNode, bIsStatic);
}

void
IPreference::SavePrefsToNode(XNode* pNode)
{
	FOREACHS_CONST(IPreference*, *m_Subscribers.m_pSubscribers, p)
	(*p)->WriteTo(pNode);
}

void
IPreference::ReadAllDefaultsFromNode(const XNode* pNode)
{
	if (pNode == NULL)
		return;
	FOREACHS_CONST(IPreference*, *m_Subscribers.m_pSubscribers, p)
	(*p)->ReadDefaultFrom(pNode);
}

void
IPreference::PushValue(lua_State* L) const
{
	if (LOG)
		LOG->Trace(
		  "The preference value \"%s\" is of a type not supported by Lua",
		  m_sName.c_str());

	lua_pushnil(L);
}

void
IPreference::SetFromStack(lua_State* L)
{
	if (LOG)
		LOG->Trace(
		  "The preference value \"%s\" is of a type not supported by Lua",
		  m_sName.c_str());

	lua_pop(L, 1);
}

void
IPreference::ReadFrom(const XNode* pNode, bool bIsStatic)
{
	RString sVal;
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
	RString sVal;
	if (!pNode->GetAttrValue(m_sName, sVal))
		return;
	SetDefaultFromString(sVal);
}

void
BroadcastPreferenceChanged(const RString& sPreferenceName)
{
	if (MESSAGEMAN)
		MESSAGEMAN->Broadcast(sPreferenceName + "Changed");
}
