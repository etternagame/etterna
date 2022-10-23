#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/DisplaySpec.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Misc/Preference.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageDisplay.h"
#include "RageUtil/File/RageFile.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Misc/RageMath.h"
#include "RageSurface.h"
#include "RageSurfaceUtils_Zoom.h"
#include "RageSurface_Save_BMP.h"
#include "RageSurface_Save_JPEG.h"
#include "RageSurface_Save_PNG.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Screen/Others/Screen.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Globals/GameLoop.h"

#include <chrono>
#include <thread>

#ifdef _WIN32
#include "archutils/Win32/GraphicsWindow.h"
#endif

// Statistics stuff
auto g_LastCheckTimer = std::chrono::steady_clock::now();
int g_iNumVerts;
int g_iFPS, g_iVPF, g_iCFPS;

int
RageDisplay::GetFPS() const
{
	return g_iFPS;
}
int
RageDisplay::GetVPF() const
{
	return g_iVPF;
}
int
RageDisplay::GetCumFPS() const
{
	return g_iCFPS;
}

static int g_iFramesRenderedSinceLastCheck, g_iFramesRenderedSinceLastReset,
  g_iVertsRenderedSinceLastCheck, g_iNumChecksSinceLastReset;
static auto g_LastFrameEndedAt = std::chrono::steady_clock::now();
static std::chrono::nanoseconds g_LastFrameDuration =
  std::chrono::duration<int64_t>();
static std::chrono::nanoseconds g_FrameCorrection =
  std::chrono::duration<int64_t>(0);
static auto g_FrameRenderTime = std::chrono::steady_clock::now();
static std::chrono::nanoseconds g_LastFrameRenderTime;
static std::chrono::nanoseconds g_LastFramePresentTime;

struct Centering
{
	explicit Centering(int iTranslateX = 0,
					   int iTranslateY = 0,
					   int iAddWidth = 0,
					   int iAddHeight = 0)
	  : m_iTranslateX(iTranslateX)
	  , m_iTranslateY(iTranslateY)
	  , m_iAddWidth(iAddWidth)
	  , m_iAddHeight(iAddHeight)
	{
	}

	int m_iTranslateX, m_iTranslateY, m_iAddWidth, m_iAddHeight;
};

static std::vector<Centering> g_CenteringStack(1, Centering(0, 0, 0, 0));

RageDisplay* DISPLAY =
  nullptr; // global and accessible from anywhere in our program

Preference<bool> LOG_FPS("LogFPS", false);
Preference<float> g_fFrameLimitPercent("FrameLimitPercent", 0.90f);
Preference<int> g_fFrameLimit("FrameLimit", 1000);
Preference<int> g_fFrameLimitGameplay("FrameLimitGameplay", 1000);
Preference<bool> g_fPredictiveFrameLimit("PredictiveFrameLimit", 0);

bool
RageDisplay::IsPredictiveFrameLimit() const
{
	return g_fPredictiveFrameLimit;
}

static const char* RagePixelFormatNames[] = {
	"RGBA8", "BGRA8", "RGBA4", "RGB5A1", "RGB5",
	"RGB8",	 "PAL",	  "BGR8",  "A1BGR5", "X1RGB5",
};
XToString(RagePixelFormat);

/* bNeedReloadTextures is set to true if the device was re-created and we need
 * to reload textures.  On failure, an error message is returned.
 * XXX: the renderer itself should probably be the one to try fallback modes */
static LocalizedString SETVIDEOMODE_FAILED("RageDisplay",
										   "SetVideoMode failed:");
std::string
RageDisplay::SetVideoMode(VideoModeParams&& p, bool& bNeedReloadTextures)
{
	std::string err;
	std::vector<std::string> vs;

	if ((err = this->TryVideoMode(p, bNeedReloadTextures)).empty())
		return std::string();
	Locator::getLogger()->error("TryVideoMode failed: {}", err.c_str());
	vs.push_back(err);

	// fall back to settings that will most likely work
	p.bpp = 16;
	if ((err = this->TryVideoMode(p, bNeedReloadTextures)).empty())
		return std::string();
	vs.push_back(err);

	// "Intel(R) 82810E Graphics Controller" won't accept a 16 bpp surface if
	// the desktop is 32 bpp, so try 32 bpp as well.
	p.bpp = 32;
	if ((err = this->TryVideoMode(p, bNeedReloadTextures)).empty())
		return std::string();
	vs.push_back(err);

	// Fall back on a known resolution good rather than 640 x 480.
	DisplaySpecs dr;
	this->GetDisplaySpecs(dr);
	if (dr.empty()) {
		vs.push_back("No display resolutions");
		return SETVIDEOMODE_FAILED.GetValue() + " " + join(";", vs);
	}

	auto d = *dr.begin();
	// Try to find DisplaySpec corresponding to requested display
	for (const auto& candidate : dr) {
		if (candidate.currentMode() != nullptr) {
			d = candidate;
			if (candidate.id() == p.sDisplayId) {
				break;
			}
		}
	}

	p.sDisplayId = d.id();
	const auto supported = d.currentMode() != nullptr
							 ? *d.currentMode()
							 : *d.supportedModes().begin();
	p.width = supported.width;
	p.height = supported.height;
	p.rate = static_cast<int>(round(supported.refreshRate));
	if ((err = this->TryVideoMode(p, bNeedReloadTextures)).empty())
		return std::string();
	vs.push_back(err);

	return SETVIDEOMODE_FAILED.GetValue() + " " + join(";", vs);
}

