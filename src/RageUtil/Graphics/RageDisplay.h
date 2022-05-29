/* RageDisplay - Renderer base class. */

#ifndef RAGEDISPLAY_H
#define RAGEDISPLAY_H

#include "Etterna/Actor/Base/ModelTypes.h"
#include "RageUtil/Misc/RageTypes.h"

#include <chrono>
#include <set>
#include <utility>

class DisplaySpec;
using DisplaySpecs = std::set<DisplaySpec>;

const int REFRESH_DEFAULT = 0;
struct RageSurface;
enum TextureUnit
{
	TextureUnit_1,
	TextureUnit_2,
	TextureUnit_3,
	TextureUnit_4,
	TextureUnit_5,
	TextureUnit_6,
	TextureUnit_7,
	TextureUnit_8,
	NUM_TextureUnit
};

// RageCompiledGeometry holds vertex data in a format that is most efficient
// for the graphics API.
class RageCompiledGeometry
{
  public:
	virtual ~RageCompiledGeometry();

	void Set(const std::vector<msMesh>& vMeshes, bool bNeedsNormals);

	virtual void Allocate(const std::vector<msMesh>& vMeshes) = 0; // allocate space
	virtual void Change(const std::vector<msMesh>& vMeshes) = 0; // new data must be
															// the same size as
															// was passed to
															// Set()
	virtual void Draw(int iMeshIndex) const = 0;

  protected:
	[[nodiscard]] auto GetTotalVertices() const -> size_t
	{
		if (m_vMeshInfo.empty()) {
			return 0;
		}
		return m_vMeshInfo.back().iVertexStart +
			   m_vMeshInfo.back().iVertexCount;
	}
	[[nodiscard]] auto GetTotalTriangles() const -> size_t
	{
		if (m_vMeshInfo.empty()) {
			return 0;
		}
		return m_vMeshInfo.back().iTriangleStart +
			   m_vMeshInfo.back().iTriangleCount;
	}

	struct MeshInfo
	{
		int iVertexStart;
		int iVertexCount;
		int iTriangleStart;
		int iTriangleCount;
		bool m_bNeedsTextureMatrixScale;
	};
	std::vector<MeshInfo> m_vMeshInfo;
	bool m_bNeedsNormals{};
	bool m_bAnyNeedsTextureMatrixScale{};
};

enum RagePixelFormat
{
	RagePixelFormat_RGBA8,
	RagePixelFormat_BGRA8,
	RagePixelFormat_RGBA4,
	RagePixelFormat_RGB5A1,
	RagePixelFormat_RGB5,
	RagePixelFormat_RGB8,
	RagePixelFormat_PAL,
	/* The above formats differ between OpenGL and D3D. These are provided as
	 * alternatives for OpenGL that match some format in D3D.  Don't use them
	 * directly; they'll be matched automatically by FindPixelFormat. */
	RagePixelFormat_BGR8,
	RagePixelFormat_A1BGR5,
	RagePixelFormat_X1RGB5,
	NUM_RagePixelFormat,
	RagePixelFormat_Invalid
};
auto
RagePixelFormatToString(RagePixelFormat i) -> const std::string&;

/** @brief The parameters used for the present Video Mode. */
class VideoModeParams
{
  public:
	// Initialize with a constructor so to guarantee all paramters
	// are filled (in case new params are added).
	VideoModeParams(bool windowed_,
					std::string sDisplayId_,
					int width_,
					int height_,
					int bpp_,
					int rate_,
					bool vsync_,
					bool interlaced_,
					bool bSmoothLines_,
					bool bTrilinearFiltering_,
					bool bAnisotropicFiltering_,
					bool bWindowIsFullscreenBorderless_,
					std::string sWindowTitle_,
					std::string sIconFile_,
					bool PAL_,
					float fDisplayAspectRatio_)
	  : windowed(windowed_)
	  , sDisplayId(std::move(sDisplayId_))
	  , width(width_)
	  , height(height_)
	  , bpp(bpp_)
	  , rate(rate_)
	  , vsync(vsync_)
	  , interlaced(interlaced_)
	  , bSmoothLines(bSmoothLines_)
	  , bTrilinearFiltering(bTrilinearFiltering_)
	  , bAnisotropicFiltering(bAnisotropicFiltering_)
	  , bWindowIsFullscreenBorderless(bWindowIsFullscreenBorderless_)
	  , sWindowTitle(std::move(sWindowTitle_))
	  , sIconFile(std::move(sIconFile_))
	  , PAL(PAL_)
	  , fDisplayAspectRatio(fDisplayAspectRatio_)
	{
	}

