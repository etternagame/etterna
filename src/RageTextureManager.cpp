/*
 * Texture garbage collection policies:
 *
 * Default: When DelayedDelete is off, delete unused textures immediately.
 *          When on, only delete textures when we change themes, on DoDelayedDelete().
 *
 * Volatile: Delete unused textures once they've been used at least once.  Ignore
 *           DelayedDelete.
 *
 *           This is for banners.  We don't want to load all low-quality banners in
 *           memory at once, since it might be ten megs of textures, and we don't
 *           need to, since we can reload them very quickly.  We don't want to keep
 *           high quality textures in memory, either, although it's unlikely that a
 *           player could actually view all banners long enough to transition to them
 *           all in the course of one song select screen.
 *
 * If a texture is loaded as DEFAULT that was already loaded as VOLATILE, DEFAULT
 * overrides.
 */
	
#include "global.h"
#include "RageTextureManager.h"
#include "RageBitmapTexture.h"
#include "arch/MovieTexture/MovieTexture.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageDisplay.h"
#include "Foreach.h"
#include "ActorUtil.h"
#include "Screen.h"
#include "ScreenManager.h"

#include <map>

RageTextureManager*		TEXTUREMAN		= NULL; // global and accessible from anywhere in our program

namespace
{
	map<RageTextureID, RageTexture*> m_mapPathToTexture;
	map<RageTextureID, RageTexture*> m_textures_to_update;
	map<RageTexture*, RageTextureID> m_texture_ids_by_pointer;
};

RageTextureManager::RageTextureManager()
	{}

RageTextureManager::~RageTextureManager()
{
	FOREACHM( RageTextureID, RageTexture*, m_mapPathToTexture, i )
	{
		RageTexture* pTexture = i->second;
		if( pTexture->m_iRefCount )
			LOG->Trace( "TEXTUREMAN LEAK: '%s', RefCount = %d.", i->first.filename.c_str(), pTexture->m_iRefCount );
		SAFE_DELETE( pTexture );
	}
	m_textures_to_update.clear();
	m_texture_ids_by_pointer.clear();
}

void RageTextureManager::Update( float fDeltaTime )
{
	static RageTimer garbageCollector;
	if (garbageCollector.PeekDeltaTime() >= 30.0f)
	{
		if (SCREENMAN && SCREENMAN->GetTopScreen() &&
			SCREENMAN->GetTopScreen()->GetScreenType() != gameplay)
		{
			DoDelayedDelete();
			garbageCollector.Touch();
		}
	}	

	FOREACHM(RageTextureID, RageTexture*, m_textures_to_update, i)
	{
		RageTexture* pTexture = i->second;
		pTexture->Update( fDeltaTime );
	}
}

void RageTextureManager::AdjustTextureID( RageTextureID &ID ) const
{
	if( ID.iColorDepth == -1 )
		ID.iColorDepth = m_Prefs.m_iTextureColorDepth;
	ID.iMaxSize = min( ID.iMaxSize, m_Prefs.m_iMaxTextureResolution );
	if( m_Prefs.m_bMipMaps )
		ID.bMipMaps = true;
}

bool RageTextureManager::IsTextureRegistered( RageTextureID ID ) const
{
	AdjustTextureID(ID);
	return m_mapPathToTexture.find(ID) != m_mapPathToTexture.end();
}

/* If you've set up a texture yourself, register it here so it can be referenced
 * and deleted by ID.  This takes ownership; the texture will be freed according to
 * its GC policy. */
void RageTextureManager::RegisterTexture( RageTextureID ID, RageTexture *pTexture )
{
	AdjustTextureID(ID);

	/* Make sure we don't already have a texture with this ID.  If we do, the
	 * caller should have used it. */
	std::map<RageTextureID, RageTexture*>::iterator p = m_mapPathToTexture.find(ID);
	if( p != m_mapPathToTexture.end() )
	{
		/* Oops, found the texture. */
		RageException::Throw( "Custom texture \"%s\" already registered!", ID.filename.c_str() );
	}

	m_mapPathToTexture[ID] = pTexture;
	m_texture_ids_by_pointer[pTexture]= ID;
}

void RageTextureManager::RegisterTextureForUpdating(const RageTextureID &id, RageTexture* tex)
{
	m_textures_to_update[id]= tex;
}

static const RString g_sDefaultTextureName = "__blank__";
RageTextureID RageTextureManager::GetDefaultTextureID()
{
	return RageTextureID( g_sDefaultTextureName );
}

static const RString g_ScreenTextureName = "__screen__";
RageTextureID RageTextureManager::GetScreenTextureID()
{
	return RageTextureID(g_ScreenTextureName);
}

RageSurface* RageTextureManager::GetScreenSurface()
{
	return DISPLAY->CreateScreenshot();
}

