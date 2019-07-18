#include "Etterna/Globals/global.h"
#include "Foreach.h"
#include "LocalizedString.h"
#include "RageUtil/Utils/RageUtil.h"
#include "SubscriptionManager.h"

static SubscriptionManager<LocalizedString> m_Subscribers;

class LocalizedStringImplDefault : public ILocalizedStringImpl
{
  public:
	static ILocalizedStringImpl* Create()
	{
		return new LocalizedStringImplDefault;
	}

	void Load(const RString& sGroup, const RString& sName) override
	{
		m_sValue = sName;
	}

	const RString& GetLocalized() const override { return m_sValue; }

  private:
	RString m_sValue;
};

static LocalizedString::MakeLocalizer g_pMakeLocalizedStringImpl =
  LocalizedStringImplDefault::Create;

void
LocalizedString::RegisterLocalizer(MakeLocalizer pFunc)
{
	g_pMakeLocalizedStringImpl = pFunc;
	FOREACHS(LocalizedString*, *m_Subscribers.m_pSubscribers, l)
	{
		LocalizedString* pLoc = *l;
		pLoc->CreateImpl();
	}
}

LocalizedString::LocalizedString(const RString& sGroup, const RString& sName)
{
	m_Subscribers.Subscribe(this);

	m_sGroup = sGroup;
	m_sName = sName;
	m_pImpl = NULL;

	CreateImpl();
}

LocalizedString::LocalizedString(LocalizedString const& other)
{
	m_Subscribers.Subscribe(this);

	m_sGroup = other.m_sGroup;
	m_sName = other.m_sName;
	m_pImpl = NULL;

	CreateImpl();
}

LocalizedString::~LocalizedString()
{
	m_Subscribers.Unsubscribe(this);

	SAFE_DELETE(m_pImpl);
}

void
LocalizedString::CreateImpl()
{
	SAFE_DELETE(m_pImpl);
	m_pImpl = g_pMakeLocalizedStringImpl();
	m_pImpl->Load(m_sGroup, m_sName);
}

void
LocalizedString::Load(const RString& sGroup, const RString& sName)
{
	m_sGroup = sGroup;
	m_sName = sName;
	CreateImpl();
}

const RString&
LocalizedString::GetValue() const
{
	return m_pImpl->GetLocalized();
}