	VideoModeParams(const VideoModeParams& other)
	  : windowed(other.windowed)
	  , sDisplayId(other.sDisplayId)
	  , width(other.width)
	  , height(other.height)
	  , bpp(other.bpp)
	  , rate(other.rate)
	  , vsync(other.vsync)
	  , interlaced(other.interlaced)
	  , bSmoothLines(other.bSmoothLines)
	  , bTrilinearFiltering(other.bTrilinearFiltering)
	  , bAnisotropicFiltering(other.bAnisotropicFiltering)
	  , bWindowIsFullscreenBorderless(other.bWindowIsFullscreenBorderless)
	  , sWindowTitle(other.sWindowTitle)
	  , sIconFile(other.sIconFile)
	  , PAL(other.PAL)
	  , fDisplayAspectRatio(other.fDisplayAspectRatio)
	{
	}

	VideoModeParams(VideoModeParams&& other)
	  : windowed(other.windowed)
	  , sDisplayId(std::move(other.sDisplayId))
	  , width(other.width)
	  , height(other.height)
	  , bpp(other.bpp)
	  , rate(other.rate)
	  , vsync(other.vsync)
	  , interlaced(other.interlaced)
	  , bSmoothLines(other.bSmoothLines)
	  , bTrilinearFiltering(other.bTrilinearFiltering)
	  , bAnisotropicFiltering(other.bAnisotropicFiltering)
	  , bWindowIsFullscreenBorderless(other.bWindowIsFullscreenBorderless)
	  , sWindowTitle(std::move(other.sWindowTitle))
	  , sIconFile(std::move(other.sIconFile))
	  , PAL(other.PAL)
	  , fDisplayAspectRatio(other.fDisplayAspectRatio)
	{
	}
	VideoModeParams() = default;
	VideoModeParams& operator=(const VideoModeParams&) = default;
	virtual ~VideoModeParams() {}

	bool windowed{ false };
	std::string sDisplayId;
	int width{ 0 };
	int height{ 0 };
	int bpp{ 0 };
	int rate{ 0 };
	bool vsync{ false };
	bool interlaced{ false };
	bool bSmoothLines{ false };
	bool bTrilinearFiltering{ false };
	bool bAnisotropicFiltering{ false };
	bool bWindowIsFullscreenBorderless{ false };
	std::string sWindowTitle;
	std::string sIconFile;
	bool PAL{ false };
	float fDisplayAspectRatio{ 0.0F };
};

/**
 * @brief The _actual_ VideoModeParams determined by the LowLevelWindow
 * implementation. Contains all the attributes of VideoModeParams, plus the
 * actual window width/height determined by LLW
 */
class ActualVideoModeParams : public VideoModeParams
{
  public:
	ActualVideoModeParams(const VideoModeParams& params)
	  : VideoModeParams(params)
	  , windowWidth(params.width)
	  , windowHeight(params.height)
	{
	}
	ActualVideoModeParams(VideoModeParams&& params)
	  : VideoModeParams(params)
	  , windowWidth(params.width)
	  , windowHeight(params.height)
	{
	}
	ActualVideoModeParams(const VideoModeParams& params,
						  int windowWidth,
						  int windowHeight,
						  bool renderOffscreen)
	  : VideoModeParams(params)
	  , windowWidth(windowWidth)
	  , windowHeight(windowHeight)
	  , renderOffscreen(renderOffscreen)
	{
	}

	ActualVideoModeParams() = default;
	ActualVideoModeParams& operator=(const ActualVideoModeParams&) = default;

	// If bWindowIsFullscreenBorderless is true,
	// then these properties will differ from width/height (which describe the
	// render size)
	int windowWidth{ 0 };
	int windowHeight{ 0 };
	bool renderOffscreen{ false };
};

struct RenderTargetParam
{
	RenderTargetParam() = default;

	// The dimensions of the actual render target, analogous to a window size:
	int iWidth{ 0 }, iHeight{ 0 };

	bool bWithDepthBuffer{ false };
	bool bWithAlpha{ false };
	bool bFloat{ false };
};

struct RageTextureLock
{
	virtual ~RageTextureLock() = default;

	/* Given a surface with a format and no pixel data, lock the texture into
	 * the surface. The data is write-only. */
	virtual void Lock(unsigned iTexHandle, RageSurface* pSurface) = 0;

	/* Unlock and update the texture. If bChanged is false, the texture update
	 * may be omitted. */
	virtual void Unlock(RageSurface* pSurface, bool bChanged = true) = 0;
};

class RageDisplay
{
	friend class RageTexture;