void
RageDisplay::ProcessStatsOnFlip()
{
	if (PREFSMAN->m_bShowStats || LOG_FPS) {
		g_iFramesRenderedSinceLastCheck++;
		g_iFramesRenderedSinceLastReset++;

		const std::chrono::duration<double> timeDelta =
		  std::chrono::steady_clock::now() - g_LastCheckTimer;
		const auto checkTime = timeDelta.count();
		if (checkTime >= 1.0) // update stats every 1 sec.
		{
			g_LastCheckTimer = std::chrono::steady_clock::now();
			g_iNumChecksSinceLastReset++;
			g_iFPS = static_cast<int>(
			  g_iFramesRenderedSinceLastCheck / checkTime + 0.5);
			g_iCFPS =
			  g_iFramesRenderedSinceLastReset / g_iNumChecksSinceLastReset;
			g_iCFPS = static_cast<int>(g_iCFPS / checkTime + 0.5);
			g_iVPF =
			  g_iVertsRenderedSinceLastCheck / g_iFramesRenderedSinceLastCheck;
			g_iFramesRenderedSinceLastCheck = g_iVertsRenderedSinceLastCheck =
			  0;
			if (LOG_FPS) {
				auto sStats = GetStats();
				s_replace(sStats, "\n", ", ");
				Locator::getLogger()->debug("{}", sStats.c_str());
			}
		}
	}
}

void
RageDisplay::ResetStats()
{
	if (PREFSMAN->m_bShowStats || LOG_FPS) {
		g_iFPS = g_iVPF = 0;
		g_iFramesRenderedSinceLastCheck = g_iFramesRenderedSinceLastReset = 0;
		g_iNumChecksSinceLastReset = 0;
		g_iVertsRenderedSinceLastCheck = 0;
		g_LastCheckTimer = std::chrono::steady_clock::now();
	}
}

std::string
RageDisplay::GetStats() const
{
	std::string s;
	// If FPS == 0, we don't have stats yet.
	if (!GetFPS())
		s = "-- FPS\n-- av FPS\n-- VPF";

	s = ssprintf("%i FPS\n%i av FPS\n%i VPF", GetFPS(), GetCumFPS(), GetVPF());

	//	#ifdef _WIN32
	s += "\n" + this->GetApiDescription();
	//	#endif

	return s;
}

bool
RageDisplay::BeginFrame()
{
	this->SetDefaultRenderStates();

	return true;
}

void
RageDisplay::EndFrame()
{
	ProcessStatsOnFlip();
}

void
RageDisplay::BeginConcurrentRendering()
{
	this->SetDefaultRenderStates();
}

void
RageDisplay::StatsAddVerts(int iNumVertsRendered)
{
	g_iVertsRenderedSinceLastCheck += iNumVertsRendered;
}

/* Draw a line as a quad.  GL_LINES with SmoothLines off can draw line
 * ends at odd angles--they're forced to axis-alignment regardless of the
 * angle of the line. */
void
RageDisplay::DrawPolyLine(const RageSpriteVertex& p1,
						  const RageSpriteVertex& p2,
						  float LineWidth)
{
	// soh cah toa strikes strikes again!
	const auto opp = p2.p.x - p1.p.x;
	const auto adj = p2.p.y - p1.p.y;
	const auto hyp = powf(opp * opp + adj * adj, 0.5f);

	const auto lsin = opp / hyp;
	const auto lcos = adj / hyp;

	RageSpriteVertex v[4];

	v[0] = v[1] = p1;
	v[2] = v[3] = p2;

	const auto ydist = lsin * LineWidth / 2;
	const auto xdist = lcos * LineWidth / 2;

	v[0].p.x += xdist;
	v[0].p.y -= ydist;
	v[1].p.x -= xdist;
	v[1].p.y += ydist;
	v[2].p.x -= xdist;
	v[2].p.y += ydist;
	v[3].p.x += xdist;
	v[3].p.y -= ydist;

	this->DrawQuad(v);
}

// Batching version of the above function
void
RageDisplay::DrawPolyLines(const RageSpriteVertex v[],
						   int iNumVerts,
						   float LineWidth)
{
	std::vector<RageSpriteVertex> batchVerts;
	batchVerts.reserve(iNumVerts * 4);

	for (auto i = 0; i < iNumVerts - 1; ++i) {
		const auto p1 = v[i];
		const auto p2 = v[i + 1];

		// soh cah toa strikes strikes again!
		const auto opp = p2.p.x - p1.p.x;
		const auto adj = p2.p.y - p1.p.y;
		const auto hyp = powf(opp * opp + adj * adj, 0.5f);

		const auto lsin = opp / hyp;
		const auto lcos = adj / hyp;

		RageSpriteVertex nv[4];

		nv[0] = nv[1] = p1;
		nv[2] = nv[3] = p2;

		const auto ydist = lsin * LineWidth / 2;
		const auto xdist = lcos * LineWidth / 2;

		nv[0].p.x += xdist;
		nv[0].p.y -= ydist;
		nv[1].p.x -= xdist;
		nv[1].p.y += ydist;
		nv[2].p.x -= xdist;
		nv[2].p.y += ydist;
		nv[3].p.x += xdist;
		nv[3].p.y -= ydist;

		for (auto j = 0; j < 4; j++) {
			batchVerts.push_back(nv[j]);
		}
	}

	this->DrawQuads(batchVerts.data(), batchVerts.size());
}

