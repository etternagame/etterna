/* RageDisplay_Null - No-op diagnostic renderer. */

#ifndef RAGE_DISPLAY_NULL_H
#define RAGE_DISPLAY_NULL_H

class RageDisplay_Null : public RageDisplay
{
  public:
	RageDisplay_Null();
	std::string Init(VideoModeParams&& p,
					 bool bAllowUnacceleratedRenderer) override;

	std::string GetApiDescription() const override { return "Null"; }
	void GetDisplaySpecs(DisplaySpecs& out) const override;
	const RagePixelFormatDesc* GetPixelFormatDesc(
	  RagePixelFormat pf) const override;

	bool BeginFrame() override { return true; }
	void EndFrame() override;
	const ActualVideoModeParams* GetActualVideoModeParams() const override
	{
		return (ActualVideoModeParams*)&m_Params;
	}
	void SetBlendMode(BlendMode) override {}
	bool SupportsTextureFormat(RagePixelFormat,
							   bool /* realtime */ = false) override
	{
		return true;
	}
	bool SupportsPerVertexMatrixScale() override { return false; }
	intptr_t CreateTexture(RagePixelFormat,
						   RageSurface* /* img */,
						   bool /* bGenerateMipMaps */) override
	{
		return 1;
	}
	void UpdateTexture(intptr_t /* iTexHandle */,
					   RageSurface* /* img */,
					   int /* xoffset */,
					   int /* yoffset */,
					   int /* width */,
					   int /* height */
					   ) override
	{
	}
	void DeleteTexture(intptr_t /* iTexHandle */) override {}
	void ClearAllTextures() override {}
	int GetNumTextureUnits() override { return 1; }
	void SetTexture(TextureUnit, intptr_t /* iTexture */) override {}
	void SetTextureMode(TextureUnit, TextureMode) override {}
	void SetTextureWrapping(TextureUnit, bool) override {}
	int GetMaxTextureSize() const override { return 2048; }
	void SetTextureFiltering(TextureUnit, bool) override {}
	bool IsZWriteEnabled() const override { return false; }
	bool IsZTestEnabled() const override { return false; }
	void SetZWrite(bool) override {}
	void SetZBias(float) override {}
	void SetZTestMode(ZTestMode) override {}
	void ClearZBuffer() override {}
	void SetCullMode(CullMode) override {}
	void SetAlphaTest(bool) override {}
	void SetMaterial(const RageColor& /* unreferenced: emissive */,
					 const RageColor& /* unreferenced: ambient */,
					 const RageColor& /* unreferenced: diffuse */,
					 const RageColor& /* unreferenced: specular */,
					 float /* unreferenced: shininess */
					 ) override
	{
	}
	void SetLighting(bool) override {}
	void SetLightOff(int /* index */) override {}
	void SetLightDirectional(
	  int /* index */,
	  const RageColor& /* unreferenced: ambient */,
	  const RageColor& /* unreferenced: diffuse */,
	  const RageColor& /* unreferenced: specular */,
	  const RageVector3& /* unreferenced: dir */) override
	{
	}

	void SetSphereEnvironmentMapping(TextureUnit /* tu */,
									 bool /* b */) override
	{
	}
	void SetCelShaded(int /* stage */) override {}

	bool IsD3DInternal() override;

	RageCompiledGeometry* CreateCompiledGeometry() override;
	void DeleteCompiledGeometry(RageCompiledGeometry*) override;

  protected:
	void DrawQuadsInternal(const RageSpriteVertex v[],
						   int /* iNumVerts */) override
	{
	}
	void DrawQuadStripInternal(const RageSpriteVertex v[],
							   int /* iNumVerts */) override
	{
	}
	void DrawFanInternal(const RageSpriteVertex v[],
						 int /* iNumVerts */) override
	{
	}
	void DrawStripInternal(const RageSpriteVertex v[],
						   int /* iNumVerts */) override
	{
	}
	void DrawTrianglesInternal(const RageSpriteVertex v[],
							   int /* iNumVerts */) override
	{
	}
	void DrawCompiledGeometryInternal(const RageCompiledGeometry* p,
									  int /* iMeshIndex */) override
	{
	}
	void DrawLineStripInternal(const RageSpriteVertex v[],
							   int /* iNumVerts */,
							   float /* LineWidth */) override
	{
	}
	void DrawSymmetricQuadStripInternal(const RageSpriteVertex v[],
										int /* iNumVerts */) override
	{
	}

	VideoModeParams m_Params;
	std::string TryVideoMode(const VideoModeParams& p,
							 bool& /* bNewDeviceOut */) override
	{
		m_Params = p;
		return std::string();
	}
	RageSurface* CreateScreenshot() override;
	RageMatrix GetOrthoMatrix(float l,
							  float r,
							  float b,
							  float t,
							  float zn,
							  float zf) override;
	bool SupportsSurfaceFormat(RagePixelFormat) { return true; }
};

#endif
