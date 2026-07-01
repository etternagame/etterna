/* Display - RageDisplay wrapper for renderer implementations */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "RageUtil/Graphics/Display/CommandBatcher.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "Renderer.h"
#include "RenderState.h"
#include <functional>
#include "arch/LowLevelWindowVK/LowLevelWindowVK.h"

namespace DisplayAdapter {
class Display : public RageDisplay
{
  public:
	static constexpr size_t TexturePixelSize = 4;
	static constexpr size_t MaxTextureSize = 4096;

	Display(std::unique_ptr<Renderer> renderer);
	~Display() override {}

	std::string Init(VideoModeParams&& p,
					 bool bAllowUnacceleratedRenderer) override;
	[[nodiscard]] std::string GetApiDescription() const override
	{
		return m_Renderer->GetApiDescription();
	}
	void GetDisplaySpecs(DisplaySpecs& out) const override;
	void ResolutionChanged() override;
	[[nodiscard]] const RagePixelFormatDesc* GetPixelFormatDesc(
	  RagePixelFormat pf) const override;

	bool BeginFrame() override;
	void EndFrame() override;
	[[nodiscard]] const ActualVideoModeParams* GetActualVideoModeParams()
	  const override;
	void SetBlendMode(BlendMode mode) override;
	bool SupportsTextureFormat(RagePixelFormat pixfmt,
							   bool realtime = false) override;
	bool SupportsThreadedRendering() override;
	bool SupportsPerVertexMatrixScale() override;

	intptr_t CreateTexture(RagePixelFormat pixfmt,
						   RageSurface* img,
						   bool bGenerateMipMaps) override;
	void UpdateTexture(intptr_t uTexHandle,
					   RageSurface* img,
					   int xoffset,
					   int yoffset,
					   int width,
					   int height) override;
	void DeleteTexture(intptr_t iTexHandle) override;
	void ClearAllTextures() override;
	[[nodiscard]] int GetMaxTextureSize() const override;

	int GetNumTextureUnits() override;
	void SetTexture(TextureUnit tu, intptr_t iTexture) override;
	void SetTextureMode(TextureUnit tu, TextureMode tm) override;
	void SetTextureWrapping(TextureUnit tu, bool b) override;
	void SetTextureFiltering(TextureUnit tu, bool b) override;
	[[nodiscard]] bool IsZWriteEnabled() const override;
	[[nodiscard]] bool IsZTestEnabled() const override;
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
	intptr_t CreateGraphicsPipeline(
	  const std::string& vertexShaderPath,
	  const std::string& fragmentShaderPath) override;
	void SetGraphicsPipeline(intptr_t pipeline,
							 const std::vector<uint8_t>& vertexShaderArgs,
							 const std::vector<uint8_t>& fragShaderArgs,
							 bool persist) override;
	void ReloadPipelines() override;
	intptr_t CreateRenderTarget(const RenderTargetParam& param,
								int& iTextureWidthOut,
								int& iTextureHeightOut) override;
	intptr_t GetRenderTarget() override;
	void SetRenderTarget(intptr_t uTexHandle, bool bPreserveTexture) override;

	void SetSphereEnvironmentMapping(TextureUnit tu, bool b) override;
	void SetCelShaded(int stage) override;

	bool IsD3DInternal() override { return m_Renderer->IsD3DInternal(); }
	[[nodiscard]] bool SupportsFullscreenBorderlessWindow() const override
	{
		return true;
	}

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
	void DrawSymmetricQuadStripInternal(const RageSpriteVertex v[],
										int iNumVerts) override;
	void DrawCompiledGeometryInternal(const RageCompiledGeometry* p,
									  int iMeshIndex) override;

	std::string TryVideoMode(const VideoModeParams& p,
							 bool& bNewDeviceOut) override;
	RageSurface* CreateScreenshot() override;
	MatrixState GetCurrentMatrixState();

  private:
	std::unique_ptr<Renderer> m_Renderer;
	bool m_IsInitDone = false;
	CommandBatcher m_Batcher;
	RenderState m_RenderState;
	intptr_t m_CurrentRenderTarget = 0;

	LowLevelWindowVK* m_Window = nullptr;
};
} // namespace DisplayAdapter

#endif