void
RageDisplay::DrawLineStripInternal(const RageSpriteVertex v[],
								   int iNumVerts,
								   float LineWidth)
{
	ASSERT(iNumVerts >= 2);

	/* Draw a line strip with rounded corners using polys. This is used on
	 * cards that have strange allergic reactions to antialiased points and
	 * lines. */
	DrawPolyLines(v, iNumVerts, LineWidth);

	// Join the lines with circles so we get rounded corners when SmoothLines is
	// off.
	if (!PREFSMAN->m_bSmoothLines) {
		for (auto i = 0; i < iNumVerts; ++i)
			DrawCircle(v[i], LineWidth / 2);
	}
}

void
RageDisplay::DrawCircleInternal(const RageSpriteVertex& p, float radius)
{
	const auto subdivisions = 32;
	RageSpriteVertex v[subdivisions + 2];
	v[0] = p;

	for (auto i = 0; i < subdivisions + 1; ++i) {
		const auto fRotation = static_cast<float>(i) / subdivisions * 2 * PI;
		const auto fX = RageFastCos(fRotation) * radius;
		const auto fY = -RageFastSin(fRotation) * radius;
		v[1 + i] = v[0];
		v[1 + i].p.x += fX;
		v[1 + i].p.y += fY;
	}

	this->DrawFan(v, subdivisions + 2);
}

void
RageDisplay::SetDefaultRenderStates()
{
	SetLighting(false);
	SetCullMode(CULL_NONE);
	SetZWrite(false);
	SetZTestMode(ZTEST_OFF);
	SetAlphaTest(true);
	SetBlendMode(BLEND_NORMAL);
	SetTextureFiltering(TextureUnit_1, true);
	SetZBias(0);
	LoadMenuPerspective(0, 640, 480, 320, 240); // 0 FOV = ortho
}

bool
RageDisplay::IsD3DInternal()
{
	return false;
}

// Matrix stuff
class MatrixStack
{
	std::vector<RageMatrix> stack;

  public:
	MatrixStack()
	  : stack()
	{
		stack.resize(1);
		LoadIdentity();
	}

	// Pops the top of the stack.
	void Pop()
	{
		stack.pop_back();
		ASSERT(!stack.empty()); // underflow
	}

	// Pushes the stack by one, duplicating the current matrix.
	void Push()
	{
		stack.push_back(stack.back());
		ASSERT(stack.size() < 100); // overflow
	}

	// Loads identity in the current matrix.
	void LoadIdentity() { RageMatrixIdentity(&stack.back()); }

	// Loads the given matrix into the current matrix
	void LoadMatrix(const RageMatrix& m) { stack.back() = m; }

	// Right-Multiplies the given matrix to the current matrix.
	// (transformation is about the current world origin)
	void MultMatrix(const RageMatrix& m)
	{
		RageMatrixMultiply(&stack.back(), &m, &stack.back());
	}

	// Left-Multiplies the given matrix to the current matrix
	// (transformation is about the local origin of the object)
	void MultMatrixLocal(const RageMatrix& m)
	{
		RageMatrixMultiply(&stack.back(), &stack.back(), &m);
	}

	// Right multiply the current matrix with the computed rotation
	// matrix, counterclockwise about the given axis with the given angle.
	// (rotation is about the current world origin)
	void RotateX(float degrees)
	{
		RageMatrix m;
		RageMatrixRotationX(&m, degrees);
		MultMatrix(m);
	}
	void RotateY(float degrees)
	{
		RageMatrix m;
		RageMatrixRotationY(&m, degrees);
		MultMatrix(m);
	}
	void RotateZ(float degrees)
	{
		RageMatrix m;
		RageMatrixRotationZ(&m, degrees);
		MultMatrix(m);
	}

	// Left multiply the current matrix with the computed rotation
	// matrix. All angles are counterclockwise. (rotation is about the
	// local origin of the object)
	void RotateXLocal(float degrees)
	{
		RageMatrix m;
		RageMatrixRotationX(&m, degrees);
		MultMatrixLocal(m);
	}
	void RotateYLocal(float degrees)
	{
		RageMatrix m;
		RageMatrixRotationY(&m, degrees);
		MultMatrixLocal(m);
	}
	void RotateZLocal(float degrees)
	{
		RageMatrix m;
		RageMatrixRotationZ(&m, degrees);
		MultMatrixLocal(m);
	}

	// Right multiply the current matrix with the computed scale
	// matrix. (transformation is about the current world origin)
	void Scale(float x, float y, float z)
	{
		RageMatrix m;
		RageMatrixScaling(&m, x, y, z);
		MultMatrix(m);
	}

	// Left multiply the current matrix with the computed scale
	// matrix. (transformation is about the local origin of the object)
	void ScaleLocal(float x, float y, float z)
	{
		RageMatrix m;
		RageMatrixScaling(&m, x, y, z);
		MultMatrixLocal(m);
	}

	// Right multiply the current matrix with the computed translation
	// matrix. (transformation is about the current world origin)
	void Translate(float x, float y, float z)
	{
		RageMatrix m;
		RageMatrixTranslation(&m, x, y, z);
		MultMatrix(m);
	}

	// Left multiply the current matrix with the computed translation
	// matrix. (transformation is about the local origin of the object)
	void TranslateLocal(float x, float y, float z)
	{
		RageMatrix m;
		RageMatrixTranslation(&m, x, y, z);
		MultMatrixLocal(m);
	}

	void SkewX(float fAmount)
	{
		RageMatrix m;
		RageMatrixSkewX(&m, fAmount);
		MultMatrixLocal(m);
	}

	void SkewY(float fAmount)
	{
		RageMatrix m;
		RageMatrixSkewY(&m, fAmount);
		MultMatrixLocal(m);
	}

	// Obtain the current matrix at the top of the stack
	const RageMatrix* GetTop() const { return &stack.back(); }
	void SetTop(const RageMatrix& m) { stack.back() = m; }
};

