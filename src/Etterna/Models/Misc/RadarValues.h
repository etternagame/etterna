#ifndef RARAR_VALUES_H
#define RARAR_VALUES_H

#include "GameConstantsAndTypes.h"
#include "ThemeMetric.h"

/** @brief Unknown radar values are given a default value. */
#define RADAR_VAL_UNKNOWN -1

class XNode;
struct lua_State;
/** @brief Cached song statistics. */
struct RadarValues
{
  private:
	int m_Values[NUM_RadarCategory];

  public:
	int operator[](RadarCategory cat) const { return m_Values[cat]; }
	int& operator[](RadarCategory cat) { return m_Values[cat]; }
	int operator[](int cat) const { return m_Values[cat]; }
	int& operator[](int cat) { return m_Values[cat]; }

	RadarValues();
	void MakeUnknown();
	void Zero();

	/**
	 * @brief Add one set of radar values to another.
	 * @param other The other set of radar values to add.
	 * @return the new set of radar values.
	 */
	RadarValues& operator+=(const RadarValues& other)
	{
		FOREACH_ENUM(RadarCategory, rc) { (*this)[rc] += other[rc]; }
		return *this;
	}
	/**
	 * @brief Determine if one set of radar values are equal to another.
	 * @param other The otehr set of radar values.
	 * @return true if the two sets are equal, false otherwise.
	 */
	bool operator==(const RadarValues& other) const
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
	bool operator!=(const RadarValues& other) const
	{
		return !operator==(other);
	}

	[[nodiscard]] XNode* CreateNode() const;
	void LoadFromNode(const XNode* pNode);

	[[nodiscard]] std::string ToString(int iMaxValues = -1) const; // default = all
	void FromString(const std::string& sValues);

	static ThemeMetric<bool> WRITE_SIMPLE_VALIES;
	static ThemeMetric<bool> WRITE_COMPLEX_VALIES;

	// Lua
	void PushSelf(lua_State* L);
};

#endif
