#ifndef RAGE_DISPLAY_GLES2_H
#define RAGE_DISPLAY_GLES2_H

#include "RageDisplay.h"

class RageDisplay_GLES2 : public RageDisplay
{
  public:
	RageDisplay_GLES2();
	~RageDisplay_GLES2() override;
	std::string Init(VideoModeParams&& p,
				 bool bAllowUnacceleratedRenderer) override;

	std::string GetApiDescription() const override;
	virtual void GetDisplayResolutions(DisplayResolutions& out) const override;
	const RagePixelFormatDesc* GetPixelFormatDesc(
	  RagePixelFormat pf) const override;

	bool BeginFrame() override;
	void EndFrame() override;
	const VideoModeParams* GetActualVideoModeParams() const override;
	void SetBlendMode(BlendMode mode) override;
	bool SupportsTextureFormat(RagePixelFormat pixfmt,
							   bool realtime = false) override;
	bool SupportsPerVertexMatrixScale() override;
	virtual intptr_t CreateTexture(RagePixelFormat pixfmt,
								   RageSurface* img,
								   bool bGenerateMipMaps) override;
	void UpdateTexture(intptr_t iTexHandle,
					   RageSurface* img,
					   int xoffset,
					   int yoffset,
					   int width,
					   int height) override;
	void DeleteTexture(intptr_t iTexHandle) override;
	void ClearAllTextures() override;
	int GetNumTextureUnits() override;
	void SetTexture(TextureUnit tu, intptr_t iTexture) override;
	void SetTextureMode(TextureUnit tu, TextureMode tm) override;
	void SetTextureWrapping(TextureUnit tu, bool b) override;
	int GetMaxTextureSize() const override;
	void SetTextureFiltering(TextureUnit tu, bool b) override;
	bool IsZWriteEnabled() const override;
	bool IsZTestEnabled() const override;
	void SetZWrite(bool b) override;
	void SetZBias(float f) override;
	void SetZTestMode(ZTestMode mode) override;
	void ClearZBuffer() override;
	void SetCullMode(CullMode mode) override;
	void SetAlphaTest(bool b) override;
	void SetMaterial(const RageColor& emissive,
					 const RageColor& ambient,
					 const RageColor& diffuse,
					 const RageColor& specular,
					 float shininess) override;
	void SetLighting(bool b) override;
	void SetLightOff(int index) override;
	void SetLightDirectional(int index,
							 const RageColor& ambient,
							 const RageColor& diffuse,
							 const RageColor& specular,
							 const RageVector3& dir) override;

	void SetSphereEnvironmentMapping(TextureUnit tu, bool b) override;
	void SetCelShaded(int stage) override;

	void SetLineWidth(float fWidth) override;
	void SetPolygonMode(PolygonMode pm) override;

	RageCompiledGeometry* CreateCompiledGeometry() override;
	void DeleteCompiledGeometry(RageCompiledGeometry* p) override;

  protected:
	void DrawQuadsInternal(const RageSpriteVertex v[], int iNumVerts) override;
	void DrawQuadStripInternal(const RageSpriteVertex v[],
							   int iNumVerts) override;
	void DrawFanInternal(const RageSpriteVertex v[], int iNumVerts) override;
	void DrawStripInternal(const RageSpriteVertex v[], int iNumVerts) override;
	void DrawTrianglesInternal(const RageSpriteVertex v[],
							   int iNumVerts) override;
	void DrawCompiledGeometryInternal(const RageCompiledGeometry* p,
									  int iMeshIndex) override;
	void DrawLineStripInternal(const RageSpriteVertex v[],
							   int iNumVerts,
							   float LineWidth) override;
	void DrawSymmetricQuadStripInternal(const RageSpriteVertex v[],
										int iNumVerts) override;

	std::string TryVideoMode(const VideoModeParams& p,
						 bool& bNewDeviceOut) override;
	RageSurface* CreateScreenshot() override;
	RageMatrix GetOrthoMatrix(float l,
							  float r,
							  float b,
							  float t,
							  float zn,
							  float zf) override;
	bool SupportsSurfaceFormat(RagePixelFormat pixfmt);
	bool SupportsRenderToTexture() const { return true; }
};

#endif