static RageMatrix g_CenteringMatrix;
static MatrixStack g_ProjectionStack;
static MatrixStack g_ViewStack;
static MatrixStack g_WorldStack;
static MatrixStack g_TextureStack;

RageDisplay::RageDisplay()
{
	RageMatrixIdentity(&g_CenteringMatrix);
	g_ProjectionStack = MatrixStack();
	g_ViewStack = MatrixStack();
	g_WorldStack = MatrixStack();
	g_TextureStack = MatrixStack();

	// Register with Lua.
	{
		auto L = LUA->Get();
		lua_pushstring(L, "DISPLAY");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

RageDisplay::~RageDisplay()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("DISPLAY");
}

const RageMatrix*
RageDisplay::GetCentering() const
{
	return &g_CenteringMatrix;
}

const RageMatrix*
RageDisplay::GetProjectionTop() const
{
	return g_ProjectionStack.GetTop();
}

const RageMatrix*
RageDisplay::GetViewTop() const
{
	return g_ViewStack.GetTop();
}

const RageMatrix*
RageDisplay::GetWorldTop() const
{
	return g_WorldStack.GetTop();
}

const RageMatrix*
RageDisplay::GetTextureTop() const
{
	return g_TextureStack.GetTop();
}

void
RageDisplay::PushMatrix()
{
	g_WorldStack.Push();
}

void
RageDisplay::PopMatrix()
{
	g_WorldStack.Pop();
}

void
RageDisplay::Translate(float x, float y, float z)
{
	g_WorldStack.TranslateLocal(x, y, z);
}

void
RageDisplay::TranslateWorld(float x, float y, float z)
{
	g_WorldStack.Translate(x, y, z);
}

void
RageDisplay::Scale(float x, float y, float z)
{
	g_WorldStack.ScaleLocal(x, y, z);
}

void
RageDisplay::RotateX(float deg)
{
	g_WorldStack.RotateXLocal(deg);
}

void
RageDisplay::RotateY(float deg)
{
	g_WorldStack.RotateYLocal(deg);
}

void
RageDisplay::RotateZ(float deg)
{
	g_WorldStack.RotateZLocal(deg);
}

void
RageDisplay::SkewX(float fAmount)
{
	g_WorldStack.SkewX(fAmount);
}

void
RageDisplay::SkewY(float fAmount)
{
	g_WorldStack.SkewY(fAmount);
}

void
RageDisplay::PostMultMatrix(const RageMatrix& m)
{
	g_WorldStack.MultMatrix(m);
}

void
RageDisplay::PreMultMatrix(const RageMatrix& m)
{
	g_WorldStack.MultMatrixLocal(m);
}

void
RageDisplay::LoadIdentity()
{
	g_WorldStack.LoadIdentity();
}

void
RageDisplay::TexturePushMatrix()
{
	g_TextureStack.Push();
}

void
RageDisplay::TexturePopMatrix()
{
	g_TextureStack.Pop();
}

void
RageDisplay::TextureTranslate(float x, float y)
{
	g_TextureStack.TranslateLocal(x, y, 0);
}

void
RageDisplay::LoadMenuPerspective(float fovDegrees,
								 float fWidth,
								 float fHeight,
								 float fVanishPointX,
								 float fVanishPointY)
{
	// fovDegrees == 0 gives ortho projection.
	if (fovDegrees == 0) {
		const float left = 0, right = fWidth, bottom = fHeight, top = 0;
		g_ProjectionStack.LoadMatrix(
		  GetOrthoMatrix(left, right, bottom, top, -1000, +1000));
		g_ViewStack.LoadIdentity();
	} else {
		CLAMP(fovDegrees, 0.1f, 179.9f);
		const auto fovRadians = fovDegrees / 180.f * PI;
		const auto theta = fovRadians / 2;
		const auto fDistCameraFromImage = fWidth / 2 / tanf(theta);

		fVanishPointX = SCALE(fVanishPointX, 0, fWidth, fWidth, 0);
		fVanishPointY = SCALE(fVanishPointY, 0, fHeight, fHeight, 0);

		fVanishPointX -= fWidth / 2;
		fVanishPointY -= fHeight / 2;

		// It's the caller's responsibility to push first.
		g_ProjectionStack.LoadMatrix(
		  GetFrustumMatrix((fVanishPointX - fWidth / 2) / fDistCameraFromImage,
						   (fVanishPointX + fWidth / 2) / fDistCameraFromImage,
						   (fVanishPointY + fHeight / 2) / fDistCameraFromImage,
						   (fVanishPointY - fHeight / 2) / fDistCameraFromImage,
						   1,
						   fDistCameraFromImage + 1000));

		g_ViewStack.LoadMatrix(RageLookAt(-fVanishPointX + fWidth / 2,
										  -fVanishPointY + fHeight / 2,
										  fDistCameraFromImage,
										  -fVanishPointX + fWidth / 2,
										  -fVanishPointY + fHeight / 2,
										  0,
										  0.0f,
										  1.0f,
										  0.0f));
	}
}

void
RageDisplay::CameraPushMatrix()
{
	g_ProjectionStack.Push();
	g_ViewStack.Push();
}

void
RageDisplay::CameraPopMatrix()
{
	g_ProjectionStack.Pop();
	g_ViewStack.Pop();
}

/* gluLookAt. The result is pre-multiplied to the matrix (M = L * M) instead of
 * post-multiplied. */
void
RageDisplay::LoadLookAt(float fFOV,
						const RageVector3& Eye,
						const RageVector3& At,
						const RageVector3& Up)
{
	const auto fAspect = (*GetActualVideoModeParams()).fDisplayAspectRatio;
	g_ProjectionStack.LoadMatrix(GetPerspectiveMatrix(fFOV, fAspect, 1, 1000));

	// Flip the Y coordinate, so positive numbers go down.
	g_ProjectionStack.Scale(1, -1, 1);

	g_ViewStack.LoadMatrix(
	  RageLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z));
}

