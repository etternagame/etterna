/* RageDisplay_Null - No-op diagnostic renderer. */

#ifndef RAGE_DISPLAY_NULL_H
#define RAGE_DISPLAY_NULL_H

class RageDisplay_Null: public RageDisplay
{
public:
	RageDisplay_Null();
	RString Init( const VideoModeParams &p, bool bAllowUnacceleratedRenderer ) override;

	RString GetApiDescription() const override { return "Null"; }
	void GetDisplayResolutions( DisplayResolutions &out ) const override;
	const RagePixelFormatDesc *GetPixelFormatDesc(RagePixelFormat pf) const override;

	bool BeginFrame() override { return true; }
	void EndFrame() override;
	const VideoModeParams* GetActualVideoModeParams() const override { return (VideoModeParams*)&m_Params; }
	void SetBlendMode( BlendMode ) override { }
	bool SupportsTextureFormat( RagePixelFormat, bool /* realtime */ =false ) override { return true; }
	bool SupportsPerVertexMatrixScale() override { return false; }
	unsigned CreateTexture( 
		RagePixelFormat, 
		RageSurface* /* img */,
		bool /* bGenerateMipMaps */ ) override { return 1; }
	void UpdateTexture( 
		unsigned /* iTexHandle */, 
		RageSurface* /* img */,
		int /* xoffset */, int /* yoffset */, int /* width */, int /* height */ 
		) override { }
	void DeleteTexture( unsigned /* iTexHandle */ ) override { }
	void ClearAllTextures() override { }
	int GetNumTextureUnits() override { return 1; }
	void SetTexture( TextureUnit, unsigned /* iTexture */ ) override { }
	void SetTextureMode( TextureUnit, TextureMode ) override { }
	void SetTextureWrapping( TextureUnit, bool ) override { }
	int GetMaxTextureSize() const override { return 2048; }
	void SetTextureFiltering( TextureUnit, bool ) override { }
	bool IsZWriteEnabled() const override { return false; }
	bool IsZTestEnabled() const override { return false; }
	void SetZWrite( bool ) override { }
	void SetZBias( float ) override { }
	void SetZTestMode( ZTestMode ) override { }
	void ClearZBuffer() override { }
	void SetCullMode( CullMode ) override { }
	void SetAlphaTest( bool ) override { }
	void SetMaterial( 
		const RageColor & /* unreferenced: emissive */,
		const RageColor & /* unreferenced: ambient */,
		const RageColor & /* unreferenced: diffuse */,
		const RageColor & /* unreferenced: specular */,
		float /* unreferenced: shininess */
		) override { }
	void SetLighting( bool ) override { }
	void SetLightOff( int /* index */ ) override { }
	void SetLightDirectional( 
		int /* index */, 
		const RageColor & /* unreferenced: ambient */, 
		const RageColor & /* unreferenced: diffuse */, 
		const RageColor & /* unreferenced: specular */, 
		const RageVector3 & /* unreferenced: dir */ ) override { }

	void SetSphereEnvironmentMapping( TextureUnit /* tu */, bool /* b */ ) override { }
	void SetCelShaded( int /* stage */ ) override { }

	bool IsD3DInternal() override;

	RageCompiledGeometry* CreateCompiledGeometry() override;
	void DeleteCompiledGeometry( RageCompiledGeometry* ) override;

protected:
	void DrawQuadsInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) override { }
	void DrawQuadStripInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) override { }
	void DrawFanInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) override { }
	void DrawStripInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) override { }
	void DrawTrianglesInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) override { }
	void DrawCompiledGeometryInternal( const RageCompiledGeometry *p, int /* iMeshIndex */ ) override { }
	void DrawLineStripInternal( const RageSpriteVertex v[], int /* iNumVerts */, float /* LineWidth */ ) override { }
	void DrawSymmetricQuadStripInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) override { }

	VideoModeParams m_Params;
	RString TryVideoMode( const VideoModeParams &p, bool & /* bNewDeviceOut */ ) override { m_Params = p; return RString(); }
	RageSurface* CreateScreenshot() override;
	RageMatrix GetOrthoMatrix( float l, float r, float b, float t, float zn, float zf ) override; 
	bool SupportsSurfaceFormat( RagePixelFormat ) { return true; }
};

#endif
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

