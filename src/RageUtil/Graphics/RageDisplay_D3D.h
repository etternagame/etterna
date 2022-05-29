/* RageDisplay_D3D - Direct3D renderer. */

#ifndef RAGE_DISPLAY_D3D_H
#define RAGE_DISPLAY_D3D_H

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

	auto TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut)
	  -> std::string override;
	auto CreateScreenshot() -> RageSurface* override;
	auto GetOrthoMatrix(float l, float r, float b, float t, float zn, float zf)
	  -> RageMatrix override;

	static void RecoverFromDeviceLoss();
	void SendCurrentMatrices();
};

class RenderTarget
{
  public:
	virtual ~RenderTarget() = default;

	virtual void Create(const RenderTargetParam& param,
						int& iTextureWidthOut,
						int& iTextureHeightOut) = 0;

	[[nodiscard]] virtual auto GetTexture() const -> intptr_t = 0;

	/* Render to this RenderTarget. */
	virtual void StartRenderingTo() = 0;

	/* Stop rendering to this RenderTarget.  Update the texture, if necessary,
	 * and make it available. */
	virtual void FinishRenderingTo() = 0;

	[[nodiscard]] virtual auto InvertY() const -> bool { return false; }

	[[nodiscard]] auto GetParam() const -> const RenderTargetParam&
	{
		return m_Param;
	}

  protected:
	RenderTargetParam m_Param;
};

#endif
