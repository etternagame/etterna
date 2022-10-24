/* RageDisplay_Legacy: OpenGL renderer. */

#ifndef RAGE_DISPLAY_OGL_H
#define RAGE_DISPLAY_OGL_H

#include "Etterna/Models/Misc/DisplaySpec.h"
#include "RageDisplay.h"
#include "Etterna/Actor/Base/Sprite.h"
#include "RageUtil/Graphics/RageTextureRenderTarget.h"

/* Making an OpenGL call doesn't also flush the error state; if we happen
 * to have an error from a previous call, then the assert below will fail.
 * Flush it. */
#define FlushGLErrors()                                                        \
	do {                                                                       \
	} while (glGetError() != GL_NO_ERROR)
#define AssertNoGLError()                                                      \
                                                                               \
	{                                                                          \
		GLenum error = glGetError();                                           \
		ASSERT_M(error == GL_NO_ERROR,                                         \
				 RageDisplay_Legacy_Helpers::GLToString(error));               \
	}

#if defined(DEBUG) || !defined(GL_GET_ERROR_IS_SLOW)
#define DebugFlushGLErrors() FlushGLErrors()
#define DebugAssertNoGLError() AssertNoGLError()
#else
#define DebugFlushGLErrors()
#define DebugAssertNoGLError()
#endif

class RageDisplay_Legacy : public RageDisplay
{
  public:
	RageDisplay_Legacy();
	~RageDisplay_Legacy() override;
	std::string Init(VideoModeParams&& p,
					 bool bAllowUnacceleratedRenderer) override;

	std::string GetApiDescription() const override { return "OpenGL"; }
	void GetDisplaySpecs(DisplaySpecs& out) const override;
	void ResolutionChanged() override;
	const RagePixelFormatDesc* GetPixelFormatDesc(
	  RagePixelFormat pf) const override;

	bool SupportsThreadedRendering() override;
	void BeginConcurrentRenderingMainThread() override;
	void EndConcurrentRenderingMainThread() override;
	void BeginConcurrentRendering() override;
	void EndConcurrentRendering() override;

	bool BeginFrame() override;
	void EndFrame() override;
	const ActualVideoModeParams* GetActualVideoModeParams() const override;
	void SetBlendMode(BlendMode mode) override;
	bool SupportsTextureFormat(RagePixelFormat pixfmt,
							   bool realtime = false) override;
	bool SupportsPerVertexMatrixScale() override;
	intptr_t CreateTexture(RagePixelFormat pixfmt,
						   RageSurface* img,
						   bool bGenerateMipMaps) override;
	void UpdateTexture(intptr_t iTexHandle,
					   RageSurface* img,
					   int xoffset,
					   int yoffset,
					   int width,
					   int height) override;
	void DeleteTexture(intptr_t iTexHandle) override;
	bool UseOffscreenRenderTarget();
	RageSurface* GetTexture(intptr_t iTexture) override;
	RageTextureLock* CreateTextureLock() override;

	void ClearAllTextures() override;
	int GetNumTextureUnits() override;
	void SetTexture(TextureUnit tu, intptr_t iTexture) override;
	void SetTextureMode(TextureUnit tu, TextureMode tm) override;
	void SetTextureWrapping(TextureUnit tu, bool b) override;
	int GetMaxTextureSize() const override;
	void SetTextureFiltering(TextureUnit tu, bool b) override;
	void SetEffectMode(EffectMode effect) override;
	bool IsEffectModeSupported(EffectMode effect) override;
	bool SupportsRenderToTexture() const override;
	bool SupportsFullscreenBorderlessWindow() const override;
	intptr_t CreateRenderTarget(const RenderTargetParam& param,
								int& iTextureWidthOut,
								int& iTextureHeightOut) override;
	intptr_t GetRenderTarget() override;
	void SetRenderTarget(intptr_t iHandle, bool bPreserveTexture) override;
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

	bool IsD3DInternal() override;

	RageCompiledGeometry* CreateCompiledGeometry() override;
	void DeleteCompiledGeometry(RageCompiledGeometry* p) override;

	// hacks for cell-shaded models
	void SetPolygonMode(PolygonMode pm) override;
	void SetLineWidth(float fWidth) override;

	std::string GetTextureDiagnostics(unsigned id) const override;

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
	RagePixelFormat GetImgPixelFormat(RageSurface*& img,
									  bool& FreeImg,
									  int width,
									  int height,
									  bool bPalettedTexture);
	bool SupportsSurfaceFormat(RagePixelFormat pixfmt);

	void SendCurrentMatrices();

  private:
	RageTextureRenderTarget* offscreenRenderTarget = nullptr;
};

#endif
