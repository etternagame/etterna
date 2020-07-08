#include "Etterna/Globals/global.h"
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

	void Load(const std::string& sGroup, const std::string& sName) override
	{
		m_sValue = sName;
	}

	[[nodiscard]] const std::string& GetLocalized() const override
	{
		return m_sValue;
	}

  private:
	std::string m_sValue;
};

static LocalizedString::MakeLocalizer g_pMakeLocalizedStringImpl =
  LocalizedStringImplDefault::Create;

void
LocalizedString::RegisterLocalizer(MakeLocalizer pFunc)
{
	g_pMakeLocalizedStringImpl = pFunc;
	for (auto l : *m_Subscribers.m_pSubscribers) {
		auto pLoc = l;
		pLoc->CreateImpl();
	}
}

LocalizedString::LocalizedString(const std::string& sGroup,
								 const std::string& sName)
{
	m_Subscribers.Subscribe(this);

	m_sGroup = sGroup;
	m_sName = sName;
	m_pImpl = nullptr;

	CreateImpl();
}

LocalizedString::LocalizedString(LocalizedString const& other)
{
	m_Subscribers.Subscribe(this);

	m_sGroup = other.m_sGroup;
	m_sName = other.m_sName;
	m_pImpl = nullptr;

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
LocalizedString::Load(const std::string& sGroup, const std::string& sName)
{
	m_sGroup = sGroup;
	m_sName = sName;
	CreateImpl();
}

const std::string&
LocalizedString::GetValue() const
{
	return m_pImpl->GetLocalized();
}