  public:
	struct RagePixelFormatDesc
	{
		int bpp;
		unsigned int masks[4];
	};

	[[nodiscard]] virtual auto GetPixelFormatDesc(RagePixelFormat pf) const
	  -> const RagePixelFormatDesc* = 0;

	RageDisplay();
	virtual ~RageDisplay();

	virtual auto Init(VideoModeParams&& p,
					  bool bAllowUnacceleratedRenderer) -> std::string = 0;

	[[nodiscard]] virtual auto GetApiDescription() const -> std::string = 0;
	virtual void GetDisplaySpecs(DisplaySpecs& out) const = 0;

	void SetPresentTime(std::chrono::nanoseconds presentTime);

	// Don't override this.  Override TryVideoMode() instead.
	// This will set the video mode to be as close as possible to params.
	// Return true if device was re-created and we need to reload textures.
	auto SetVideoMode(VideoModeParams&& p, bool& bNeedReloadTextures)
	  -> std::string;

	// Call this when the resolution has been changed externally:
	virtual void ResolutionChanged();
	auto IsD3D() -> bool;

	virtual auto BeginFrame() -> bool;
	virtual void EndFrame();
	[[nodiscard]] virtual auto GetActualVideoModeParams() const
	  -> const ActualVideoModeParams* = 0;
	auto IsWindowed() -> bool { return (*GetActualVideoModeParams()).windowed; }

	auto GetFrameTimingAdjustment(std::chrono::steady_clock::time_point now)
		-> float;

	virtual void SetBlendMode(BlendMode mode) = 0;

	virtual auto SupportsTextureFormat(RagePixelFormat pixfmt,
									   bool realtime = false) -> bool = 0;
	virtual auto SupportsThreadedRendering() -> bool { return false; }
	virtual auto SupportsPerVertexMatrixScale() -> bool = 0;

	// If threaded rendering is supported, these will be called from the
	// rendering thread before and after rendering.
	virtual void BeginConcurrentRenderingMainThread() {}
	virtual void EndConcurrentRenderingMainThread() {}
	virtual void BeginConcurrentRendering();
	virtual void EndConcurrentRendering() {}

	/* return 0 if failed or internal texture resource handle
	 * (unsigned in OpenGL, texture pointer in D3D) */
	virtual auto CreateTexture(
	  RagePixelFormat pixfmt, // format of img and of texture in video mem
	  RageSurface* img,		  // must be in pixfmt
	  bool bGenerateMipMaps) -> intptr_t = 0;
	virtual void UpdateTexture(intptr_t iTexHandle,
							   RageSurface* img,
							   int xoffset,
							   int yoffset,
							   int width,
							   int height) = 0;
	virtual void DeleteTexture(intptr_t iTexHandle) = 0;
	/* Return an object to lock pixels for streaming. If not supported, returns
	 * NULL. Delete the object normally. */
	virtual auto CreateTextureLock() -> RageTextureLock* { return nullptr; }
	virtual void ClearAllTextures() = 0;
	virtual auto GetNumTextureUnits() -> int = 0;
	virtual void SetTexture(TextureUnit, intptr_t /* iTexture */) = 0;
	virtual void SetTextureMode(TextureUnit, TextureMode) = 0;
	virtual void SetTextureWrapping(TextureUnit, bool) = 0;
	[[nodiscard]] virtual auto GetMaxTextureSize() const -> int = 0;
	virtual void SetTextureFiltering(TextureUnit, bool) = 0;
	virtual void SetEffectMode(EffectMode /*unused*/) {}
	virtual auto IsEffectModeSupported(EffectMode effect) -> bool
	{
		return effect == EffectMode_Normal;
	}

	[[nodiscard]] virtual auto SupportsRenderToTexture() const -> bool
	{
		return false;
	}
	[[nodiscard]] virtual auto SupportsFullscreenBorderlessWindow() const
	  -> bool
	{
		return false;
	}

	/* Create a render target, returning a texture handle. In addition to normal
	 * texture functions, this can be passed to SetRenderTarget. Delete with
	 * DeleteTexture. (UpdateTexture is not permitted.) Returns 0 if render-to-
	 * texture is unsupported.
	 */
	virtual auto CreateRenderTarget(const RenderTargetParam& /*unused*/,
									int& /* iTextureWidthOut */,
									int &
									/* iTextureHeightOut */) -> intptr_t
	{
		return 0;
	}

	virtual auto GetRenderTarget() -> intptr_t { return 0; }

