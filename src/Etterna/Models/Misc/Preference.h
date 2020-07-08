/* Preference - Holds user-chosen preferences that are saved between sessions.
 */

#ifndef PREFERENCE_H
#define PREFERENCE_H

#include "EnumHelper.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/Utils/RageUtil.h"
class XNode;

struct lua_State;
class IPreference
{
  public:
	IPreference(const std::string& sName);
	virtual ~IPreference();
	void ReadFrom(const XNode* pNode, bool bIsStatic);
	void WriteTo(XNode* pNode) const;
	void ReadDefaultFrom(const XNode* pNode);

	virtual void LoadDefault() = 0;
	virtual void SetDefaultFromString(const std::string& s) = 0;

	[[nodiscard]] virtual auto ToString() const -> std::string = 0;
	virtual void FromString(const std::string& s) = 0;

	virtual void SetFromStack(lua_State* L);
	virtual void PushValue(lua_State* L) const;

	[[nodiscard]] auto GetName() const -> const std::string& { return m_sName; }

	static auto GetPreferenceByName(const std::string& sName) -> IPreference*;
	static void LoadAllDefaults();
	static void ReadAllPrefsFromNode(const XNode* pNode, bool bIsStatic);
	static void SavePrefsToNode(XNode* pNode);
	static void ReadAllDefaultsFromNode(const XNode* pNode);

	auto GetName() -> std::string { return m_sName; }
	void SetStatic(bool b) { m_bIsStatic = b; }

  private:
	std::string m_sName;
	bool m_bIsStatic; // loaded from Static.ini?  If so, don't write to
					  // Preferences.ini
};

void
BroadcastPreferenceChanged(const std::string& sPreferenceName);

template<class T>
class Preference : public IPreference
{
  public:
	Preference(const std::string& sName,
			   const T& defaultValue,
			   void(pfnValidate)(T& val) = nullptr)
	  : IPreference(sName)
	  , m_currentValue(defaultValue)
	  , m_defaultValue(defaultValue)
	  , m_pfnValidate(pfnValidate)
	{
		LoadDefault();
	}

	[[nodiscard]] auto ToString() const -> std::string override
	{
		return StringConversion::ToString<T>(m_currentValue);
	}
	void FromString(const std::string& s) override
	{
		if (!StringConversion::FromString<T>(s, m_currentValue)) {
			m_currentValue = m_defaultValue;
		}
		if (m_pfnValidate) {
			m_pfnValidate(m_currentValue);
		}
	}
	void SetFromStack(lua_State* L) override
	{
		LuaHelpers::Pop<T>(L, m_currentValue);
		if (m_pfnValidate) {
			m_pfnValidate(m_currentValue);
		}
	}
	void PushValue(lua_State* L) const override
	{
		LuaHelpers::Push<T>(L, m_currentValue);
	}

	void LoadDefault() override { m_currentValue = m_defaultValue; }
	void SetDefaultFromString(const std::string& s) override
	{
		T def = m_defaultValue;
		if (!StringConversion::FromString<T>(s, m_defaultValue)) {
			m_defaultValue = def;
		}
	}

	[[nodiscard]] auto Get() const -> const T& { return m_currentValue; }

	[[nodiscard]] auto GetDefault() const -> const T& { return m_defaultValue; }

	operator const T() const { return Get(); }

	void Set(const T& other)
	{
		m_currentValue = other;
		BroadcastPreferenceChanged(GetName());
	}

	static auto GetPreferenceByName(const std::string& sName) -> Preference<T>*
	{
		auto pPreference = IPreference::GetPreferenceByName(sName);
		Preference<T>* pRet = dynamic_cast<Preference<T>*>(pPreference);
		return pRet;
	}

  private:
	T m_currentValue;
	T m_defaultValue;
	void (*m_pfnValidate)(T& val);
};

/** @brief Utilities for working with Lua. */
namespace LuaHelpers {
template<typename T>
void
Push(lua_State* L, const Preference<T>& Object)
{
	LuaHelpers::Push<T>(L, Object.Get());
}
}

template<class T>
class Preference1D
{
  public:
	using PreferenceT = Preference<T>;
	std::vector<PreferenceT*> m_v;

	Preference1D(void pfn(size_t i, std::string& sNameOut, T& defaultValueOut),
				 size_t N)
	{
		for (size_t i = 0; i < N; ++i) {
			std::string sName;
			T defaultValue;
			pfn(i, sName, defaultValue);
			m_v.push_back(new Preference<T>(sName, defaultValue));
		}
	}

	~Preference1D()
	{
		for (auto& v : m_v) {
			SAFE_DELETE(v);
		}
	}
	auto operator[](size_t i) const -> const Preference<T>& { return *m_v[i]; }
	auto operator[](size_t i) -> Preference<T>& { return *m_v[i]; }
};

#endif