class RageTexture_Default: public RageTexture
{
public:
	RageTexture_Default():
		RageTexture( RageTextureID() ) 
	{
		m_iSourceWidth = m_iSourceHeight = 1;
		m_iTextureWidth = m_iTextureHeight = 1;
		m_iImageWidth = m_iImageHeight = 1;
		CreateFrameRects();
	}
	unsigned GetTexHandle() const override { return m_uTexHandle; }

private:
	unsigned m_uTexHandle{0};
};

// Load and unload textures from disk.
RageTexture* RageTextureManager::LoadTextureInternal( RageTextureID ID )
{
	CHECKPOINT_M( ssprintf( "RageTextureManager::LoadTexture(%s).", ID.filename.c_str() ) );

	AdjustTextureID(ID);

	/* We could have two copies of the same bitmap if there are equivalent but
	 * different paths, e.g. "Bitmaps\me.bmp" and "..\Rage PC Edition\Bitmaps\me.bmp". */
	std::map<RageTextureID, RageTexture*>::iterator p = m_mapPathToTexture.find(ID);
	if( p != m_mapPathToTexture.end() )
	{
		/* Found the texture.  Just increase the refcount and return it. */
		RageTexture* pTexture = p->second;
		pTexture->m_iRefCount++;
		return pTexture;
	}

	// The texture is not already loaded.  Load it.

	RageTexture* pTexture;
	if( ID.filename == g_sDefaultTextureName )
	{
		pTexture = new RageTexture_Default;
	}
	else if(ActorUtil::GetFileType(ID.filename) == FT_Movie)
	{
		pTexture = RageMovieTexture::Create( ID );
	}
	else
	{
		pTexture = new RageBitmapTexture( ID );
	}

	m_mapPathToTexture[ID] = pTexture;
	m_texture_ids_by_pointer[pTexture]= ID;

	return pTexture;
}

/* Load a normal texture.  Use this call to actually use a texture. */
RageTexture* RageTextureManager::LoadTexture( const RageTextureID &ID )
{
	RageTexture* pTexture = LoadTextureInternal( ID );
	if ( pTexture )
	{
		pTexture->m_lastRefTime.Touch();
		pTexture->m_bWasUsed = true;
	}
		
	return pTexture;
}

RageTexture* RageTextureManager::CopyTexture( RageTexture *pCopy )
{
	++pCopy->m_iRefCount;
	return pCopy;
}

void RageTextureManager::VolatileTexture( const RageTextureID &ID )
{
	RageTexture* pTexture = LoadTextureInternal( ID );
	pTexture->GetPolicy() = min( pTexture->GetPolicy(), RageTextureID::TEX_VOLATILE );
	UnloadTexture( pTexture );
}

void RageTextureManager::UnloadTexture( RageTexture *t )
{
	if( t == NULL )
		return;

	t->m_iRefCount--;
	ASSERT_M( t->m_iRefCount >= 0, ssprintf("%i, %s", t->m_iRefCount, t->GetID().filename.c_str()) );

	if( t->m_iRefCount )
		return; /* Can't unload textures that are still referenced. */

	bool bDeleteThis = false;

	/* Always unload movies, so we don't waste time decoding. */
	if( t->IsAMovie() )
		bDeleteThis = true;

	/* Delete normal textures immediately unless m_bDelayedDelete is is on. */
	if( t->GetPolicy() == RageTextureID::TEX_DEFAULT && (!m_Prefs.m_bDelayedDelete || t->m_lastRefTime.PeekDeltaTime() >= 30.0f) )
		bDeleteThis = true;

	/* Delete volatile textures after they've been used at least once. */
	if( t->GetPolicy() == RageTextureID::TEX_VOLATILE && t->m_bWasUsed )
		bDeleteThis = true;
	
	if( bDeleteThis )
		DeleteTexture( t );
}

void RageTextureManager::DeleteTexture( RageTexture *t )
{
	ASSERT( t->m_iRefCount == 0 );
	//LOG->Trace( "RageTextureManager: deleting '%s'.", t->GetID().filename.c_str() );

	map<RageTexture*, RageTextureID>::iterator id_entry=
		m_texture_ids_by_pointer.find(t);
	if(id_entry != m_texture_ids_by_pointer.end())
	{
		map<RageTextureID, RageTexture*>::iterator tex_entry=
			m_mapPathToTexture.find(id_entry->second);
		if(tex_entry != m_mapPathToTexture.end())
		{
			m_mapPathToTexture.erase(tex_entry);
			SAFE_DELETE(t);
		}
		map<RageTextureID, RageTexture*>::iterator tex_update_entry=
			m_textures_to_update.find(id_entry->second);
		if(tex_update_entry != m_textures_to_update.end())
		{
			m_textures_to_update.erase(tex_update_entry);
		}
		m_texture_ids_by_pointer.erase(id_entry);
		return;
	}
	else
	{
		FAIL_M("Tried to delete a texture that wasn't in the ids by pointer list.");
		FOREACHM( RageTextureID, RageTexture*, m_mapPathToTexture, i )
		{
			if( i->second == t )
			{
				m_mapPathToTexture.erase( i );	// remove map entry
				SAFE_DELETE( t );	// free the texture
				map<RageTextureID, RageTexture*>::iterator tex_update_entry=
					m_textures_to_update.find(i->first);
				if(tex_update_entry != m_textures_to_update.end())
				{
					m_textures_to_update.erase(tex_update_entry);
				}
				return;
			}
		}
	}

	FAIL_M("Tried to delete a texture that wasn't loaded");
}

