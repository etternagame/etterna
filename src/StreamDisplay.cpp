#include "global.h"
#include "StreamDisplay.h"
#include "GameState.h"
#include <cfloat>
#include "RageDisplay.h"
#include "ThemeManager.h"
#include "EnumHelper.h"

static const char *StreamTypeNames[] = {
	"Normal",
	"Passing",
	"Hot",
};
XToString( StreamType );

StreamDisplay::StreamDisplay()
{
	m_fPercent = 0;
	m_fTrailingPercent = 0;
	m_fVelocity = 0;
	m_fPassingAlpha = 0;
	m_fHotAlpha = 0;
	m_bAlwaysBounce = false;

	m_bDeleteChildren = true;
}

void StreamDisplay::Load( const RString & /* unreferenced: _sMetricsGroup  */)
{
	// XXX: actually load from the metrics group passed in -aj
	RString sMetricsGroup = "StreamDisplay";

	m_transformPill.SetFromReference( THEME->GetMetricR(sMetricsGroup,"PillTransformFunction") );
	VELOCITY_MULTIPLIER.Load(sMetricsGroup, "VelocityMultiplier");
	VELOCITY_MIN.Load(sMetricsGroup, "VelocityMin");
	VELOCITY_MAX.Load(sMetricsGroup, "VelocityMax");
	SPRING_MULTIPLIER.Load(sMetricsGroup, "SpringMultiplier");
	VISCOSITY_MULTIPLIER.Load(sMetricsGroup, "ViscosityMultiplier");

	float fTextureCoordScaleX = THEME->GetMetricF(sMetricsGroup,"TextureCoordScaleX");
	int iNumPills = static_cast<int>(THEME->GetMetricF(sMetricsGroup,"NumPills"));
	m_bAlwaysBounce = THEME->GetMetricB(sMetricsGroup,"AlwaysBounceNormalBar");

	FOREACH_ENUM( StreamType, st )
	{
		ASSERT( m_vpSprPill[st].empty() );

		for( int i=0; i<iNumPills; i++ )
		{
			auto *pSpr = new Sprite;

			pSpr->Load( THEME->GetPathG( sMetricsGroup, StreamTypeToString(st) ) );
			m_vpSprPill[st].push_back( pSpr );

			m_transformPill.TransformItemDirect( *pSpr, -1, i, iNumPills );
			float f = 1 / fTextureCoordScaleX;
			pSpr->SetCustomTextureRect( RectF(f*i,0,f*(i+1),1) );

			this->AddChild( pSpr );
		}
	}
}

void StreamDisplay::Update( float fDeltaSecs )
{
	// Sorry but the bar doesn't need to update 10 times per change. If you want physics go play farcry 3. -Mina.
	const float fDelta = m_fPercent - m_fTrailingPercent;

	if (fDelta==0) {
		m_fVelocity = 0;
		m_fTrailingPercent = 0;
		return;
	}

	ActorFrame::Update(fDeltaSecs);

	if (fabsf(fDelta) < 0.00001f)
		m_fVelocity = 0; // prevent div/0
	else
		m_fVelocity = (fDelta / fabsf(fDelta)) * VELOCITY_MULTIPLIER * 10;

	CLAMP(m_fVelocity, VELOCITY_MIN * 10, VELOCITY_MAX * 10);

	m_fTrailingPercent += m_fVelocity * fDeltaSecs;

	// set crop of pills
	const float fPillWidthPercent = 1.0f / m_vpSprPill[0].size();
	FOREACH_ENUM( StreamType, st )
	{
		for( int i=0; i<(int)m_vpSprPill[st].size(); i++ )
		{
			Sprite *pSpr = m_vpSprPill[st][i];
			pSpr->SetCropRight( 0.99f - m_fPercent);

			// Optimization: Don't draw pills that are covered up
			switch( st )
			{
			DEFAULT_FAIL( st );
			case StreamType_Normal:
				pSpr->SetVisible( m_fPassingAlpha < 1  ||  m_fHotAlpha < 1 );
				pSpr->SetDiffuseAlpha( 1 );
				break;
			case StreamType_Passing:
				pSpr->SetDiffuseAlpha( m_fPassingAlpha );
				pSpr->SetVisible( m_fHotAlpha < 1 );
				break;
			case StreamType_Hot:
				pSpr->SetDiffuseAlpha( m_fHotAlpha );
				break;
			}
		}
	}
}

void StreamDisplay::SetPercent( float fPercent )
{
#ifdef DEBUG
	float fLifeMultiplier = THEME->GetMetricF("LifeMeterBar","LifeMultiplier");
#endif
	DEBUG_ASSERT( fPercent >= 0.0f && fPercent <= 1.0f * fLifeMultiplier );
	if( isnan(fPercent) )
	{
		DEBUG_ASSERT_M( 0, "fPercent is NaN" );
		fPercent = 1;
	}
	if( !isfinite(fPercent) )
	{
		DEBUG_ASSERT_M( 0, "fPercent is infinite" );
		fPercent = 1;
	}

	float fDeltaPercent = fPercent - m_fPercent;
	m_fVelocity += fDeltaPercent;
	m_fPercent = fPercent;
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
