/* Model - A 3D model. */

#ifndef MODEL_H
#define MODEL_H

#include "Actor.h"
#include "RageTypes.h"
#include "ModelTypes.h"
#include <vector>
#include <map>

class RageModelGeometry;
class RageCompiledGeometry;

class Model : public Actor
{
public:
	Model();
	~Model() override;
	Model *Copy() const override;

	void	Clear();
	void	Load( const RString &sFile );

	void	LoadPieces( const RString &sMeshesPath, const RString &sMaterialsPath, const RString &sBomesPath );
	void	LoadMilkshapeAscii( const RString &sFile );
	void 	LoadMaterialsFromMilkshapeAscii( const RString &sPath );
	bool	LoadMilkshapeAsciiBones( const RString &sAniName, const RString &sPath );

	void LoadFromNode( const XNode* pNode ) override;

	void	PlayAnimation( const RString &sAniName, float fPlayRate = 1 );
	void	SetRate( float fRate ) { m_fCurAnimationRate = fRate; }
	void	SetLoop( bool b ) { m_bLoop = b; }
	void	SetPosition( float fSeconds );

	void	Update( float fDelta ) override;
	bool	EarlyAbortDraw() const override;
	void	DrawPrimitives() override;

	void	DrawCelShaded();
	void	SetCelShading( bool bShading ) { m_bDrawCelShaded = bShading; }

	int GetNumStates() const override;
	void SetState( int iNewState ) override;
	float GetAnimationLengthSeconds() const override
	{ return m_animation_length_seconds; }
	virtual void RecalcAnimationLengthSeconds();
	void SetSecondsIntoAnimation( float fSeconds ) override;

	RString		GetDefaultAnimation() const { return m_sDefaultAnimation; };
	void		SetDefaultAnimation( const RString &sAnimation, float fPlayRate = 1 );

	bool	MaterialsNeedNormals() const;

	// Lua
	void PushSelf( lua_State *L ) override;

private:
	RageModelGeometry		*m_pGeometry;

	float m_animation_length_seconds;
	vector<msMaterial>		m_Materials;
	map<RString,msAnimation>	m_mapNameToAnimation;
	const msAnimation*		m_pCurAnimation;

	static void SetBones( const msAnimation* pAnimation, float fFrame, vector<myBone_t> &vpBones );
	vector<myBone_t>	m_vpBones;

	// If any vertex has a bone weight, then then render from m_pTempGeometry.  
	// Otherwise, render directly from m_pGeometry.
	RageCompiledGeometry*		m_pTempGeometry;
	void UpdateTempGeometry();

	/* Keep a copy of the mesh data only if m_pTempGeometry is in use.  The normal and
	 * position data will be changed; the rest is static and kept only to prevent making
	 * a complete copy. */
	vector<msMesh>	m_vTempMeshes;

	void DrawMesh( int i ) const;
	void AdvanceFrame( float fDeltaTime );

	float			m_fCurFrame;
	RString			m_sDefaultAnimation;
	float			m_fDefaultAnimationRate;
	float			m_fCurAnimationRate;
	bool			m_bLoop;
	bool			m_bDrawCelShaded; // for Lua models

	Model& operator=(const Model& rhs);
};

#endif

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