RageMatrix
RageDisplay::GetPerspectiveMatrix(float fovy,
								  float aspect,
								  float zNear,
								  float zFar)
{
	const auto ymax = zNear * tanf(fovy * PI / 360.0f);
	const auto ymin = -ymax;
	const auto xmin = ymin * aspect;
	const auto xmax = ymax * aspect;

	return GetFrustumMatrix(xmin, xmax, ymin, ymax, zNear, zFar);
}

RageSurface*
RageDisplay::CreateSurfaceFromPixfmt(RagePixelFormat pixfmt,
									 void* pixels,
									 int width,
									 int height,
									 int pitch)
{
	auto tpf = GetPixelFormatDesc(pixfmt);

	const auto surf = CreateSurfaceFrom(width,
										height,
										tpf->bpp,
										tpf->masks[0],
										tpf->masks[1],
										tpf->masks[2],
										tpf->masks[3],
										static_cast<uint8_t*>(pixels),
										pitch);

	return surf;
}

RagePixelFormat
RageDisplay::FindPixelFormat(int iBPP,
							 unsigned iRmask,
							 unsigned iGmask,
							 unsigned iBmask,
							 unsigned iAmask,
							 bool bRealtime)
{
	RagePixelFormatDesc tmp = { iBPP, { iRmask, iGmask, iBmask, iAmask } };

	FOREACH_ENUM(RagePixelFormat, iPixFmt)
	{
		const auto pf = GetPixelFormatDesc(RagePixelFormat(iPixFmt));
		if (!SupportsTextureFormat(RagePixelFormat(iPixFmt), bRealtime))
			continue;

		if (memcmp(pf, &tmp, sizeof(tmp)))
			continue;
		return iPixFmt;
	}

	return RagePixelFormat_Invalid;
}

/* These convert to OpenGL's coordinate system: -1,-1 is the bottom-left,
 * +1,+1 is the top-right, and Z goes from -1 (viewer) to +1 (distance).
 * It's a little odd, but very well-defined. */
RageMatrix
RageDisplay::GetOrthoMatrix(float l,
							float r,
							float b,
							float t,
							float zn,
							float zf)
{
	RageMatrix m(2 / (r - l),
				 0,
				 0,
				 0,
				 0,
				 2 / (t - b),
				 0,
				 0,
				 0,
				 0,
				 -2 / (zf - zn),
				 0,
				 -(r + l) / (r - l),
				 -(t + b) / (t - b),
				 -(zf + zn) / (zf - zn),
				 1);
	return m;
}

RageMatrix
RageDisplay::GetFrustumMatrix(float l,
							  float r,
							  float b,
							  float t,
							  float zn,
							  float zf)
{
	// glFrustum
	const auto A = (r + l) / (r - l);
	const auto B = (t + b) / (t - b);
	const auto C = -1 * (zf + zn) / (zf - zn);
	const auto D = -1 * (2 * zf * zn) / (zf - zn);
	RageMatrix m(2 * zn / (r - l),
				 0,
				 0,
				 0,
				 0,
				 2 * zn / (t - b),
				 0,
				 0,
				 A,
				 B,
				 C,
				 -1,
				 0,
				 0,
				 D,
				 0);
	return m;
}

void
RageDisplay::ResolutionChanged()
{
	// The centering matrix depends on the resolution.
	UpdateCentering();
}

void
RageDisplay::CenteringPushMatrix()
{
	g_CenteringStack.push_back(g_CenteringStack.back());
	ASSERT(g_CenteringStack.size() < 100); // overflow
}

void
RageDisplay::CenteringPopMatrix()
{
	g_CenteringStack.pop_back();
	ASSERT(!g_CenteringStack.empty()); // underflow
	UpdateCentering();
}

void
RageDisplay::ChangeCentering(int iTranslateX,
							 int iTranslateY,
							 int iAddWidth,
							 int iAddHeight)
{
	g_CenteringStack.back() =
	  Centering(iTranslateX, iTranslateY, iAddWidth, iAddHeight);

	UpdateCentering();
}

RageMatrix
RageDisplay::GetCenteringMatrix(float fTranslateX,
								float fTranslateY,
								float fAddWidth,
								float fAddHeight)
{
	// in screen space, left edge = -1, right edge = 1, bottom edge = -1. top
	// edge = 1
	const auto fWidth =
	  static_cast<float>((*GetActualVideoModeParams()).windowWidth);
	const auto fHeight =
	  static_cast<float>((*GetActualVideoModeParams()).windowHeight);
	const auto fPercentShiftX = SCALE(fTranslateX, 0, fWidth, 0, +2.0f);
	const auto fPercentShiftY = SCALE(fTranslateY, 0, fHeight, 0, -2.0f);
	const auto fPercentScaleX = SCALE(fAddWidth, 0, fWidth, 1.0f, 2.0f);
	const auto fPercentScaleY = SCALE(fAddHeight, 0, fHeight, 1.0f, 2.0f);

	RageMatrix m1;
	RageMatrix m2;
	RageMatrixTranslation(&m1, fPercentShiftX, fPercentShiftY, 0);
	RageMatrixScaling(&m2, fPercentScaleX, fPercentScaleY, 1);
	RageMatrix mOut;
	RageMatrixMultiply(&mOut, &m1, &m2);
	return mOut;
}