	/* Set the render target, or 0 to resume rendering to the framebuffer. An
	 * active render target may not be used as a texture. If bPreserveTexture is
	 * true, the contents of the texture will be preserved from the previous
	 * call; otherwise, cleared.  If bPreserveTexture is true the first time a
	 * render target is used, behave as if bPreserveTexture was false.
	 */
	virtual void SetRenderTarget(intptr_t /* iHandle */,
								 bool /* bPreserveTexture */ = true)
	{
	}

	[[nodiscard]] virtual auto IsZTestEnabled() const -> bool = 0;
	[[nodiscard]] virtual auto IsZWriteEnabled() const -> bool = 0;
	virtual void SetZWrite(bool) = 0;
	virtual void SetZTestMode(ZTestMode) = 0;
	virtual void SetZBias(float) = 0;
	virtual void ClearZBuffer() = 0;

	virtual void SetCullMode(CullMode mode) = 0;

	virtual void SetAlphaTest(bool b) = 0;

	virtual void SetMaterial(const RageColor& emissive,
							 const RageColor& ambient,
							 const RageColor& diffuse,
							 const RageColor& specular,
							 float shininess) = 0;

	virtual void SetLighting(bool b) = 0;
	virtual void SetLightOff(int index) = 0;
	virtual void SetLightDirectional(int index,
									 const RageColor& ambient,
									 const RageColor& diffuse,
									 const RageColor& specular,
									 const RageVector3& dir) = 0;

	virtual void SetSphereEnvironmentMapping(TextureUnit tu, bool b) = 0;
	virtual void SetCelShaded(int stage) = 0;

	virtual auto CreateCompiledGeometry() -> RageCompiledGeometry* = 0;
	virtual void DeleteCompiledGeometry(RageCompiledGeometry* p) = 0;

	void DrawQuads(const RageSpriteVertex v[], int iNumVerts);
	void DrawQuadStrip(const RageSpriteVertex v[], int iNumVerts);
	void DrawFan(const RageSpriteVertex v[], int iNumVerts);
	void DrawStrip(const RageSpriteVertex v[], int iNumVerts);
	void DrawTriangles(const RageSpriteVertex v[], int iNumVerts);
	void DrawCompiledGeometry(const RageCompiledGeometry* p,
							  int iMeshIndex,
							  const std::vector<msMesh>& vMeshes);
	void DrawLineStrip(const RageSpriteVertex v[],
					   int iNumVerts,
					   float LineWidth);
	void DrawSymmetricQuadStrip(const RageSpriteVertex v[], int iNumVerts);
	void DrawCircle(const RageSpriteVertex& v, float radius);

	void DrawQuad(const RageSpriteVertex v[])
	{
		DrawQuads(v, 4);
	} /* alias. upper-left, upper-right, lower-left, lower-right */

	// hacks for cell-shaded models
	virtual void SetPolygonMode(PolygonMode /*unused*/) {}
	virtual void SetLineWidth(float /*unused*/) {}

	enum GraphicsFileFormat
	{
		SAVE_LOSSLESS,			// bmp
		SAVE_LOSSLESS_SENSIBLE, // png
		SAVE_LOSSY_LOW_QUAL,	// jpg
		SAVE_LOSSY_HIGH_QUAL	// jpg
	};
	auto SaveScreenshot(const std::string& sPath, GraphicsFileFormat format)
	  -> bool;

	[[nodiscard]] virtual auto GetTextureDiagnostics(unsigned /* id */) const
	  -> std::string
	{
		return std::string();
	}
	virtual auto CreateScreenshot()
	  -> RageSurface* = 0; // allocates a surface.  Caller must delete it.
	virtual auto GetTexture(intptr_t /* iTexture */) -> RageSurface*
	{
		return nullptr;
	} // allocates a surface.  Caller must delete it.

  protected:
	virtual void DrawQuadsInternal(const RageSpriteVertex v[],
								   int iNumVerts) = 0;
	virtual void DrawQuadStripInternal(const RageSpriteVertex v[],
									   int iNumVerts) = 0;
	virtual void DrawFanInternal(const RageSpriteVertex v[], int iNumVerts) = 0;
	virtual void DrawStripInternal(const RageSpriteVertex v[],
								   int iNumVerts) = 0;
	virtual void DrawTrianglesInternal(const RageSpriteVertex v[],
									   int iNumVerts) = 0;
	virtual void DrawCompiledGeometryInternal(const RageCompiledGeometry* p,
											  int iMeshIndex) = 0;
	virtual void DrawLineStripInternal(const RageSpriteVertex v[],
									   int iNumVerts,
									   float LineWidth);
	virtual void DrawSymmetricQuadStripInternal(const RageSpriteVertex v[],
												int iNumVerts) = 0;
	virtual void DrawCircleInternal(const RageSpriteVertex& v, float radius);