void RageTextureManager::GarbageCollect( GCType type )
{
	// Search for old textures with refcount==0 to unload
	LOG->Trace("Performing texture garbage collection.");

	for( std::map<RageTextureID, RageTexture*>::iterator i = m_mapPathToTexture.begin();
		i != m_mapPathToTexture.end(); )
	{
		std::map<RageTextureID, RageTexture*>::iterator j = i;
		i++;

		RString sPath = j->first.filename;
		RageTexture* t = j->second;

		if( t->m_iRefCount )
			continue; /* Can't unload textures that are still referenced. */

		bool bDeleteThis = false;
		if( type==screen_changed )
		{
			RageTextureID::TexPolicy policy = t->GetPolicy();
			switch( policy )
			{
			case RageTextureID::TEX_DEFAULT: 
				/* If m_bDelayedDelete, wait until delayed_delete.  If !m_bDelayedDelete,
				 * it should have been deleted when it reached no references, but we
				 * might have just changed the preference. */
				if( !m_Prefs.m_bDelayedDelete )
					bDeleteThis = true;
				break;
			case RageTextureID::TEX_VOLATILE:
				bDeleteThis = true;
				break;
			default:
				FAIL_M(ssprintf("Invalid texture policy: %i", policy));
			}
		}

		/* This happens when we change themes; free all textures. */
		if( type==delayed_delete )
			bDeleteThis = true;
			
		if( bDeleteThis )
			DeleteTexture( t );
	}
}


void RageTextureManager::ReloadAll()
{
	DisableOddDimensionWarning();

	/* Let's get rid of all unreferenced textures, so we don't reload a
	 * ton of cached data that we're not necessarily going to use. */
	DoDelayedDelete();

	FOREACHM( RageTextureID, RageTexture*, m_mapPathToTexture, i )
	{
		i->second->Reload();
	}

	EnableOddDimensionWarning();
}

/* In some cases, changing the display mode will reset the rendering context,
 * releasing all textures.  We don't want to reload immediately if that happens,
 * since we might be changing texture preferences too, which also may have to reload
 * textures.  Instead, tell all textures that their texture ID is invalid, so it
 * doesn't try to free it later when we really do reload (since that ID might be
 * associated with a different texture).  Ack. */
void RageTextureManager::InvalidateTextures()
{
	FOREACHM( RageTextureID, RageTexture*, m_mapPathToTexture, i )
	{
		RageTexture* pTexture = i->second;
		pTexture->Invalidate();
	}
}

bool RageTextureManager::SetPrefs( RageTextureManagerPrefs prefs )
{
	bool bNeedReload = false;
	if( m_Prefs != prefs )
		bNeedReload = true;

	m_Prefs = prefs;
	
	ASSERT( m_Prefs.m_iTextureColorDepth==16 || m_Prefs.m_iTextureColorDepth==32 );
	ASSERT( m_Prefs.m_iMovieColorDepth==16 || m_Prefs.m_iMovieColorDepth==32 );
	return bNeedReload;
}

void RageTextureManager::DiagnosticOutput() const
{
	unsigned iCount = distance( m_mapPathToTexture.begin(), m_mapPathToTexture.end() );
	LOG->Trace( "%u textures loaded:", iCount );

	int iTotal = 0;
	FOREACHM_CONST( RageTextureID, RageTexture*, m_mapPathToTexture, i )
	{
		const RageTextureID &ID = i->first;
		const RageTexture *pTex = i->second;

		RString sDiags = DISPLAY->GetTextureDiagnostics( pTex->GetTexHandle() );
		RString sStr = ssprintf( "%3ix%3i (%2i)", pTex->GetTextureHeight(), pTex->GetTextureWidth(),
			pTex->m_iRefCount );

		if( sDiags != "" )
			sStr += " " + sDiags;

		LOG->Trace( " %-40s %s", sStr.c_str(), Basename(ID.filename).c_str() );
		iTotal += pTex->GetTextureHeight() * pTex->GetTextureWidth();
	}
	LOG->Trace( "total %3i texels", iTotal );
}

/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard
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
