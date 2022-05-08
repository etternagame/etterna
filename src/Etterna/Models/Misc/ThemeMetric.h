/* ThemeMetric - Theme specific data. */

#ifndef THEME_METRIC_H
#define THEME_METRIC_H

#include "Etterna/Singletons/ThemeManager.h"

#include <map>

/** @brief The general interface for reading ThemeMetrics. */
class IThemeMetric
{
  public:
	virtual ~IThemeMetric() = default;
	virtual void Read() = 0;
	virtual void Clear() = 0;
};

template<class T>
struct ThemeMetricTypeTraits
{
	enum
	{
		Callable = 1
	};
};

/* LuaReference and apActorCommands return the function directly without calling
 * it. */
template<>
struct ThemeMetricTypeTraits<LuaReference>
{
	enum
	{
		Callable = 0
	};
};
template<>
struct ThemeMetricTypeTraits<apActorCommands>
{
	enum
	{
		Callable = 0
	};
};

/** @brief The theme specific data.
 *
 * Each piece of data is to correspond to a type. */
template<class T>
class ThemeMetric : public IThemeMetric
{
  protected:
	/** @brief the metric's group.
	 *
	 * In metrics.ini, it is usually done as such: [GroupName] */
	std::string m_sGroup;
	/** @brief the metric's name. */
	std::string m_sName;
	/** @brief the metric's value. */
	LuaReference m_Value;
	mutable T m_currentValue;
	bool m_bCallEachTime{ false };

  public:
	/* Initializing with no group and name is allowed; if you do this, you must
	 * call Load() to set them.  This is done to allow initializing cached
	 * metrics in one place for classes that don't receive their m_sName in the
	 * constructor (everything except screens). */
	ThemeMetric(std::string sGroup = "", std::string sName = "")
	  : m_sGroup(std::move(sGroup))
	  , m_sName(std::move(sName))
	  , m_currentValue(T())
	{
		ThemeManager::Subscribe(this);
	}

	ThemeMetric(const ThemeMetric<T>& cpy)
	  : IThemeMetric(cpy)
	  , m_sGroup(cpy.m_sGroup)
	  , m_sName(cpy.m_sName)
	  , m_Value(cpy.m_Value)
	// do we transfer the current value or bCallEachTime?
	{
		m_currentValue = T();
		m_bCallEachTime = false;
		ThemeManager::Subscribe(this);
	}

	~ThemeMetric() override { ThemeManager::Unsubscribe(this); }
	/**
	 * @brief Load the chosen metric from the .ini file.
	 * @param sGroup the group the metric is in.
	 * @param sName the name of the metric. */
	void Load(const std::string& sGroup, const std::string& sName)
	{
		m_sGroup = sGroup;
		m_sName = sName;
		Read();
	}

	void ChangeGroup(const std::string& sGroup)
	{
		m_sGroup = sGroup;
		Read();
	}
	/**
	 * @brief Actually read the metric and get its data. */
	void Read() override
	{
		if (!m_sName.empty() && (THEME != nullptr) && THEME->IsThemeLoaded()) {
			auto L = LUA->Get();
			THEME->GetMetric(m_sGroup, m_sName, m_Value);
			m_Value.PushSelf(L);
			LuaHelpers::FromStack(L, m_currentValue, -1);
			lua_pop(L, 1);
			LUA->Release(L);

			/* If the value is a function, evaluate it every time. */
			m_bCallEachTime = ThemeMetricTypeTraits<T>::Callable &&
							  m_Value.GetLuaType() == LUA_TFUNCTION;
		} else {
			m_Value.Unset();
			m_bCallEachTime = false;
		}
	}

	void PushSelf(Lua* L)
	{
		ASSERT(m_Value.IsSet());
		m_Value.PushSelf(L);
	}

	void Clear() override { m_Value.Unset(); }

	/**
	 * @brief Retrieve the metric's name.
	 * @return the metric's name. */
	auto GetName() const -> const std::string& { return m_sName; }
	/**
	 * @brief Retrieve the metric's group.
	 * @return the metric's group. */
	auto GetGroup() const -> const std::string& { return m_sGroup; }

	/**
	 * @brief Retrieve the metric's value.
	 * @return the metric's value. */
	auto GetValue() const -> const T&
	{
		ASSERT(!m_sName.empty());
		ASSERT_M(m_Value.IsSet(), m_sGroup + " " + m_sName);

		if (m_bCallEachTime) {
			auto L = LUA->Get();

			// call function with 0 arguments and 1 result
			m_Value.PushSelf(L);
			auto error = m_sGroup + ": " + m_sName + ": ";
			LuaHelpers::RunScriptOnStack(L, error, 0, 1, true);
			if (!lua_isnil(L, -1)) {
				LuaHelpers::Pop(L, m_currentValue);
			} else {
				lua_pop(L, 1);
			}
			LUA->Release(L);
		}

		return m_currentValue;
	}