void
RageDisplay::UpdateCentering()
{
	const auto& p = g_CenteringStack.back();
	g_CenteringMatrix = GetCenteringMatrix(static_cast<float>(p.m_iTranslateX),
										   static_cast<float>(p.m_iTranslateY),
										   static_cast<float>(p.m_iAddWidth),
										   static_cast<float>(p.m_iAddHeight));
}

bool
RageDisplay::SaveScreenshot(const std::string& sPath, GraphicsFileFormat format)
{
	auto surface = this->CreateScreenshot();
	/* Unless we're in lossless, resize the image to 640x480.  If we're saving
	 * lossy, there's no sense in saving 1280x960 screenshots, and we don't want
	 * to output screenshots in a strange (non-1) sample aspect ratio. */
	if (format != SAVE_LOSSLESS && format != SAVE_LOSSLESS_SENSIBLE) {
		// Maintain the DAR.
		ASSERT((*GetActualVideoModeParams()).fDisplayAspectRatio > 0);
		const auto iHeight = 480;
		// This used to be lrintf. However, lrintf causes odd resolutions like
		// 639x480 (4:3) and 853x480 (16:9). ceilf gives correct values. -aj
		const auto iWidth = static_cast<int>(
		  ceilf(iHeight * (*GetActualVideoModeParams()).fDisplayAspectRatio));
		RageSurfaceUtils::Zoom(surface, iWidth, iHeight);
	}

	RageFile out;
	if (!out.Open(sPath, RageFile::WRITE)) {
		Locator::getLogger()->warn("Couldn't write {}: {}", sPath.c_str(), out.GetError().c_str());
		SAFE_DELETE(surface);
		return false;
	}

	auto bSuccess = false;
	std::string strError = "";
	switch (format) {
		case SAVE_LOSSLESS:
			bSuccess = RageSurfaceUtils::SaveBMP(surface, out);
			break;
		case SAVE_LOSSLESS_SENSIBLE:
			bSuccess = RageSurfaceUtils::SavePNG(surface, out, strError);
			break;
		case SAVE_LOSSY_LOW_QUAL:
			bSuccess = RageSurfaceUtils::SaveJPEG(surface, out, false);
			break;
		case SAVE_LOSSY_HIGH_QUAL:
			bSuccess = RageSurfaceUtils::SaveJPEG(surface, out, true);
			break;
			DEFAULT_FAIL(format);
	}

	SAFE_DELETE(surface);

	if (!bSuccess) {
		Locator::getLogger()->warn(
		  "Couldn't write {}: {}", sPath.c_str(), out.GetError().c_str());
		return false;
	}

	return true;
}

void
RageDisplay::DrawQuads(const RageSpriteVertex v[], int iNumVerts)
{
	ASSERT((iNumVerts % 4) == 0);

	if (iNumVerts == 0)
		return;

	this->DrawQuadsInternal(v, iNumVerts);

	StatsAddVerts(iNumVerts);
}

void
RageDisplay::DrawQuadStrip(const RageSpriteVertex v[], int iNumVerts)
{
	ASSERT((iNumVerts % 2) == 0);

	if (iNumVerts < 4)
		return;

	this->DrawQuadStripInternal(v, iNumVerts);

	StatsAddVerts(iNumVerts);
}

void
RageDisplay::DrawFan(const RageSpriteVertex v[], int iNumVerts)
{
	ASSERT(iNumVerts >= 3);

	this->DrawFanInternal(v, iNumVerts);

	StatsAddVerts(iNumVerts);
}

void
RageDisplay::DrawStrip(const RageSpriteVertex v[], int iNumVerts)
{
	ASSERT(iNumVerts >= 3);

	this->DrawStripInternal(v, iNumVerts);

	StatsAddVerts(iNumVerts);
}

void
RageDisplay::DrawTriangles(const RageSpriteVertex v[], int iNumVerts)
{
	if (iNumVerts == 0)
		return;

	ASSERT(iNumVerts >= 3);

	this->DrawTrianglesInternal(v, iNumVerts);

	StatsAddVerts(iNumVerts);
}

void
RageDisplay::DrawCompiledGeometry(const RageCompiledGeometry* p,
								  int iMeshIndex,
								  const std::vector<msMesh>& vMeshes)
{
	this->DrawCompiledGeometryInternal(p, iMeshIndex);

	StatsAddVerts(static_cast<int>(vMeshes[iMeshIndex].Triangles.size()));
}

void
RageDisplay::DrawLineStrip(const RageSpriteVertex v[],
						   int iNumVerts,
						   float LineWidth)
{
	ASSERT(iNumVerts >= 2);

	this->DrawLineStripInternal(v, iNumVerts, LineWidth);
}

/*
 * Draw a strip of:
 *
 * 0..1..2
 * . /.\ .
 * ./ . \.
 * 3..4..5
 * . /.\ .
 * ./ . \.
 * 6..7..8
 */

void
RageDisplay::DrawSymmetricQuadStrip(const RageSpriteVertex v[], int iNumVerts)
{
	ASSERT(iNumVerts >= 3);

	if (iNumVerts < 6)
		return;

	this->DrawSymmetricQuadStripInternal(v, iNumVerts);

	StatsAddVerts(iNumVerts);
}

void
RageDisplay::DrawCircle(const RageSpriteVertex& v, float radius)
{
	this->DrawCircleInternal(v, radius);
}

