#include "Etterna/Globals/global.h"
#include "LuaExpressionTransform.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/Utils/RageUtil.h"

LuaExpressionTransform::LuaExpressionTransform()
{
	m_iNumSubdivisions = 1;
}

LuaExpressionTransform::~LuaExpressionTransform() = default;

void
LuaExpressionTransform::SetFromReference(const LuaReference& ref)
{
	m_exprTransformFunction = ref;
	if (ref.GetLuaType() != LUA_TFUNCTION) {
		LuaHelpers::ReportScriptError("Ignoring invalid transform function.");
		m_exprTransformFunction.SetFromNil();
	}
}

void
LuaExpressionTransform::TransformItemDirect(Actor& a,
											float fPositionOffsetFromCenter,
											int iItemIndex,
											int iNumItems) const
{
	if (m_exprTransformFunction.IsNil()) {
		return;
	}
	Lua* L = LUA->Get();
	m_exprTransformFunction.PushSelf(L);
	a.PushSelf(L);
	LuaHelpers::Push(L, fPositionOffsetFromCenter);
	LuaHelpers::Push(L, iItemIndex);
	LuaHelpers::Push(L, iNumItems);
	std::string error = "Lua error in Transform function: ";
	LuaHelpers::RunScriptOnStack(L, error, 4, 0, true);
	LUA->Release(L);
}

const Actor::TweenState&
LuaExpressionTransform::GetTransformCached(float fPositionOffsetFromCenter,
										   int iItemIndex,
										   int iNumItems) const
{
	PositionOffsetAndItemIndex key = { fPositionOffsetFromCenter, iItemIndex };

	auto iter = m_mapPositionToTweenStateCache.find(key);
	if (iter != m_mapPositionToTweenStateCache.end())
		return iter->second;

	Actor a;
	TransformItemDirect(a, fPositionOffsetFromCenter, iItemIndex, iNumItems);
	return m_mapPositionToTweenStateCache[key] = a.DestTweenState();
}

void
LuaExpressionTransform::TransformItemCached(Actor& a,
											float fPositionOffsetFromCenter,
											int iItemIndex,
											int iNumItems)
{
	float fInterval = 1.0f / m_iNumSubdivisions;
	float fFloor = QuantizeDown(fPositionOffsetFromCenter, fInterval);
	float fCeil = QuantizeUp(fPositionOffsetFromCenter, fInterval);

	if (fFloor == fCeil) {
		a.DestTweenState() = GetTransformCached(fCeil, iItemIndex, iNumItems);
	} else {
		const Actor::TweenState& tsFloor =
		  GetTransformCached(fFloor, iItemIndex, iNumItems);
		const Actor::TweenState& tsCeil =
		  GetTransformCached(fCeil, iItemIndex, iNumItems);

		float fPercentTowardCeil =
		  SCALE(fPositionOffsetFromCenter, fFloor, fCeil, 0.0f, 1.0f);
		Actor::TweenState::MakeWeightedAverage(
		  a.DestTweenState(), tsFloor, tsCeil, fPercentTowardCeil);
	}
}
