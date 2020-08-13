#include "Etterna/Globals/global.h"
#include "RadarValues.h"
#include "Etterna/FileTypes/XmlFile.h"

#include <algorithm>

RadarValues::RadarValues()
{
	MakeUnknown();
}

void
RadarValues::MakeUnknown()
{
	FOREACH_ENUM(RadarCategory, rc)
	(*this)[rc] = RADAR_VAL_UNKNOWN;
}

void
RadarValues::Zero()
{
	FOREACH_ENUM(RadarCategory, rc)
	(*this)[rc] = 0;
}

XNode*
RadarValues::CreateNode() const
{
	XNode* pNode = new XNode("RadarValues");

	FOREACH_ENUM(RadarCategory, rc)
	pNode->AppendChild(RadarCategoryToString(rc), (*this)[rc]);
	return pNode;
}

void
RadarValues::LoadFromNode(const XNode* pNode)
{
	ASSERT(pNode->GetName() == "RadarValues");

	Zero();
	FOREACH_ENUM(RadarCategory, rc)
	pNode->GetChildValue(RadarCategoryToString(rc), (*this)[rc]);
}

/* iMaxValues is only used for writing compatibility fields in non-cache
 * SM files; they're never actually read. */
std::string
RadarValues::ToString(int iMaxValues) const
{
	if (iMaxValues == -1)
		iMaxValues = NUM_RadarCategory;
	iMaxValues = std::min(iMaxValues, static_cast<int>(NUM_RadarCategory));

	vector<std::string> asRadarValues;
	asRadarValues.reserve(iMaxValues);
	for (int r = 0; r < iMaxValues; r++) {
		asRadarValues.push_back(IntToString((*this)[r]));
	}

	return join(",", asRadarValues);
}

void
RadarValues::FromString(const std::string& sRadarValues)
{
	vector<std::string> saValues;
	split(sRadarValues, ",", saValues, true);

	if (saValues.size() != NUM_RadarCategory) {
		MakeUnknown();
		return;
	}

	FOREACH_ENUM(RadarCategory, rc) { (*this)[rc] = StringToInt(saValues[rc]); }
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the RadarValues. */
class LunaRadarValues : public Luna<RadarValues>
{
  public:
	static int GetValue(T* p, lua_State* L)
	{
		lua_pushnumber(L, (*p)[Enum::Check<RadarCategory>(L, 1)]);
		return 1;
	}

	LunaRadarValues() { ADD_METHOD(GetValue); }
};

LUA_REGISTER_CLASS(RadarValues)
// lua end