float
RageDisplay::GetFrameTimingAdjustment(std::chrono::steady_clock::time_point now)
{
	/*
	 * We get one update per frame, and we're updated early, almost immediately
	 * after vsync, near the beginning of the game loop.  However, it's very
	 * likely that we'll lose the scheduler while waiting for vsync, and some
	 * other thread will be working.  Especially with a low-resolution scheduler
	 * (Linux 2.4, Win9x), we may not get the scheduler back immediately after
	 * the vsync; there may be up to a ~10ms delay.  This can cause jitter in
	 * the rendered arrows.
	 *
	 * Compensate.  If vsync is enabled, and we're maintaining the refresh rate
	 * consistently, we should have a very precise game loop interval.  If we
	 * have that, but we're off by a small amount (less than the interval),
	 * adjust the time to line it up.  As long as we adjust both the sound time
	 * and the timestamp, this won't adversely affect input timing. If we're off
	 * by more than that, we probably had a frame skip, in which case we have
	 * bigger skip problems, so don't adjust.
	 */

	if (GetActualVideoModeParams()->vsync == false) {
		return 0;
	}

	if (IsPredictiveFrameLimit()) {
		return 0;
	}

	const int iThisFPS = GetActualVideoModeParams()->rate;

	std::chrono::duration<float> dDelta = g_LastFrameDuration;
	std::chrono::duration<float> dTimeIntoFrame = now - g_LastFrameEndedAt;
	const float fExpectedDelay = 1.0f / iThisFPS;
	const float fDeltaTime = dDelta.count();
	const float fExtraDelay = fDeltaTime - fExpectedDelay;
	const float fAdjustTo = -dTimeIntoFrame.count();

	if (fabsf(fExtraDelay) >= fExpectedDelay / 2)
		return fAdjustTo;

	return fAdjustTo + std::min(-fExtraDelay, 0.0f);
}

static auto
targetFrameTime() -> double
{
	auto result = 0.0;
	if ((SCREENMAN != nullptr) && (SCREENMAN->GetTopScreen() != nullptr)) {
		auto inGameplay =
		  SCREENMAN->GetTopScreen()->GetScreenType() == gameplay;
		if (inGameplay && g_fFrameLimitGameplay.Get() > 0) {
			result = 1.0 / g_fFrameLimitGameplay.Get();
		} else if (!inGameplay && g_fFrameLimit.Get() > 0) {
			result = 1.0 / g_fFrameLimit.Get();
		}
	}
	return result;
}

void
RageDisplay::FrameLimitBeforeVsync()
{
	auto waitTime = targetFrameTime();
	if (g_fPredictiveFrameLimit.Get()) {
		const auto afterRender = std::chrono::steady_clock::now();
		const auto endTime = afterRender - g_FrameRenderTime;

		g_LastFrameRenderTime = endTime;
	} else if (!PREFSMAN->m_bVsync.Get() && waitTime > 0.0) {
		auto waitNanoseconds =
		  std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::duration<double>(waitTime));

		// Estimate how long to wait to meet our frame time.
		// Rationale: the following
		//
		//  auto t = g_LastFrameEndedAt + waitTime;
		//  while (t > now());
		//
		// is too tight and waits slightly too long (trivially, by at least
		// the amount of time required to exit the busy loop). In practice this
		// effect noticeably reduces measured FPS. To account for it, measure
		// the frame times we are actually getting and gradually adjust the
		// delay to bring it in line with the target frame time.
		auto error = g_LastFrameDuration - waitNanoseconds;
		if (error < std::chrono::milliseconds(1)) {
			g_FrameCorrection -= error / 64;
			CLAMP(g_FrameCorrection,
				std::chrono::milliseconds(-1),
				std::chrono::milliseconds(0));
		}

		auto estimatedTimeToWait = waitNanoseconds + g_FrameCorrection;
		auto t = g_LastFrameEndedAt + estimatedTimeToWait;
		while (t > std::chrono::steady_clock::now()) {
			std::this_thread::yield();
		}
	} else {
		// Ignore frame limit preferences if v-sync is enabled without
		// predictive frame limit.
	}
}

// Frame pacing code
// The aim of this function is to delay the start of the next loop as long as
// possible so that what the player sees when using VSync is as close to real
// time as possible.
//
// The issue:
// If we wait too long we miss the present and cause a stutter that displays
// information which could be over 1 frame old If we are too cautious then we
// lose out on latency we could remove with better frame pacing
void
RageDisplay::FrameLimitAfterVsync(int iFPS)
{
	const auto frameEndedAt = std::chrono::steady_clock::now();
	g_LastFrameDuration = frameEndedAt - g_LastFrameEndedAt;
	g_LastFrameEndedAt = frameEndedAt;

	if (!g_fPredictiveFrameLimit.Get()) {
		return;
	}

	auto waitTime = targetFrameTime();

	if (!PREFSMAN->m_bVsync.Get() && waitTime == 0.0) {
		return;
	}

	// Not using frame limiter and vsync is enabled
	// Or Frame limiter is set beyond vsync
	if (PREFSMAN->m_bVsync.Get() &&
		(waitTime == 0.0 || waitTime < (1.0 / iFPS)))
		waitTime = 1.0 / iFPS;

	// Calculate wait time and attempt to not overshoot waiting
	// Cautiousness is needed due to CPU power states and driver present times
	// causing entire loop time variations of around +-250 microseconds.

	// Conservative default of 10% target frame time is used incase someone is
	// using really old hardware
	const auto waitCautiousness =
	  g_fFrameLimitPercent.Get() > 0 ? g_fFrameLimitPercent.Get() : 0.90;

	// Target frame time
	waitTime *= waitCautiousness;
	const auto waitTimeNano = std::chrono::duration<double>(waitTime);
	const auto waitTimeActuallyNano =
	  std::chrono::duration_cast<std::chrono::nanoseconds>(waitTimeNano);

	// Last render time, DirectX and OpenGL do work in present time so we need
	// to include them
	const auto renderTime = g_LastFrameRenderTime + g_LastFramePresentTime;

	// ex. 8.33ms refresh rate - 1ms render time
	const auto startLoopTime = waitTimeActuallyNano - renderTime;

	// Check if we need to wait
	if (startLoopTime.count() < 0) {
		// Just go, we're late
		// LOG->Trace("Game loop start is late by %d", startLoopTime);
	} else {
		// LOG->Trace("Waiting before game loop by %d", startLoopTime);
		// Wait until our desired time, aka g_fFrameLimitPercent of the way
		// until needed present time
		std::this_thread::sleep_for(startLoopTime);
	}

	g_FrameRenderTime = std::chrono::steady_clock::now();
}

