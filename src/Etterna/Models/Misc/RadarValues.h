#ifndef RARAR_VALUES_H
#define RARAR_VALUES_H

#include "GameConstantsAndTypes.h"
#include "ThemeMetric.h"

/** @brief Unknown radar values are given a default value. */
#define RADAR_VAL_UNKNOWN (-1)

class XNode;
struct lua_State;
/** @brief Cached song statistics. */
struct RadarValues
{
  private:
	int m_Values[NUM_RadarCategory];

  public:
	auto operator[](RadarCategory cat) const -> int { return m_Values[cat]; }
	auto operator[](RadarCategory cat) -> int& { return m_Values[cat]; }
	auto operator[](int cat) const -> int { return m_Values[cat]; }
	auto operator[](int cat) -> int& { return m_Values[cat]; }

	RadarValues();
	void MakeUnknown();
	void Zero();

	/**
	 * @brief Add one set of radar values to another.
	 * @param other The other set of radar values to add.
	 * @return the new set of radar values.
	 */
	auto operator+=(const RadarValues& other) -> RadarValues&
	{
		FOREACH_ENUM(RadarCategory, rc) { (*this)[rc] += other[rc]; }
		return *this;
	}
	/**
	 * @brief Determine if one set of radar values are equal to another.
	 * @param other The otehr set of radar values.
	 * @return true if the two sets are equal, false otherwise.
	 */
	auto operator==(const RadarValues& other) const -> bool
	{
		FOREACH_ENUM(RadarCategory, rc)
		{
			if ((*this)[rc] != other[rc]) {
				return false;
			}
		}
		return true;
	}
	/**
	 * @brief Determine if one set of radar values are not equal to another.
	 * @param other The otehr set of radar values.
	 * @return true if the two sets are not equal, false otherwise.
	 */
	auto operator!=(const RadarValues& other) const -> bool
	{
		return !operator==(other);
	}

	[[nodiscard]] auto CreateNode() const -> XNode*;
	void LoadFromNode(const XNode* pNode);

	[[nodiscard]] auto ToString(int iMaxValues = -1) const
	  -> std::string; // default = all
	void FromString(const std::string& sValues);

	static ThemeMetric<bool> WRITE_SIMPLE_VALIES;
	static ThemeMetric<bool> WRITE_COMPLEX_VALIES;

	// Lua
	void PushSelf(lua_State* L);
};

#endif