	virtual auto IsD3DInternal() -> bool;

	// return std::string() if mode change was successful, an error message
	// otherwise. bNewDeviceOut is set true if a new device was created and
	// textures need to be reloaded.
	virtual auto TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut)
	  -> std::string = 0;

	void DrawPolyLine(const RageSpriteVertex& p1,
					  const RageSpriteVertex& p2,
					  float LineWidth);
	void DrawPolyLines(const RageSpriteVertex v[],
					   int iNumVerts,
					   float LineWidth);

	// Stuff in RageDisplay.cpp
	void SetDefaultRenderStates();

  public:
	// Statistics
	[[nodiscard]] auto IsPredictiveFrameLimit() const -> bool;
	[[nodiscard]] auto GetFPS() const -> int;
	[[nodiscard]] auto GetVPF() const -> int;
	[[nodiscard]] auto GetCumFPS() const -> int; // average FPS since last reset
	virtual void ResetStats();
	virtual void ProcessStatsOnFlip();
	[[nodiscard]] virtual auto GetStats() const -> std::string;
	void StatsAddVerts(int iNumVertsRendered);

	// World matrix stack functions.
	void PushMatrix();
	void PopMatrix();
	void Translate(float x, float y, float z);
	void TranslateWorld(float x, float y, float z);
	void Scale(float x, float y, float z);
	void RotateX(float deg);
	void RotateY(float deg);
	void RotateZ(float deg);
	void SkewX(float fAmount);
	void SkewY(float fAmount);
	void MultMatrix(const RageMatrix& f)
	{
		this->PostMultMatrix(f);
	} /* alias */
	void PostMultMatrix(const RageMatrix& f);
	void PreMultMatrix(const RageMatrix& f);
	void LoadIdentity();

	// Texture matrix functions
	void TexturePushMatrix();
	void TexturePopMatrix();
	void TextureTranslate(float x, float y);
	void TextureTranslate(const RageVector2& v)
	{
		this->TextureTranslate(v.x, v.y);
	}

	// Projection and View matrix stack functions.
	void CameraPushMatrix();
	void CameraPopMatrix();
	void LoadMenuPerspective(float fFOVDegrees,
							 float fWidth,
							 float fHeight,
							 float fVanishPointX,
							 float fVanishPointY);
	void LoadLookAt(float fov,
					const RageVector3& Eye,
					const RageVector3& At,
					const RageVector3& Up);

	// Centering matrix
	void CenteringPushMatrix();
	void CenteringPopMatrix();
	void ChangeCentering(int trans_x,
						 int trans_y,
						 int add_width,
						 int add_height);

	auto CreateSurfaceFromPixfmt(RagePixelFormat pixfmt,
								 void* pixels,
								 int width,
								 int height,
								 int pitch) -> RageSurface*;
	auto FindPixelFormat(int bpp,
						 unsigned Rmask,
						 unsigned Gmask,
						 unsigned Bmask,
						 unsigned Amask,
						 bool realtime = false) -> RagePixelFormat;

	// Lua
	void PushSelf(lua_State* L);

  protected:
	auto GetPerspectiveMatrix(float fovy, float aspect, float zNear, float zFar)
	  -> RageMatrix;

	// Different for D3D and OpenGL. Not sure why they're not compatible. -Chris
	virtual auto GetOrthoMatrix(float l,
								float r,
								float b,
								float t,
								float zn,
								float zf) -> RageMatrix;
	virtual auto GetFrustumMatrix(float l,
								  float r,
								  float b,
								  float t,
								  float zn,
								  float zf) -> RageMatrix;

	// Matrix that adjusts position and scale of image on the screen
	auto GetCenteringMatrix(float fTranslateX,
							float fTranslateY,
							float fAddWidth,
							float fAddHeight) -> RageMatrix;
	void UpdateCentering();

	// Called by the RageDisplay derivitives
	[[nodiscard]] auto GetCentering() const -> const RageMatrix*;
	[[nodiscard]] auto GetProjectionTop() const -> const RageMatrix*;
	[[nodiscard]] auto GetViewTop() const -> const RageMatrix*;
	[[nodiscard]] auto GetWorldTop() const -> const RageMatrix*;
	[[nodiscard]] auto GetTextureTop() const -> const RageMatrix*;

	void FrameLimitBeforeVsync();
	void FrameLimitAfterVsync(int iFPS);
};

extern RageDisplay*
  DISPLAY; // global and accessible from anywhere in our program

#endif
