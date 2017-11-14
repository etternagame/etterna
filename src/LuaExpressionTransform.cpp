#include "global.h"
#include "LuaExpressionTransform.h"
#include "LuaManager.h"
#include "RageUtil.h"

LuaExpressionTransform::LuaExpressionTransform()
{
	m_iNumSubdivisions = 1;
}

LuaExpressionTransform::~LuaExpressionTransform()
= default;

void LuaExpressionTransform::SetFromReference( const LuaReference &ref )
{
	m_exprTransformFunction = ref;
	if(ref.GetLuaType() != LUA_TFUNCTION)
	{
		LuaHelpers::ReportScriptError("Ignoring invalid transform function.");
		m_exprTransformFunction.SetFromNil();
	}
}

void LuaExpressionTransform::TransformItemDirect( Actor &a, float fPositionOffsetFromCenter, int iItemIndex, int iNumItems ) const
{
	if(m_exprTransformFunction.IsNil())
	{
		return;
	}
	Lua *L = LUA->Get();
	m_exprTransformFunction.PushSelf( L );
	a.PushSelf( L );
	LuaHelpers::Push( L, fPositionOffsetFromCenter );
	LuaHelpers::Push( L, iItemIndex );
	LuaHelpers::Push( L, iNumItems );
	std::string error= "Lua error in Transform function: ";
	LuaHelpers::RunScriptOnStack(L, error, 4, 0, true);
	LUA->Release(L);
}

const Actor::TweenState &LuaExpressionTransform::GetTransformCached( float fPositionOffsetFromCenter, int iItemIndex, int iNumItems ) const
{
	PositionOffsetAndItemIndex key = { fPositionOffsetFromCenter, iItemIndex };

	auto iter = m_mapPositionToTweenStateCache.find( key );
	if( iter != m_mapPositionToTweenStateCache.end() )
		return iter->second;

	Actor a;
	TransformItemDirect( a, fPositionOffsetFromCenter, iItemIndex, iNumItems );
	return m_mapPositionToTweenStateCache[key] = a.DestTweenState();
}

void LuaExpressionTransform::TransformItemCached( Actor &a, float fPositionOffsetFromCenter, int iItemIndex, int iNumItems )
{
	float fInterval = 1.0f / m_iNumSubdivisions;
	float fFloor = QuantizeDown( fPositionOffsetFromCenter, fInterval );
	float fCeil = QuantizeUp( fPositionOffsetFromCenter, fInterval );

	if( fFloor == fCeil )
	{
		a.DestTweenState() = GetTransformCached( fCeil, iItemIndex, iNumItems );
	}
	else
	{
		const Actor::TweenState &tsFloor = GetTransformCached( fFloor, iItemIndex, iNumItems );
		const Actor::TweenState &tsCeil = GetTransformCached( fCeil, iItemIndex, iNumItems );

		float fPercentTowardCeil = SCALE( fPositionOffsetFromCenter, fFloor, fCeil, 0.0f, 1.0f );
		Actor::TweenState::MakeWeightedAverage( a.DestTweenState(), tsFloor, tsCeil, fPercentTowardCeil );
	}
}


/*
 * (c) 2003-2004 Chris Danford
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