void
RageDisplay::SetPresentTime(std::chrono::nanoseconds presentTime)
{
	g_LastFramePresentTime = presentTime;
}

bool
RageDisplay::IsD3D()
{
	return DISPLAY->IsD3DInternal();
}

RageCompiledGeometry::~RageCompiledGeometry()
{
	m_bNeedsNormals = false;
}

void
RageCompiledGeometry::Set(const std::vector<msMesh>& vMeshes, bool bNeedsNormals)
{
	m_bNeedsNormals = bNeedsNormals;

	size_t totalVerts = 0;
	size_t totalTriangles = 0;

	m_bAnyNeedsTextureMatrixScale = false;

	m_vMeshInfo.resize(vMeshes.size());
	for (unsigned i = 0; i < vMeshes.size(); i++) {
		const auto& mesh = vMeshes[i];
		const auto& Vertices = mesh.Vertices;
		const auto& Triangles = mesh.Triangles;

		auto& meshInfo = m_vMeshInfo[i];
		meshInfo.m_bNeedsTextureMatrixScale = false;

		meshInfo.iVertexStart = static_cast<int>(totalVerts);
		meshInfo.iVertexCount = static_cast<int>(Vertices.size());
		meshInfo.iTriangleStart = static_cast<int>(totalTriangles);
		meshInfo.iTriangleCount = static_cast<int>(Triangles.size());

		totalVerts += Vertices.size();
		totalTriangles += Triangles.size();

		for (unsigned j = 0; j < Vertices.size(); ++j) {
			if (Vertices[j].TextureMatrixScale.x != 1.0f ||
				Vertices[j].TextureMatrixScale.y != 1.0f) {
				meshInfo.m_bNeedsTextureMatrixScale = true;
				m_bAnyNeedsTextureMatrixScale = true;
			}
		}
	}

	this->Allocate(vMeshes);

	Change(vMeshes);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

static void
register_REFRESH_DEFAULT(lua_State* L)
{
	lua_pushstring(L, "REFRESH_DEFAULT");
	lua_pushinteger(L, REFRESH_DEFAULT);
	lua_settable(L, LUA_GLOBALSINDEX);
}
REGISTER_WITH_LUA_FUNCTION(register_REFRESH_DEFAULT);

/** @brief Allow Lua to have access to the RageDisplay. */
class LunaRageDisplay : public Luna<RageDisplay>
{
  public:
	static int GetDisplayWidth(T* p, lua_State* L)
	{
		const VideoModeParams params = *p->GetActualVideoModeParams();
		LuaHelpers::Push(L, params.width);
		return 1;
	}

	static int GetDisplayHeight(T* p, lua_State* L)
	{
		const VideoModeParams params = *p->GetActualVideoModeParams();
		LuaHelpers::Push(L, params.height);
		return 1;
	}

	static int GetFPS(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetFPS());
		return 1;
	}

	static int GetVPF(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetVPF());
		return 1;
	}

	static int GetCumFPS(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetCumFPS());
		return 1;
	}

	static int GetDisplayRefreshRate(T* p, lua_State* L)
	{
		const auto params = p->GetActualVideoModeParams();
		lua_pushnumber(L, params->rate);
		return 1;
	}

	static int GetDisplaySpecs(T* p, lua_State* L)
	{
		DisplaySpecs s;
		p->GetDisplaySpecs(s);
		pushDisplaySpecs(L, s);
		return 1;
	}

	static int SupportsRenderToTexture(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->SupportsRenderToTexture());
		return 1;
	}

	static int SupportsFullscreenBorderlessWindow(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->SupportsFullscreenBorderlessWindow());
		return 1;
	}
	static int MoveWindow(T* p, lua_State* L)
	{
		auto success = false;
#ifdef _WIN32
		const auto x = IArg(1);
		const auto y = IArg(2);
		success = GraphicsWindow::PushWindow(x, y);
#endif
		lua_pushboolean(L, success);
		return 1;
	}

	LunaRageDisplay()
	{
		ADD_METHOD(GetDisplayWidth);
		ADD_METHOD(GetDisplayHeight);
		ADD_METHOD(GetDisplayRefreshRate);
		ADD_METHOD(GetFPS);
		ADD_METHOD(GetVPF);
		ADD_METHOD(GetCumFPS);
		ADD_METHOD(GetDisplaySpecs);
		ADD_METHOD(SupportsRenderToTexture);
		ADD_METHOD(SupportsFullscreenBorderlessWindow);
		ADD_METHOD(MoveWindow);
	}
};

LUA_REGISTER_CLASS(RageDisplay)
// lua end