	operator const T&() const { return GetValue(); }

	auto IsLoaded() const -> bool { return m_Value.IsSet(); }

	// Hacks for VC6 for all boolean operators.
	// These three no longer appear to be required:
	// bool operator ! () const { return !GetValue(); }
	// bool operator && ( const T& input ) const { return GetValue() && input; }
	// bool operator || ( const T& input ) const { return GetValue() || input; }

	// This one is still required in at least Visual Studio 2008:
	auto operator==(const T& input) const -> bool
	{
		return GetValue() == input;
	}
};

using MetricName1D = std::string (*)(size_t);

template<class T>
class ThemeMetric1D : public IThemeMetric
{
	using ThemeMetricT = ThemeMetric<T>;
	std::vector<ThemeMetricT> m_metric;

  public:
	ThemeMetric1D(const std::string& sGroup, MetricName1D pfn, size_t N)
	{
		Load(sGroup, pfn, N);
	}
	ThemeMetric1D() { Load(std::string(), nullptr, 0); }
	void Load(const std::string& sGroup, MetricName1D pfn, size_t N)
	{
		m_metric.resize(N);
		for (unsigned i = 0; i < N; i++) {
			m_metric[i].Load(sGroup, pfn(i));
		}
	}
	void Read() override
	{
		for (unsigned i = 0; i < m_metric.size(); i++) {
			m_metric[i].Read();
		}
	}
	void Clear() override
	{
		for (unsigned i = 0; i < m_metric.size(); i++) {
			m_metric[i].Clear();
		}
	}

	[[nodiscard]] auto GetValue(size_t i) const -> const T&
	{
		return m_metric[i].GetValue();
	}
};

using MetricName2D = std::string (*)(size_t, size_t);

template<class T>
class ThemeMetric2D : public IThemeMetric
{
	using ThemeMetricT = ThemeMetric<T>;
	using ThemeMetricTVector = std::vector<ThemeMetricT>;
	std::vector<ThemeMetricTVector> m_metric;

  public:
	ThemeMetric2D(const std::string& sGroup = "",
				  MetricName2D pfn = nullptr,
				  size_t N = 0,
				  size_t M = 0)
	{
		Load(sGroup, pfn, N, M);
	}
	void Load(const std::string& sGroup, MetricName2D pfn, size_t N, size_t M)
	{
		m_metric.resize(N);
		for (unsigned i = 0; i < N; i++) {
			m_metric[i].resize(M);
			for (unsigned j = 0; j < M; j++)
				m_metric[i][j].Load(sGroup, pfn(i, j));
		}
	}
	void Read() override
	{
		for (unsigned i = 0; i < m_metric.size(); i++)
			for (unsigned j = 0; j < m_metric[i].size(); j++)
				m_metric[i][j].Read();
	}
	void Clear() override
	{
		for (unsigned i = 0; i < m_metric.size(); i++)
			for (unsigned j = 0; j < m_metric[i].size(); j++)
				m_metric[i][j].Clear();
	}
	auto GetValue(size_t i, size_t j) const -> const T&
	{
		return m_metric[i][j].GetValue();
	}
};

using MetricNameMap = std::string (*)(std::string);

template<class T>
class ThemeMetricMap : public IThemeMetric
{
	using ThemeMetricT = ThemeMetric<T>;
	std::map<std::string, ThemeMetricT> m_metric;

  public:
	ThemeMetricMap(
	  const std::string& sGroup = "",
	  MetricNameMap pfn = nullptr,
	  const std::vector<std::string>& vsValueNames = std::vector<std::string>())
	{
		Load(sGroup, pfn, vsValueNames);
	}
	void Load(const std::string& sGroup,
			  MetricNameMap pfn,
			  const std::vector<std::string>& vsValueNames)
	{
		m_metric.clear();
		for (auto& s : vsValueNames) {
			m_metric[s].Load(sGroup, pfn(s));
		}
	}
	void Read() override
	{
		// HACK: GCC (3.4) takes this and pretty much nothing else.
		// I don't know why.
		for (typename std::map<std::string, ThemeMetric<T>>::iterator m =
			   m_metric.begin();
			 m != m_metric.end();
			 ++m) {
			m->second.Read();
		}
	}
	void Clear() override
	{
		for (typename std::map<std::string, ThemeMetric<T>>::iterator m =
			   m_metric.begin();
			 m != m_metric.end();
			 ++m) {
			m->second.Clear();
		}
	}
	[[nodiscard]] auto GetValue(const std::string& s) const -> const T&
	{
		// HACK: GCC (3.4) takes this and pretty much nothing else.
		// I don't know why.
		typename std::map<std::string, ThemeMetric<T>>::const_iterator iter =
		  m_metric.find(s);
		ASSERT(iter != m_metric.end());
		return iter->second.GetValue();
	}
};

#endif
