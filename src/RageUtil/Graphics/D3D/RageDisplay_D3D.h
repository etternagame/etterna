/* RageDisplay_D3D - Direct3D renderer. */

#ifndef RAGE_DISPLAY_D3D_H
#define RAGE_DISPLAY_D3D_H

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnew-returns-null"
#pragma clang diagnostic ignored "-Wcomment"
#endif
#include <d3dx9tex.h>
#include <d3d9.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "RageUtil/Graphics/RenderTarget.h"
#include "RagePixelShaderHandler_D3D.h"
#include "RageVertexShaderHandler_D3D.h"
#include <list>

class RageDisplay_D3D : public RageDisplay
{
  public:
	RageDisplay_D3D();
	~RageDisplay_D3D() override;
	auto Init(VideoModeParams&& p, bool bAllowUnacceleratedRenderer)
	  -> std::string override;

	[[nodiscard]] auto GetApiDescription() const -> std::string override
	{
		return "D3D";
	}
	virtual void GetDisplaySpecs(DisplaySpecs& out) const override;
	void ResolutionChanged() override;
	[[nodiscard]] auto GetPixelFormatDesc(RagePixelFormat pf) const
	  -> const RagePixelFormatDesc* override;

	auto BeginFrame() -> bool override;
	void EndFrame() override;
	[[nodiscard]] auto GetActualVideoModeParams() const
	  -> const ActualVideoModeParams* override;
	void SetBlendMode(BlendMode mode) override;
	auto SupportsTextureFormat(RagePixelFormat pixfmt, bool realtime = false)
	  -> bool override;
	auto SupportsThreadedRendering() -> bool override;
	auto SupportsPerVertexMatrixScale() -> bool override { return false; }
	auto CreateTexture(RagePixelFormat pixfmt,
					   RageSurface* img,
					   bool bGenerateMipMaps) -> intptr_t override;
	void UpdateTexture(intptr_t uTexHandle,
					   RageSurface* img,
					   int xoffset,
					   int yoffset,
					   int width,
					   int height) override;
	void DeleteTexture(intptr_t iTexHandle) override;
	void ClearAllTextures() override;
	auto GetNumTextureUnits() -> int override;
	void SetTexture(TextureUnit tu, intptr_t iTexture) override;
	void SetTextureMode(TextureUnit tu, TextureMode tm) override;
	void SetTextureWrapping(TextureUnit tu, bool b) override;
	[[nodiscard]] auto GetMaxTextureSize() const -> int override;
	void SetTextureFiltering(TextureUnit tu, bool b) override;
	[[nodiscard]] auto IsZWriteEnabled() const -> bool override;
	[[nodiscard]] auto IsZTestEnabled() const -> bool override;
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

	auto CreateRenderTarget(const RenderTargetParam& param,
							int& iTextureWidthOut,
							int& iTextureHeightOut) -> intptr_t override;
	auto GetRenderTarget() -> intptr_t override;
	void SetRenderTarget(intptr_t uTexHandle, bool bPreserveTexture) override;

	void SetSphereEnvironmentMapping(TextureUnit tu, bool b) override;
	void SetCelShaded(int stage) override;

	auto IsD3DInternal() -> bool override;
	[[nodiscard]] auto SupportsFullscreenBorderlessWindow() const
	  -> bool override
	{
		return true;
	}

	auto CreateCompiledGeometry() -> RageCompiledGeometry* override;
	void DeleteCompiledGeometry(RageCompiledGeometry* p) override;

	void SetShader(const RageShaderWeakRef& reference) override;
	RageShaderWeakRef CreateShaderFromPath(const std::string& path,
										   RageShaderType shaderType,
										   bool useAsDefault) override;

	void SetShadersForDeclaration(bool useSpriteDeclaration);
	std::vector<std::string> GetSupportedShaderProfiles() override;
	LPDIRECT3DDEVICE9 GetD3DDevice() { return m_Device; }

  protected:
	void DrawQuadsInternal(const RageSpriteVertex v[], int iNumVerts) override;
	D3DXMATRIX* GetWorldViewProjectionMatrix();
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

	auto TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut)
	  -> std::string override;
	auto CreateScreenshot() -> RageSurface* override;
	auto GetOrthoMatrix(float l, float r, float b, float t, float zn, float zf)
	  -> RageMatrix override;

	void RecoverFromDeviceLoss();
	void SendCurrentMatrices();
	void SetPalette(unsigned TexResource);
	auto FindBackBufferType(bool bWindowed, int iBPP) -> D3DFORMAT;
	auto SetD3DParams(bool& bNewDeviceOut) -> std::string;
	auto D3DReduceParams(D3DPRESENT_PARAMETERS* pp) -> bool;
	void SetPresentParametersFromVideoModeParams(const VideoModeParams& p,
												 D3DPRESENT_PARAMETERS* pD3Dpp);

	std::optional<RageVertexShaderHandler_D3D> m_VertexShaderHandler;
	std::optional<RagePixelShaderHandler_D3D> m_PixelShaderHandler;

	HMODULE m_D3D9_Module = nullptr;
	LPDIRECT3D9 m_D3D = nullptr;
	LPDIRECT3DDEVICE9 m_Device = nullptr;
	D3DCAPS9 m_DeviceCaps;
	D3DDISPLAYMODE m_DesktopMode;
	D3DPRESENT_PARAMETERS m_PresentationParameters;
	int m_ModelMatrixCnt = 0;
	DWORD m_LastFVF = 0;
	bool m_bSphereMapping[NUM_TextureUnit] = { false, false };

	IDirect3DVertexDeclaration9* m_SpriteVertexDeclaration = nullptr;
	IDirect3DVertexDeclaration9* m_ModelVertexDeclaration = nullptr;

	// TODO(Sam): Instead of defining this here, enumerate the possible formats
	// and select whatever one we want to use. This format should be fine for
	// the uses of this application though.
	const D3DFORMAT m_DefaultAdapterFormat = D3DFMT_X8R8G8B8;

	std::map<intptr_t, RenderTarget*> m_mapRenderTargets;
	RenderTarget* m_pCurrentRenderTarget = nullptr;

	bool m_bInvertY = false;

	/* Direct3D doesn't associate a palette with textures. Instead, we load a
	 * palette into a slot. We need to keep track of which texture's palette is
	 * stored in what slot. */
	std::map<intptr_t, int> m_TexResourceToPaletteIndex;
	std::list<int> m_PaletteIndex;

	struct TexturePalette
	{
		PALETTEENTRY p[256];
	};

	std::map<intptr_t, TexturePalette> m_TexResourceToTexturePalette;
};

#endif
