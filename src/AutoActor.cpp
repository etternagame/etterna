#include "global.h"
#include "AutoActor.h"
#include "ThemeManager.h"
#include "Actor.h"
#include "ActorUtil.h"

void AutoActor::Unload()
{
	if(m_pActor != nullptr)
	{
		delete m_pActor;
	}
	m_pActor=nullptr;
}

AutoActor::AutoActor( const AutoActor &cpy )
{ 
	if( cpy.m_pActor == nullptr )
		m_pActor = nullptr;
	else
		m_pActor = cpy.m_pActor->Copy();
}

AutoActor &AutoActor::operator=( const AutoActor &cpy )
{
	Unload();

	if( cpy.m_pActor == nullptr )
		m_pActor = nullptr;
	else
		m_pActor = cpy.m_pActor->Copy();
	return *this;
}

void AutoActor::Load( Actor *pActor )
{
	Unload();
	m_pActor = pActor;
}

void AutoActor::Load( const RString &sPath )
{
	Unload();
	m_pActor = ActorUtil::MakeActor( sPath );

	// If a Condition is false, MakeActor will return NULL.
	if( m_pActor == nullptr )
		m_pActor = new Actor;
}

void AutoActor::LoadB( const RString &sMetricsGroup, const RString &sElement )
{
	ThemeManager::PathInfo pi;
	bool b = THEME->GetPathInfo( pi, EC_BGANIMATIONS, sMetricsGroup, sElement );
	ASSERT( b );
	LuaThreadVariable var1( "MatchingMetricsGroup", pi.sMatchingMetricsGroup );
	LuaThreadVariable var2( "MatchingElement", pi.sMatchingElement );
	Load( pi.sResolvedPath );
}

void AutoActor::LoadActorFromNode( const XNode* pNode, Actor *pParent )
{
	Unload();

	m_pActor = ActorUtil::LoadFromNode( pNode, pParent );
}

void AutoActor::LoadAndSetName( const RString &sScreenName, const RString &sActorName )
{
	Load( THEME->GetPathG(sScreenName,sActorName) );
	m_pActor->SetName( sActorName );
	ActorUtil::LoadAllCommands( *m_pActor, sScreenName );
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
