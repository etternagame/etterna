#include "global.h"
#include "MeterDisplay.h"
#include "RageUtil.h"
#include "GameState.h"
#include "Song.h"
#include "ActorUtil.h"
#include "XmlFile.h"
#include "RageLog.h"
#include "LuaManager.h"

REGISTER_ACTOR_CLASS(MeterDisplay);
REGISTER_ACTOR_CLASS(SongMeterDisplay);

MeterDisplay::MeterDisplay()
= default;

void MeterDisplay::Load( const RString &sStreamPath, float fStreamWidth, const RString &sTipPath )
{
	m_sprStream.Load( sStreamPath );
	this->AddChild( m_sprStream );

	m_sprTip.Load( sTipPath );
	this->AddChild( m_sprTip );

	SetStreamWidth( fStreamWidth );
	SetPercent( 0.5f );
}

void MeterDisplay::LoadFromNode( const XNode* pNode )
{
	LOG->Trace( "MeterDisplay::LoadFromNode(%s)", ActorUtil::GetWhere(pNode).c_str() );

	const XNode *pStream = pNode->GetChild( "Stream" );
	if( pStream == NULL )
	{
		LuaHelpers::ReportScriptErrorFmt("%s: MeterDisplay: missing the \"Stream\" attribute", ActorUtil::GetWhere(pNode).c_str());
		return;
	}
	m_sprStream.LoadActorFromNode( pStream, this );
	m_sprStream->SetName( "Stream" );
	//LOAD_ALL_COMMANDS( m_sprStream );
	this->AddChild( m_sprStream );

	const XNode* pChild = pNode->GetChild( "Tip" );
	if( pChild != NULL )
	{
		m_sprTip.LoadActorFromNode( pChild, this );
		m_sprTip->SetName( "Tip" );
		//LOAD_ALL_COMMANDS( m_sprTip );
		this->AddChild( m_sprTip );
	}

	float fStreamWidth = 0;
	pNode->GetAttrValue( "StreamWidth", fStreamWidth );
	SetStreamWidth( fStreamWidth );

	SetPercent( 0.5f );

	ActorFrame::LoadFromNode( pNode );
}

void MeterDisplay::SetPercent( float fPercent )
{
	ASSERT( fPercent >= 0 && fPercent <= 1 );

	m_sprStream->SetCropRight( 1-fPercent );

	if( m_sprTip.IsLoaded() )
		m_sprTip->SetX( SCALE(fPercent, 0.f, 1.f, -m_fStreamWidth/2, m_fStreamWidth/2) );
}

void MeterDisplay::SetStreamWidth( float fStreamWidth )
{
	m_fStreamWidth = fStreamWidth;
	m_sprStream->SetZoomX( m_fStreamWidth / m_sprStream->GetUnzoomedWidth() );
}

void SongMeterDisplay::Update( float fDeltaTime )
{
	if( GAMESTATE->m_pCurSong )
	{
		float fSongStartSeconds = GAMESTATE->m_pCurSong->GetFirstSecond();
		float fSongEndSeconds = GAMESTATE->m_pCurSong->GetLastSecond();
		float fPercentPositionSong = SCALE( GAMESTATE->m_Position.m_fMusicSeconds, fSongStartSeconds, fSongEndSeconds, 0.0f, 1.0f );
		CLAMP( fPercentPositionSong, 0, 1 );

		SetPercent( fPercentPositionSong );
	}

	MeterDisplay::Update( fDeltaTime );
}

// lua start
#include "LuaBinding.h"

class LunaMeterDisplay: public Luna<MeterDisplay>
{
public:
	static int SetStreamWidth( T* p, lua_State *L )		{ p->SetStreamWidth(FArg(1)); COMMON_RETURN_SELF; }

	LunaMeterDisplay()
	{
		ADD_METHOD( SetStreamWidth );
	}
};

LUA_REGISTER_DERIVED_CLASS( MeterDisplay, ActorFrame )
// lua end

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
