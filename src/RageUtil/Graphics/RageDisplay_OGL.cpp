#include "Etterna/Globals/global.h"
#include "RageDisplay_OGL.h"
#include "RageDisplay_OGL_Helpers.h"
#include "Etterna/Models/Misc/EnumHelper.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/File/RageFile.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Misc/RageMath.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageTextureManager.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Utils/RageUtil.h"

#include "arch/LowLevelWindow/LowLevelWindow.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <map>

#ifdef _WIN32
#include <GL/wglew.h>
#endif

#if defined(_MSC_VER)
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#endif

#ifdef NO_GL_FLUSH
#define glFlush()
#endif

using std::max;
using std::min;
using namespace RageDisplay_Legacy_Helpers;

//
// Globals
//

static bool g_bReversePackedPixelsWorks = true;
static bool g_bColorIndexTableWorks = true;

/* OpenGL system information that generally doesn't change at runtime. */

/* Range and granularity of points and lines: */
static float g_line_range[2];
static float g_point_range[2];

/* OpenGL version * 10: */
static int g_glVersion;

/* GLU version * 10: */
static int g_gluVersion;

static int g_iMaxTextureUnits = 0;

/* We don't actually use normals (we don't turn on lighting), there's just
 * no GL_T2F_C4F_V3F. */
/* If we support texture matrix scaling, a handle to the vertex program: */
static GLhandleARB g_bTextureMatrixShader = 0;

static std::map<intptr_t, RenderTarget*> g_mapRenderTargets;
static RenderTarget* g_pCurrentRenderTarget = nullptr;

static LowLevelWindow* g_pWind;

static bool g_bInvertY = false;

static void
InvalidateObjects();

static RageDisplay::RagePixelFormatDesc
  PIXEL_FORMAT_DESC[NUM_RagePixelFormat] = {
	  { /* R8G8B8A8 */
		32,
		{ 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF } },
	  { /* B8G8R8A8 */
		32,
		{ 0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF } },
	  {
		/* R4G4B4A4 */
		16,
		{ 0xF000, 0x0F00, 0x00F0, 0x000F },
	  },
	  {
		/* R5G5B5A1 */
		16,
		{ 0xF800, 0x07C0, 0x003E, 0x0001 },
	  },
	  {
		/* R5G5B5X1 */
		16,
		{ 0xF800, 0x07C0, 0x003E, 0x0000 },
	  },
	  { /* R8G8B8 */
		24,
		{ 0xFF0000, 0x00FF00, 0x0000FF, 0x000000 } },
	  {
		/* Paletted */
		8,
		{ 0, 0, 0, 0 } /* N/A */
	  },
	  { /* B8G8R8 */
		24,
		{ 0x0000FF, 0x00FF00, 0xFF0000, 0x000000 } },
	  {
		/* A1R5G5B5 */
		16,
		{ 0x7C00, 0x03E0, 0x001F, 0x8000 },
	  },
	  {
		/* X1R5G5B5 */
		16,
		{ 0x7C00, 0x03E0, 0x001F, 0x0000 },
	  }
  };

/* g_GLPixFmtInfo is used for both texture formats and surface formats.  For
 * example, it's fine to ask for a RagePixelFormat_RGB5 texture, but to supply a
 * surface matching RagePixelFormat_RGB8.  OpenGL will simply discard the extra
 * bits.
 *
 * It's possible for a format to be supported as a texture format but not as a
 * surface format.  For example, if packed pixels aren't supported, we can still
 * use GL_RGB5_A1, but we'll have to convert to a supported surface pixel format
 * first.  It's not ideal, since we'll convert to RGBA8 and OGL will convert
 * back, but it works fine.
 */
struct GLPixFmtInfo_t
{
	GLenum internalfmt; /* target format */
	GLenum format;		/* target format */
	GLenum type;		/* data format */
} const g_GLPixFmtInfo[NUM_RagePixelFormat] = {
	{
	  /* R8G8B8A8 */
	  GL_RGBA8,
	  GL_RGBA,
	  GL_UNSIGNED_BYTE,
	},
	{
	  /* R8G8B8A8 */
	  GL_RGBA8,
	  GL_BGRA,
	  GL_UNSIGNED_BYTE,
	},
	{
	  /* B4G4R4A4 */
	  GL_RGBA4,
	  GL_RGBA,
	  GL_UNSIGNED_SHORT_4_4_4_4,
	},
	{
	  /* B5G5R5A1 */
	  GL_RGB5_A1,
	  GL_RGBA,
	  GL_UNSIGNED_SHORT_5_5_5_1,
	},
	{
	  /* B5G5R5 */
	  GL_RGB5,
	  GL_RGBA,
	  GL_UNSIGNED_SHORT_5_5_5_1,
	},
	{
	  /* B8G8R8 */
	  GL_RGB8,
	  GL_RGB,
	  GL_UNSIGNED_BYTE,
	},
	{
	  /* Paletted */
	  GL_COLOR_INDEX8_EXT,
	  GL_COLOR_INDEX,
	  GL_UNSIGNED_BYTE,
	},
	{
	  /* B8G8R8 */
	  GL_RGB8,
	  GL_BGR,
	  GL_UNSIGNED_BYTE,
	},
	{
	  /* A1R5G5B5 (matches D3DFMT_A1R5G5B5) */
	  GL_RGB5_A1,
	  GL_BGRA,
	  GL_UNSIGNED_SHORT_1_5_5_5_REV,
	},
	{
	  /* X1R5G5B5 */
	  GL_RGB5,
	  GL_BGRA,
	  GL_UNSIGNED_SHORT_1_5_5_5_REV,
	}
};

static void
FixLittleEndian()
{
	static auto bInitialized = false;
	if (bInitialized)
		return;
	bInitialized = true;

	for (auto i = 0; i < NUM_RagePixelFormat; ++i) {
		auto& pf = PIXEL_FORMAT_DESC[i];

		/* OpenGL and RageSurface handle byte formats differently; we need
		 * to flip non-paletted masks to make them line up. */
		if (g_GLPixFmtInfo[i].type != GL_UNSIGNED_BYTE || pf.bpp == 8)
			continue;

		for (unsigned int& mask : pf.masks) {
			int m = mask;
			switch (pf.bpp) {
				case 24:
					m = Swap24(m);
					break;
				case 32:
					m = Swap32(m);
					break;
				default:
					FAIL_M(ssprintf("Unsupported BPP value: %i", pf.bpp));
			}
			mask = m;
		}
	}
}

static void
TurnOffHardwareVBO()
{
	if (GLEW_ARB_vertex_buffer_object) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
}

RageDisplay_Legacy::RageDisplay_Legacy()
{
	if (PREFSMAN->m_verbose_log > 1) {
		Locator::getLogger()->trace("RageDisplay_Legacy::RageDisplay_Legacy()");
		Locator::getLogger()->trace("Current renderer: OpenGL");
	}

	FixLittleEndian();
	RageDisplay_Legacy_Helpers::Init();

	g_pWind = nullptr;
	g_bTextureMatrixShader = 0;
}

std::string
GetInfoLog(GLhandleARB h)
{
	GLint iLength;
	glGetObjectParameterivARB(h, GL_OBJECT_INFO_LOG_LENGTH_ARB, &iLength);
	if (!iLength)
		return std::string();

	auto* pInfoLog = new GLcharARB[iLength];
	glGetInfoLogARB(h, iLength, &iLength, pInfoLog);
	std::string sRet = pInfoLog;
	delete[] pInfoLog;
	TrimRight(sRet);
	return sRet;
}

GLhandleARB
CompileShader(GLenum ShaderType,
			  std::string sFile,
			  vector<std::string> asDefines)
{
	/* XXX: This would not be necessary if it wasn't for the special case for
	 * Cel. */
	if (ShaderType == GL_FRAGMENT_SHADER_ARB &&
		!glewIsSupported("GL_VERSION_2_0")) {
		Locator::getLogger()->warn("Fragment shaders not supported by driver. Some effects will "
				  "not be available.");
		return 0;
	}

	std::string sBuffer;
	{
		RageFile file;
		if (!file.Open(sFile)) {
			Locator::getLogger()->warn("Error compiling shader {}: {}",
					  sFile.c_str(),
					  file.GetError().c_str());
			return 0;
		}

		if (file.Read(sBuffer, file.GetFileSize()) == -1) {
			Locator::getLogger()->warn("Error compiling shader {}: {}",
					  sFile.c_str(),
					  file.GetError().c_str());
			return 0;
		}
	}

	if (PREFSMAN->m_verbose_log > 1)
		Locator::getLogger()->trace("Compiling shader {}", sFile.c_str());
	const auto hShader = glCreateShaderObjectARB(ShaderType);
	vector<const GLcharARB*> apData;
	vector<GLint> aiLength;
	for (auto& s : asDefines) {
		s = ssprintf("#define %s\n", s.c_str());
		apData.push_back(s.data());
		aiLength.push_back(s.size());
	}
	apData.push_back("#line 1\n");
	aiLength.push_back(8);

	apData.push_back(sBuffer.data());
	aiLength.push_back(sBuffer.size());
	glShaderSourceARB(hShader, apData.size(), &apData[0], &aiLength[0]);

	glCompileShaderARB(hShader);

	const auto sInfo = GetInfoLog(hShader);

	auto bCompileStatus = GL_FALSE;
	glGetObjectParameterivARB(
	  hShader, GL_OBJECT_COMPILE_STATUS_ARB, &bCompileStatus);
	if (!bCompileStatus) {
		Locator::getLogger()->warn("Error compiling shader {}:\n{}", sFile.c_str(), sInfo.c_str());
		glDeleteObjectARB(hShader);
		return 0;
	}

	if (!sInfo.empty())
		Locator::getLogger()->trace("Messages compiling shader {}:\n{}", sFile.c_str(), sInfo.c_str());

	return hShader;
}

GLhandleARB
LoadShader(GLenum ShaderType, std::string sFile, vector<std::string> asDefines)
{
	/* Vertex shaders are supported by more hardware than fragment shaders.
	 * If this causes any trouble I will have to up the requirement for both
	 * of them to at least GL 2.0. Regardless we need basic GLSL support.
	 * -Colby */
	if (!glewIsSupported("GL_ARB_shading_language_100 GL_ARB_shader_objects") ||
		(ShaderType == GL_FRAGMENT_SHADER_ARB &&
		 !glewIsSupported("GL_VERSION_2_0")) ||
		(ShaderType == GL_VERTEX_SHADER_ARB &&
		 !glewIsSupported("GL_ARB_vertex_shader"))) {
		Locator::getLogger()->warn("{} shaders not supported by driver. Some effects will not "
				  "be available.",
				  (ShaderType == GL_FRAGMENT_SHADER_ARB) ? "Fragment"
														 : "Vertex");
		return 0;
	}

	// XXX: dumb, but I don't feel like refactoring ragedisplay for this. -Colby
	GLhandleARB secondaryShader = 0;
	if (sFile == "Data/Shaders/GLSL/Cel.vert")
		secondaryShader = CompileShader(
		  GL_FRAGMENT_SHADER_ARB, "Data/Shaders/GLSL/Cel.frag", asDefines);
	else if (sFile == "Data/Shaders/GLSL/Shell.vert")
		secondaryShader = CompileShader(
		  GL_FRAGMENT_SHADER_ARB, "Data/Shaders/GLSL/Shell.frag", asDefines);

	const auto hShader = CompileShader(ShaderType, sFile, asDefines);
	if (hShader == 0)
		return 0;

	const auto hProgram = glCreateProgramObjectARB();
	glAttachObjectARB(hProgram, hShader);

	if (secondaryShader) {
		glAttachObjectARB(hProgram, secondaryShader);
		glDeleteObjectARB(secondaryShader);
	}
	glDeleteObjectARB(hShader);

	// Link the program.
	glLinkProgramARB(hProgram);
	GLint bLinkStatus = false;
	glGetObjectParameterivARB(
	  hProgram, GL_OBJECT_LINK_STATUS_ARB, &bLinkStatus);

	if (!bLinkStatus) {
		Locator::getLogger()->warn("Error linking shader {}: {}", sFile.c_str(), GetInfoLog(hProgram).c_str());
		glDeleteObjectARB(hProgram);
		return 0;
	}
	return hProgram;
}

static int g_iAttribTextureMatrixScale;

static GLhandleARB g_bUnpremultiplyShader = 0;
static GLhandleARB g_bColorBurnShader = 0;
static GLhandleARB g_bColorDodgeShader = 0;
static GLhandleARB g_bVividLightShader = 0;
static GLhandleARB g_hHardMixShader = 0;
static GLhandleARB g_hOverlayShader = 0;
static GLhandleARB g_hScreenShader = 0;
static GLhandleARB g_hYUYV422Shader = 0;
static GLhandleARB g_gShellShader = 0;
static GLhandleARB g_gCelShader = 0;

void
InitShaders()
{
	// xxx: replace this with a ShaderManager or something that reads in
	// the shaders and determines shader type by file extension. -aj
	// argh shaders in stepmania are painful -colby
	const vector<std::string> asDefines;

	// used for scrolling textures (I think)
	g_bTextureMatrixShader =
	  LoadShader(GL_VERTEX_SHADER_ARB,
				 "Data/Shaders/GLSL/Texture matrix scaling.vert",
				 asDefines);

	// these two are for dancing characters and are both actually shader pairs
	g_gShellShader = LoadShader(
	  GL_VERTEX_SHADER_ARB, "Data/Shaders/GLSL/Shell.vert", asDefines);
	g_gCelShader =
	  LoadShader(GL_VERTEX_SHADER_ARB, "Data/Shaders/GLSL/Cel.vert", asDefines);

	// effects
	g_bUnpremultiplyShader = LoadShader(GL_FRAGMENT_SHADER_ARB,
										"Data/Shaders/GLSL/Unpremultiply.frag",
										asDefines);
	g_bColorBurnShader = LoadShader(
	  GL_FRAGMENT_SHADER_ARB, "Data/Shaders/GLSL/Color burn.frag", asDefines);
	g_bColorDodgeShader = LoadShader(
	  GL_FRAGMENT_SHADER_ARB, "Data/Shaders/GLSL/Color dodge.frag", asDefines);
	g_bVividLightShader = LoadShader(
	  GL_FRAGMENT_SHADER_ARB, "Data/Shaders/GLSL/Vivid light.frag", asDefines);
	g_hHardMixShader = LoadShader(
	  GL_FRAGMENT_SHADER_ARB, "Data/Shaders/GLSL/Hard mix.frag", asDefines);
	g_hOverlayShader = LoadShader(
	  GL_FRAGMENT_SHADER_ARB, "Data/Shaders/GLSL/Overlay.frag", asDefines);
	g_hScreenShader = LoadShader(
	  GL_FRAGMENT_SHADER_ARB, "Data/Shaders/GLSL/Screen.frag", asDefines);
	g_hYUYV422Shader = LoadShader(
	  GL_FRAGMENT_SHADER_ARB, "Data/Shaders/GLSL/YUYV422.frag", asDefines);

	// Bind attributes.
	if (g_bTextureMatrixShader) {
		FlushGLErrors();
		g_iAttribTextureMatrixScale =
		  glGetAttribLocationARB(g_bTextureMatrixShader, "TextureMatrixScale");
		if (g_iAttribTextureMatrixScale == -1) {
			Locator::getLogger()->trace(R"(Scaling shader link failed: couldn't bind attribute "TextureMatrixScale")");
			glDeleteObjectARB(g_bTextureMatrixShader);
			g_bTextureMatrixShader = 0;
		} else {
			AssertNoGLError();

			/* Older Catalyst drivers seem to throw GL_INVALID_OPERATION here.
			 */
			glVertexAttrib2fARB(g_iAttribTextureMatrixScale, 1, 1);
			const auto iError = glGetError();
			if (iError == GL_INVALID_OPERATION) {
				Locator::getLogger()->trace("Scaling shader failed: glVertexAttrib2fARB "
						   "returned GL_INVALID_OPERATION");
				glDeleteObjectARB(g_bTextureMatrixShader);
				g_bTextureMatrixShader = 0;
			} else {
				ASSERT_M(iError == GL_NO_ERROR, GLToString(iError));
			}
		}
	}
}

static LocalizedString OBTAIN_AN_UPDATED_VIDEO_DRIVER(
  "RageDisplay_Legacy",
  "Obtain an updated driver from your video card manufacturer.");
static LocalizedString GLDIRECT_IS_NOT_COMPATIBLE("RageDisplay_Legacy",
												  "GLDirect was detected.  "
												  "GLDirect is not compatible "
												  "with this game and should "
												  "be disabled.");
std::string
RageDisplay_Legacy::Init(const VideoModeParams& p,
						 bool bAllowUnacceleratedRenderer)
{
	g_pWind = LowLevelWindow::Create();

	auto bIgnore = false;
	auto sError = SetVideoMode(p, bIgnore);
	if (!sError.empty())
		return sError;

	// Log driver details
	g_pWind->LogDebugInformation();
	if (PREFSMAN->m_verbose_log > 1) {
		Locator::getLogger()->trace("OGL Vendor: {}", glGetString(GL_VENDOR));
		Locator::getLogger()->trace("OGL Renderer: {}", glGetString(GL_RENDERER));
		Locator::getLogger()->trace("OGL Version: {}", glGetString(GL_VERSION));
		Locator::getLogger()->trace("OGL Max texture size: {}", GetMaxTextureSize());
		Locator::getLogger()->trace("OGL Texture units: {}", g_iMaxTextureUnits);
		Locator::getLogger()->trace("GLU Version: {}", gluGetString(GLU_VERSION));

		/* Pretty-print the extension string: */
		Locator::getLogger()->trace("OGL Extensions:");
		{
			const auto szExtensionString =
			  (const char*)glGetString(GL_EXTENSIONS);
			vector<std::string> asExtensions;
			split(szExtensionString, " ", asExtensions);
			sort(asExtensions.begin(), asExtensions.end());
			size_t iNextToPrint = 0;
			while (iNextToPrint < asExtensions.size()) {
				auto iLastToPrint = iNextToPrint;
				std::string sType;
				for (auto i = iNextToPrint; i < asExtensions.size(); ++i) {
					vector<std::string> asBits;
					split(asExtensions[i], "_", asBits);
					std::string sThisType;
					if (asBits.size() > 2)
						sThisType = join(
						  std::string("_"), asBits.begin(), asBits.begin() + 2);
					if (i > iNextToPrint && sThisType != sType)
						break;
					sType = sThisType;
					iLastToPrint = i;
				}

				if (iNextToPrint == iLastToPrint) {
					Locator::getLogger()->trace("  {}", asExtensions[iNextToPrint].c_str());
					++iNextToPrint;
					continue;
				}

				auto sList = ssprintf("  %s: ", sType.c_str());
				while (iNextToPrint <= iLastToPrint) {
					vector<std::string> asBits;
					split(asExtensions[iNextToPrint], "_", asBits);
					const auto sShortExt =
					  join(std::string("_"), asBits.begin() + 2, asBits.end());
					sList += sShortExt;
					if (iNextToPrint < iLastToPrint)
						sList += ", ";
					if (iNextToPrint == iLastToPrint ||
						sList.size() + asExtensions[iNextToPrint + 1].size() >
						  120) {
						Locator::getLogger()->trace(sList.c_str());
						sList = "    ";
					}
					++iNextToPrint;
				}
			}
		}
	}

	if (g_pWind->IsSoftwareRenderer(sError)) {
		if (!bAllowUnacceleratedRenderer)
			return sError + "  " + OBTAIN_AN_UPDATED_VIDEO_DRIVER.GetValue() +
				   "\n\n";
		Locator::getLogger()->warn("Low-performance OpenGL renderer: {}", sError.c_str());
	}

#ifdef _WIN32
	/* GLDirect is a Direct3D wrapper for OpenGL.  It's rather buggy; and if in
	 * any case GLDirect can successfully render us, we should be able to do so
	 * too using Direct3D directly.  (If we can't, it's a bug that we can work
	 * around--if GLDirect can do it, so can we!) */
	if (!strncmp((const char*)glGetString(GL_RENDERER), "GLDirect", 8))
		return GLDIRECT_IS_NOT_COMPATIBLE.GetValue() + "\n";
#endif

	/* Log this, so if people complain that the radar looks bad on their
	 * system we can compare them: */
	glGetFloatv(GL_LINE_WIDTH_RANGE, g_line_range);
	glGetFloatv(GL_POINT_SIZE_RANGE, g_point_range);

	return std::string();
}

RageDisplay_Legacy::~RageDisplay_Legacy()
{
	delete g_pWind;
}

void
RageDisplay_Legacy::GetDisplaySpecs(DisplaySpecs& out) const
{
	out.clear();
	g_pWind->GetDisplaySpecs(out);
}

static void
CheckPalettedTextures()
{
	std::string sError;
	do {
		if (!GLEW_EXT_paletted_texture) {
			sError = "GL_EXT_paletted_texture missing";
			break;
		}

		/* Check to see if paletted textures really work. */
		const auto glTexFormat =
		  g_GLPixFmtInfo[RagePixelFormat_PAL].internalfmt;
		const auto glImageFormat = g_GLPixFmtInfo[RagePixelFormat_PAL].format;
		const auto glImageType = g_GLPixFmtInfo[RagePixelFormat_PAL].type;

		const auto iBits = 8;

		FlushGLErrors();
#define GL_CHECK_ERROR(f)                                                      \
                                                                               \
	{                                                                          \
		GLenum glError = glGetError();                                         \
		if (glError != GL_NO_ERROR) {                                          \
			sError = ssprintf(f " failed (%s)", GLToString(glError).c_str());  \
			break;                                                             \
		}                                                                      \
	}

		glTexImage2D(GL_PROXY_TEXTURE_2D,
					 0,
					 glTexFormat,
					 16,
					 16,
					 0,
					 glImageFormat,
					 glImageType,
					 nullptr);
		GL_CHECK_ERROR("glTexImage2D");

		GLuint iFormat = 0;
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D,
								 0,
								 GLenum(GL_TEXTURE_INTERNAL_FORMAT),
								 (GLint*)&iFormat);
		GL_CHECK_ERROR("glGetTexLevelParameteriv(GL_TEXTURE_INTERNAL_FORMAT)");
		if (iFormat != glTexFormat) {
			sError = ssprintf("Expected format %s, got %s instead",
							  GLToString(glTexFormat).c_str(),
							  GLToString(iFormat).c_str());
			break;
		}

		GLubyte palette[256 * 4];
		memset(palette, 0, sizeof(palette));
		glColorTableEXT(GL_PROXY_TEXTURE_2D,
						GL_RGBA8,
						256,
						GL_RGBA,
						GL_UNSIGNED_BYTE,
						palette);
		GL_CHECK_ERROR("glColorTableEXT");

		auto iSize = 0;
		glGetTexLevelParameteriv(
		  GL_PROXY_TEXTURE_2D, 0, GLenum(GL_TEXTURE_INDEX_SIZE_EXT), &iSize);
		GL_CHECK_ERROR("glGetTexLevelParameteriv(GL_TEXTURE_INDEX_SIZE_EXT)");
		if (iBits > iSize || iSize > 8) {
			sError =
			  ssprintf("Expected %i-bit palette, got a %i-bit one instead",
					   iBits,
					   static_cast<int>(iSize));
			break;
		}

		auto iRealWidth = 0;
		glGetColorTableParameterivEXT(
		  GL_PROXY_TEXTURE_2D, GL_COLOR_TABLE_WIDTH, &iRealWidth);
		GL_CHECK_ERROR("glGetColorTableParameterivEXT(GL_COLOR_TABLE_WIDTH)");
		if (iRealWidth != 1 << iBits) {
			sError = ssprintf("GL_COLOR_TABLE_WIDTH returned %i instead of %i",
							  static_cast<int>(iRealWidth),
							  1 << iBits);
			break;
		}

		auto iRealFormat = 0;
		glGetColorTableParameterivEXT(
		  GL_PROXY_TEXTURE_2D, GL_COLOR_TABLE_FORMAT, &iRealFormat);
		GL_CHECK_ERROR("glGetColorTableParameterivEXT(GL_COLOR_TABLE_FORMAT)");
		if (iRealFormat != GL_RGBA8) {
			sError =
			  ssprintf("GL_COLOR_TABLE_FORMAT returned %s instead of GL_RGBA8",
					   GLToString(iRealFormat).c_str());
			break;
		}
	} while (false);
#undef GL_CHECK_ERROR

	if (sError.empty())
		return;

	/* If 8-bit palettes don't work, disable them entirely--don't trust 4-bit
	 * palettes if it can't even get 8-bit ones right. */
	glColorTableEXT = nullptr;
	glGetColorTableParameterivEXT = nullptr;
	Locator::getLogger()->trace("Paletted textures disabled: {}.", sError.c_str());
}

static void
CheckReversePackedPixels()
{
	/* Try to create a texture. */
	FlushGLErrors();
	glTexImage2D(GL_PROXY_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 16,
				 16,
				 0,
				 GL_BGRA,
				 GL_UNSIGNED_SHORT_1_5_5_5_REV,
				 nullptr);

	const auto glError = glGetError();
	if (glError == GL_NO_ERROR) {
		g_bReversePackedPixelsWorks = true;
	} else {
		g_bReversePackedPixelsWorks = false;
		Locator::getLogger()->trace("GL_UNSIGNED_SHORT_1_5_5_5_REV failed ({}), disabled",
				  GLToString(glError).c_str());
	}
}

void
SetupExtensions()
{
	const auto fGLVersion = StringToFloat((const char*)glGetString(GL_VERSION));
	g_glVersion = lround(fGLVersion * 10);

	const auto fGLUVersion =
	  StringToFloat((const char*)gluGetString(GLU_VERSION));
	g_gluVersion = lround(fGLUVersion * 10);

	glewInit();

	g_iMaxTextureUnits = 1;
	if (GLEW_ARB_multitexture)
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,
					  static_cast<GLint*>(&g_iMaxTextureUnits));

	CheckPalettedTextures();
	CheckReversePackedPixels();

	{
		auto iMaxTableSize = 0;
		glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE, &iMaxTableSize);
		if (iMaxTableSize < 256) {
			/* The minimum GL_MAX_PIXEL_MAP_TABLE is 32; if it's not at least
			 * 256, we can't fit a palette in it, so we can't send paletted data
			 * as input for a non-paletted texture. */
			Locator::getLogger()->trace("GL_MAX_PIXEL_MAP_TABLE is only {}",
					  static_cast<int>(iMaxTableSize));
			g_bColorIndexTableWorks = false;
		} else {
			g_bColorIndexTableWorks = true;
		}
	}
}

bool
RageDisplay_Legacy::UseOffscreenRenderTarget()
{
	if (!(*GetActualVideoModeParams()).renderOffscreen || !TEXTUREMAN) {
		return false;
	}

	if (!offscreenRenderTarget) {
		RenderTargetParam param;
		param.bWithDepthBuffer = true;
		param.bWithAlpha = true;
		param.bFloat = false;
		param.iWidth = (*GetActualVideoModeParams()).width;
		param.iHeight = (*GetActualVideoModeParams()).height;
		const RageTextureID id(
		  ssprintf("FullscreenTexture%dx%d", param.iWidth, param.iHeight));
		// See if we have this texture loaded already
		// (not GC'd yet). If it exists and we try to recreate
		// it, we'll get an error
		if (TEXTUREMAN->IsTextureRegistered(id)) {
			offscreenRenderTarget = std::static_pointer_cast<RageTextureRenderTarget>(TEXTUREMAN->LoadTexture(id));
		} else {
			offscreenRenderTarget = std::make_shared<RageTextureRenderTarget>(id, param);
			TEXTUREMAN->RegisterTexture(id, offscreenRenderTarget);
		}
	}
	return true;
}

void
RageDisplay_Legacy::ResolutionChanged()
{
	// LOG->Warn( "RageDisplay_Legacy::ResolutionChanged" );

	/* Clear any junk that's in the framebuffer. */
	if (BeginFrame())
		EndFrame();

	RageDisplay::ResolutionChanged();

	if (offscreenRenderTarget && TEXTUREMAN) {
		TEXTUREMAN->UnloadTexture(offscreenRenderTarget);
		offscreenRenderTarget = nullptr;
	}
}

// Return true if mode change was successful.
// bNewDeviceOut is set true if a new device was created and textures
// need to be reloaded.
std::string
RageDisplay_Legacy::TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut)
{
	// LOG->Warn( "RageDisplay_Legacy::TryVideoMode( %d, %d, %d, %d, %d, %d )",
	// p.windowed, p.width, p.height, p.bpp, p.rate, p.vsync );

	std::string err;
	err = g_pWind->TryVideoMode(p, bNewDeviceOut);
	if (!err.empty())
		return err; // failed to set video mode

	/* Now that we've initialized, we can search for extensions.  Do this before
	 * InvalidateObjects, since AllocateBuffers needs it. */
	SetupExtensions();

	if (bNewDeviceOut) {
		/* We have a new OpenGL context, so we have to tell our textures that
		 * their OpenGL texture number is invalid. */
		if (TEXTUREMAN != nullptr)
			TEXTUREMAN->InvalidateTextures();

		/* Delete all render targets.  They may have associated resources other
		 * than the texture itself. */
		for (auto& rt : g_mapRenderTargets) {
			delete rt.second;
		}

		g_mapRenderTargets.clear();

		/* Recreate all vertex buffers. */
		InvalidateObjects();

		InitShaders();
	}

// I'm not sure this is correct -Colby
#ifdef _WIN32
	/* Set vsync the Windows way, if we can.  (What other extensions are there
	 * to do this, for other archs?) */
	if (wglewIsSupported("WGL_EXT_swap_control"))
		wglSwapIntervalEXT(p.vsync);
	else
		return std::string(
		  "The WGL_EXT_swap_control extension is not supported on "
		  "your computer.");
#endif

	ResolutionChanged();

	return std::string(); // successfully set mode
}

int
RageDisplay_Legacy::GetMaxTextureSize() const
{
	GLint size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
	return size;
}

bool
RageDisplay_Legacy::BeginFrame()
{
	/* We do this in here, rather than ResolutionChanged, or we won't update the
	 * viewport for the concurrent rendering context. */

	const auto fWidth = (*g_pWind->GetActualVideoModeParams()).windowWidth;
	const auto fHeight = (*g_pWind->GetActualVideoModeParams()).windowHeight;
	glViewport(0, 0, fWidth, fHeight);
	glClearColor(0, 0, 0, 0);
	SetZWrite(true);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const auto beginFrame = RageDisplay::BeginFrame();
	if (beginFrame && UseOffscreenRenderTarget()) {
		offscreenRenderTarget->BeginRenderingTo(false);
	}

	return beginFrame;
}

void
RageDisplay_Legacy::EndFrame()
{
	if (UseOffscreenRenderTarget()) {
		offscreenRenderTarget->FinishRenderingTo();
		Sprite fullscreenSprite;
		// We've got a hold of this, don't want sprite deleting it when
		// it's deleted
		offscreenRenderTarget->m_iRefCount++;
		fullscreenSprite.SetTexture(offscreenRenderTarget);
		fullscreenSprite.SetHorizAlign(align_left);
		fullscreenSprite.SetVertAlign(align_top);
		CameraPushMatrix();
		LoadMenuPerspective(
		  0,
		  static_cast<float>(GetActualVideoModeParams()->width),
		  static_cast<float>(GetActualVideoModeParams()->height),
		  static_cast<float>(GetActualVideoModeParams()->width) / 2.f,
		  static_cast<float>(GetActualVideoModeParams()->height) / 2.f);
		fullscreenSprite.Draw();
		CameraPopMatrix();
	}

	FrameLimitBeforeVsync();
	const auto beforePresent = std::chrono::steady_clock::now();
	g_pWind->SwapBuffers();
	glFlush();

	g_pWind->Update();

	const auto afterPresent = std::chrono::steady_clock::now();
	const auto endTime = afterPresent - beforePresent;

	SetPresentTime(endTime);

	FrameLimitAfterVsync((*GetActualVideoModeParams()).rate);

	RageDisplay::EndFrame();
}

RageSurface*
RageDisplay_Legacy::CreateScreenshot()
{
	const auto width = (*g_pWind->GetActualVideoModeParams()).width;
	const auto height = (*g_pWind->GetActualVideoModeParams()).height;

	RageSurface* image = nullptr;
	if (offscreenRenderTarget) {
		const auto raw = GetTexture(offscreenRenderTarget->GetTexHandle());
		image = CreateSurface(offscreenRenderTarget->GetImageWidth(),
							  offscreenRenderTarget->GetImageHeight(),
							  raw->fmt.BitsPerPixel,
							  raw->fmt.Rmask,
							  raw->fmt.Gmask,
							  raw->fmt.Bmask,
							  raw->fmt.Amask);
		RageSurfaceUtils::Blit(raw, image);
		delete raw;
	} else {
		const auto& desc = PIXEL_FORMAT_DESC[RagePixelFormat_RGBA8];
		image = CreateSurface(width,
							  height,
							  desc.bpp,
							  desc.masks[0],
							  desc.masks[1],
							  desc.masks[2],
							  0);

		DebugFlushGLErrors();

		glReadBuffer(GL_FRONT);
		DebugAssertNoGLError();

		glReadPixels(0,
					 0,
					 (*g_pWind->GetActualVideoModeParams()).width,
					 (*g_pWind->GetActualVideoModeParams()).height,
					 GL_RGBA,
					 GL_UNSIGNED_BYTE,
					 image->pixels);
		DebugAssertNoGLError();

		RageSurfaceUtils::FlipVertically(image);
	}

	return image;
}

RageSurface*
RageDisplay_Legacy::GetTexture(intptr_t iTexture)
{
	if (iTexture == 0)
		return nullptr; // XXX

	FlushGLErrors();

	glBindTexture(GL_TEXTURE_2D, iTexture);
	GLint iHeight, iWidth, iAlphaBits;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &iHeight);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &iWidth);
	glGetTexLevelParameteriv(
	  GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &iAlphaBits);
	const int iFormat =
	  iAlphaBits ? RagePixelFormat_RGBA8 : RagePixelFormat_RGB8;

	const auto& desc = PIXEL_FORMAT_DESC[iFormat];
	const auto pImage = CreateSurface(iWidth,
									  iHeight,
									  desc.bpp,
									  desc.masks[0],
									  desc.masks[1],
									  desc.masks[2],
									  desc.masks[3]);

	glGetTexImage(GL_TEXTURE_2D,
				  0,
				  g_GLPixFmtInfo[iFormat].format,
				  GL_UNSIGNED_BYTE,
				  pImage->pixels);
	AssertNoGLError();

	return pImage;
}

const ActualVideoModeParams*
RageDisplay_Legacy::GetActualVideoModeParams() const
{
	return g_pWind->GetActualVideoModeParams();
}

static void
SetupVertices(const RageSpriteVertex v[], int iNumVerts)
{
	static float *Vertex, *Texture, *Normal;
	static GLubyte* Color;
	static auto Size = 0;
	if (iNumVerts > Size) {
		Size = iNumVerts;
		delete[] Vertex;
		delete[] Color;
		delete[] Texture;
		delete[] Normal;
		Vertex = new float[Size * 3];
		Color = new GLubyte[Size * 4];
		Texture = new float[Size * 2];
		Normal = new float[Size * 3];
	}

	for (unsigned i = 0; i < static_cast<unsigned>(iNumVerts); ++i) {
		Vertex[i * 3 + 0] = v[i].p[0];
		Vertex[i * 3 + 1] = v[i].p[1];
		Vertex[i * 3 + 2] = v[i].p[2];
		Color[i * 4 + 0] = v[i].c.r;
		Color[i * 4 + 1] = v[i].c.g;
		Color[i * 4 + 2] = v[i].c.b;
		Color[i * 4 + 3] = v[i].c.a;
		Texture[i * 2 + 0] = v[i].t[0];
		Texture[i * 2 + 1] = v[i].t[1];
		Normal[i * 3 + 0] = v[i].n[0];
		Normal[i * 3 + 1] = v[i].n[1];
		Normal[i * 3 + 2] = v[i].n[2];
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, Vertex);

	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, Color);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, Texture);

	if (GLEW_ARB_multitexture) {
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, Texture);
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 0, Normal);
}

void
RageDisplay_Legacy::SendCurrentMatrices()
{
	static RageMatrix Centering;
	static RageMatrix Projection;

	if (Centering != *GetCentering() || Projection != *GetProjectionTop()) {
		Centering = *GetCentering();
		Projection = *GetProjectionTop();

		RageMatrix projection;
		RageMatrixMultiply(&projection, GetCentering(), GetProjectionTop());

		if (g_bInvertY) {
			RageMatrix flip;
			RageMatrixScale(&flip, +1, -1, +1);
			RageMatrixMultiply(&projection, &flip, &projection);
		}
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf((const float*)&projection);

		// OpenGL has just "modelView", whereas D3D has "world" and "view"
		RageMatrix modelView;
		RageMatrixMultiply(&modelView, GetViewTop(), GetWorldTop());
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((const float*)&modelView);

		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf((const float*)GetTextureTop());
	}
}

class RageCompiledGeometrySWOGL : public RageCompiledGeometry
{
  public:
	void Allocate(const vector<msMesh>& vMeshes) override
	{
		/* Always allocate at least 1 entry, so &x[0] is valid. */
		m_vPosition.resize(max(1U, static_cast<unsigned>(GetTotalVertices())));
		m_vTexture.resize(max(1U, static_cast<unsigned>(GetTotalVertices())));
		m_vNormal.resize(max(1U, static_cast<unsigned>(GetTotalVertices())));
		m_vTexMatrixScale.resize(
		  max(1U, static_cast<unsigned>(GetTotalVertices())));
		m_vTriangles.resize(
		  max(1U, static_cast<unsigned>(GetTotalTriangles())));
	}
	void Change(const vector<msMesh>& vMeshes) override
	{
		for (unsigned i = 0; i < vMeshes.size(); i++) {
			const auto& meshInfo = m_vMeshInfo[i];
			const auto& mesh = vMeshes[i];
			const auto& Vertices = mesh.Vertices;
			const auto& Triangles = mesh.Triangles;

			for (unsigned j = 0; j < Vertices.size(); j++) {
				m_vPosition[meshInfo.iVertexStart + j] = Vertices[j].p;
				m_vTexture[meshInfo.iVertexStart + j] = Vertices[j].t;
				m_vNormal[meshInfo.iVertexStart + j] = Vertices[j].n;
				m_vTexMatrixScale[meshInfo.iVertexStart + j] =
				  Vertices[j].TextureMatrixScale;
			}

			for (unsigned j = 0; j < Triangles.size(); j++)
				for (unsigned k = 0; k < 3; k++) {
					const auto iVertexIndexInVBO =
					  meshInfo.iVertexStart + Triangles[j].nVertexIndices[k];
					m_vTriangles[meshInfo.iTriangleStart + j]
					  .nVertexIndices[k] =
					  static_cast<uint16_t>(iVertexIndexInVBO);
				}
		}
	}
	void Draw(int iMeshIndex) const override
	{
		TurnOffHardwareVBO();

		const auto& meshInfo = m_vMeshInfo[iMeshIndex];

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, &m_vPosition[0]);

		glDisableClientState(GL_COLOR_ARRAY);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, &m_vTexture[0]);

		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, &m_vNormal[0]);

		if (meshInfo.m_bNeedsTextureMatrixScale) {
			// Kill the texture translation.
			// XXX: Change me to scale the translation by the
			// TextureTranslationScale of the first vertex.
			RageMatrix mat;
			glGetFloatv(GL_TEXTURE_MATRIX, static_cast<float*>(mat));

			/*
			for( int i=0; i<4; i++ )
			{
				std::string s;
				for( int j=0; j<4; j++ )
					s += ssprintf( "%f ", mat.m[i][j] );
				LOG->Trace( s );
			}
			*/

			mat.m[3][0] = 0;
			mat.m[3][1] = 0;
			mat.m[3][2] = 0;

			glMatrixMode(GL_TEXTURE);
			glLoadMatrixf(static_cast<const float*>(mat));
		}

		glDrawElements(GL_TRIANGLES,
					   meshInfo.iTriangleCount * 3,
					   GL_UNSIGNED_SHORT,
					   &m_vTriangles[0] + meshInfo.iTriangleStart);
	}

  protected:
	vector<RageVector3> m_vPosition;
	vector<RageVector2> m_vTexture;
	vector<RageVector3> m_vNormal;
	vector<msTriangle> m_vTriangles;
	vector<RageVector2> m_vTexMatrixScale;
};

class InvalidateObject;
static std::set<InvalidateObject*> g_InvalidateList;
class InvalidateObject
{
  public:
	InvalidateObject() { g_InvalidateList.insert(this); }
	virtual ~InvalidateObject() { g_InvalidateList.erase(this); }
	virtual void Invalidate() = 0;
};

static void
InvalidateObjects()
{
	for (auto& it : g_InvalidateList) {
		it->Invalidate();
	}
}

class RageCompiledGeometryHWOGL
  : public RageCompiledGeometrySWOGL
  , public InvalidateObject
{
  protected:
	// vertex buffer object names
	GLuint m_nPositions;
	GLuint m_nTextureCoords;
	GLuint m_nNormals;
	GLuint m_nTriangles;
	GLuint m_nTextureMatrixScale;

	void AllocateBuffers();
	void UploadData();

  public:
	RageCompiledGeometryHWOGL();
	~RageCompiledGeometryHWOGL() override;

	/* This is called when our OpenGL context is invalidated. */
	void Invalidate() override;

	void Allocate(const vector<msMesh>& vMeshes) override;
	void Change(const vector<msMesh>& vMeshes) override;
	void Draw(int iMeshIndex) const override;
};

RageCompiledGeometryHWOGL::RageCompiledGeometryHWOGL()
{
	m_nPositions = 0;
	m_nTextureCoords = 0;
	m_nNormals = 0;
	m_nTriangles = 0;
	m_nTextureMatrixScale = 0;

	AllocateBuffers();
}

RageCompiledGeometryHWOGL::~RageCompiledGeometryHWOGL()
{
	DebugFlushGLErrors();

	glDeleteBuffersARB(1, &m_nPositions);
	DebugAssertNoGLError();
	glDeleteBuffersARB(1, &m_nTextureCoords);
	DebugAssertNoGLError();
	glDeleteBuffersARB(1, &m_nNormals);
	DebugAssertNoGLError();
	glDeleteBuffersARB(1, &m_nTriangles);
	DebugAssertNoGLError();
	glDeleteBuffersARB(1, &m_nTextureMatrixScale);
	DebugAssertNoGLError();
}

void
RageCompiledGeometryHWOGL::AllocateBuffers()
{
	DebugFlushGLErrors();

	if (!m_nPositions) {
		glGenBuffersARB(1, &m_nPositions);
		DebugAssertNoGLError();
	}

	if (!m_nTextureCoords) {
		glGenBuffersARB(1, &m_nTextureCoords);
		DebugAssertNoGLError();
	}

	if (!m_nNormals) {
		glGenBuffersARB(1, &m_nNormals);
		DebugAssertNoGLError();
	}

	if (!m_nTriangles) {
		glGenBuffersARB(1, &m_nTriangles);
		DebugAssertNoGLError();
	}

	if (!m_nTextureMatrixScale) {
		glGenBuffersARB(1, &m_nTextureMatrixScale);
		DebugAssertNoGLError();
	}
}

void
RageCompiledGeometryHWOGL::UploadData()
{
	DebugFlushGLErrors();

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nPositions);
	DebugAssertNoGLError();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
					GetTotalVertices() * sizeof(RageVector3),
					&m_vPosition[0],
					GL_STATIC_DRAW_ARB);
	DebugAssertNoGLError();

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nTextureCoords);
	DebugAssertNoGLError();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
					GetTotalVertices() * sizeof(RageVector2),
					&m_vTexture[0],
					GL_STATIC_DRAW_ARB);
	DebugAssertNoGLError();

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormals);
	DebugAssertNoGLError();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
					GetTotalVertices() * sizeof(RageVector3),
					&m_vNormal[0],
					GL_STATIC_DRAW_ARB);
	DebugAssertNoGLError();

	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_nTriangles);
	DebugAssertNoGLError();
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
					GetTotalTriangles() * sizeof(msTriangle),
					&m_vTriangles[0],
					GL_STATIC_DRAW_ARB);
	DebugAssertNoGLError();

	if (m_bAnyNeedsTextureMatrixScale) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nTextureMatrixScale);
		DebugAssertNoGLError();
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,
						GetTotalVertices() * sizeof(RageVector2),
						&m_vTexMatrixScale[0],
						GL_STATIC_DRAW_ARB);
		DebugAssertNoGLError();
	}
}

void
RageCompiledGeometryHWOGL::Invalidate()
{
	/* Our vertex buffers no longer exist.  Reallocate and reupload. */
	m_nPositions = 0;
	m_nTextureCoords = 0;
	m_nNormals = 0;
	m_nTriangles = 0;
	m_nTextureMatrixScale = 0;
	AllocateBuffers();
	UploadData();
}

void
RageCompiledGeometryHWOGL::Allocate(const vector<msMesh>& vMeshes)
{
	DebugFlushGLErrors();

	RageCompiledGeometrySWOGL::Allocate(vMeshes);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nPositions);
	DebugAssertNoGLError();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
					GetTotalVertices() * sizeof(RageVector3),
					nullptr,
					GL_STATIC_DRAW_ARB);
	DebugAssertNoGLError();

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nTextureCoords);
	DebugAssertNoGLError();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
					GetTotalVertices() * sizeof(RageVector2),
					nullptr,
					GL_STATIC_DRAW_ARB);
	DebugAssertNoGLError();

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormals);
	DebugAssertNoGLError();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
					GetTotalVertices() * sizeof(RageVector3),
					nullptr,
					GL_STATIC_DRAW_ARB);
	DebugAssertNoGLError();

	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_nTriangles);
	DebugAssertNoGLError();
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
					GetTotalTriangles() * sizeof(msTriangle),
					nullptr,
					GL_STATIC_DRAW_ARB);
	DebugAssertNoGLError();

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nTextureMatrixScale);
	DebugAssertNoGLError();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
					GetTotalVertices() * sizeof(RageVector2),
					nullptr,
					GL_STATIC_DRAW_ARB);
}

void
RageCompiledGeometryHWOGL::Change(const vector<msMesh>& vMeshes)
{
	RageCompiledGeometrySWOGL::Change(vMeshes);

	UploadData();
}

void
RageCompiledGeometryHWOGL::Draw(int iMeshIndex) const
{
	DebugFlushGLErrors();

	const auto& meshInfo = m_vMeshInfo[iMeshIndex];
	if ((meshInfo.iVertexCount == 0) || (meshInfo.iTriangleCount == 0))
		return;

	glEnableClientState(GL_VERTEX_ARRAY);
	DebugAssertNoGLError();
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nPositions);
	DebugAssertNoGLError();
	glVertexPointer(3, GL_FLOAT, 0, nullptr);
	DebugAssertNoGLError();

	glDisableClientState(GL_COLOR_ARRAY);
	DebugAssertNoGLError();

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	DebugAssertNoGLError();
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nTextureCoords);
	DebugAssertNoGLError();
	glTexCoordPointer(2, GL_FLOAT, 0, nullptr);
	DebugAssertNoGLError();

	// TRICKY:  Don't bind and send normals if lighting is disabled.  This
	// will save some effort transforming these values.
	// XXX: We should keep track of these ourself and avoid glGet*()
	GLboolean bLighting;
	glGetBooleanv(GL_LIGHTING, &bLighting);
	GLboolean bTextureGenS;
	glGetBooleanv(GL_TEXTURE_GEN_S, &bTextureGenS);
	GLboolean bTextureGenT;
	glGetBooleanv(GL_TEXTURE_GEN_T, &bTextureGenT);

	if (bLighting || bTextureGenS || bTextureGenT) {
		glEnableClientState(GL_NORMAL_ARRAY);
		DebugAssertNoGLError();
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormals);
		DebugAssertNoGLError();
		glNormalPointer(GL_FLOAT, 0, nullptr);
		DebugAssertNoGLError();
	} else {
		glDisableClientState(GL_NORMAL_ARRAY);
		DebugAssertNoGLError();
	}

	if (meshInfo.m_bNeedsTextureMatrixScale) {
		if (g_bTextureMatrixShader != 0) {
			/* If we're using texture matrix scales, set up that buffer, too,
			 * and enable the vertex shader.  This shader doesn't support all
			 * OpenGL state, so only enable it if we're using it. */
			glEnableVertexAttribArrayARB(g_iAttribTextureMatrixScale);
			DebugAssertNoGLError();
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nTextureMatrixScale);
			DebugAssertNoGLError();
			glVertexAttribPointerARB(
			  g_iAttribTextureMatrixScale, 2, GL_FLOAT, false, 0, nullptr);
			DebugAssertNoGLError();

			glUseProgramObjectARB(g_bTextureMatrixShader);
			DebugAssertNoGLError();
		} else {
			// Kill the texture translation.
			// XXX: Change me to scale the translation by the
			// TextureTranslationScale of the first vertex.
			RageMatrix mat;
			glGetFloatv(GL_TEXTURE_MATRIX, static_cast<float*>(mat));

			/*
			for( int i=0; i<4; i++ )
			{
				std::string s;
				for( int j=0; j<4; j++ )
					s += ssprintf( "%f ", mat.m[i][j] );
				LOG->Trace( s );
			}
			*/

			mat.m[3][0] = 0;
			mat.m[3][1] = 0;
			mat.m[3][2] = 0;

			glMatrixMode(GL_TEXTURE);
			glLoadMatrixf(static_cast<const float*>(mat));
			DebugAssertNoGLError();
		}
	}

	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_nTriangles);
	DebugAssertNoGLError();

#define BUFFER_OFFSET(o) ((char*)(o))

	ASSERT(glDrawRangeElements != NULL);
	glDrawRangeElements(
	  GL_TRIANGLES,
	  meshInfo.iVertexStart, // minimum array index contained in indices
	  meshInfo.iVertexStart + meshInfo.iVertexCount - 1,
	  // maximum array index contained in indices
	  meshInfo.iTriangleCount * 3, // number of elements to be rendered
	  GL_UNSIGNED_SHORT,
	  BUFFER_OFFSET(meshInfo.iTriangleStart * sizeof(msTriangle)));
	DebugAssertNoGLError();

	if (meshInfo.m_bNeedsTextureMatrixScale && g_bTextureMatrixShader != 0) {
		glDisableVertexAttribArrayARB(g_iAttribTextureMatrixScale);
		glUseProgramObjectARB(0);
	}
}

RageCompiledGeometry*
RageDisplay_Legacy::CreateCompiledGeometry()
{
	if (GLEW_ARB_vertex_buffer_object)
		return new RageCompiledGeometryHWOGL;
	else
		return new RageCompiledGeometrySWOGL;
}

void
RageDisplay_Legacy::DeleteCompiledGeometry(RageCompiledGeometry* p)
{
	delete p;
}

void
RageDisplay_Legacy::DrawQuadsInternal(const RageSpriteVertex v[], int iNumVerts)
{
	TurnOffHardwareVBO();
	SendCurrentMatrices();

	SetupVertices(v, iNumVerts);
	glDrawArrays(GL_QUADS, 0, iNumVerts);
}

void
RageDisplay_Legacy::DrawQuadStripInternal(const RageSpriteVertex v[],
										  int iNumVerts)
{
	TurnOffHardwareVBO();
	SendCurrentMatrices();

	SetupVertices(v, iNumVerts);
	glDrawArrays(GL_QUAD_STRIP, 0, iNumVerts);
}

void
RageDisplay_Legacy::DrawSymmetricQuadStripInternal(const RageSpriteVertex v[],
												   int iNumVerts)
{
	const auto iNumPieces = (iNumVerts - 3) / 3;
	const auto iNumTriangles = iNumPieces * 4;
	const auto iNumIndices = iNumTriangles * 3;

	// make a temporary index buffer
	static vector<uint16_t> vIndices;
	const unsigned uOldSize = vIndices.size();
	const auto uNewSize = max(uOldSize, static_cast<unsigned>(iNumIndices));
	vIndices.resize(uNewSize);
	for (uint16_t i = static_cast<uint16_t>(uOldSize) / 12;
		 i < static_cast<uint16_t>(iNumPieces);
		 i++) {
		// { 1, 3, 0 } { 1, 4, 3 } { 1, 5, 4 } { 1, 2, 5 }
		vIndices[i * 12 + 0] = i * 3 + 1;
		vIndices[i * 12 + 1] = i * 3 + 3;
		vIndices[i * 12 + 2] = i * 3 + 0;
		vIndices[i * 12 + 3] = i * 3 + 1;
		vIndices[i * 12 + 4] = i * 3 + 4;
		vIndices[i * 12 + 5] = i * 3 + 3;
		vIndices[i * 12 + 6] = i * 3 + 1;
		vIndices[i * 12 + 7] = i * 3 + 5;
		vIndices[i * 12 + 8] = i * 3 + 4;
		vIndices[i * 12 + 9] = i * 3 + 1;
		vIndices[i * 12 + 10] = i * 3 + 2;
		vIndices[i * 12 + 11] = i * 3 + 5;
	}

	TurnOffHardwareVBO();
	SendCurrentMatrices();

	SetupVertices(v, iNumVerts);
	glDrawElements(GL_TRIANGLES, iNumIndices, GL_UNSIGNED_SHORT, &vIndices[0]);
}

void
RageDisplay_Legacy::DrawFanInternal(const RageSpriteVertex v[], int iNumVerts)
{
	TurnOffHardwareVBO();
	SendCurrentMatrices();

	SetupVertices(v, iNumVerts);
	glDrawArrays(GL_TRIANGLE_FAN, 0, iNumVerts);
}

void
RageDisplay_Legacy::DrawStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
	TurnOffHardwareVBO();
	SendCurrentMatrices();

	SetupVertices(v, iNumVerts);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, iNumVerts);
}

void
RageDisplay_Legacy::DrawTrianglesInternal(const RageSpriteVertex v[],
										  int iNumVerts)
{
	TurnOffHardwareVBO();
	SendCurrentMatrices();

	SetupVertices(v, iNumVerts);
	glDrawArrays(GL_TRIANGLES, 0, iNumVerts);
}

void
RageDisplay_Legacy::DrawCompiledGeometryInternal(const RageCompiledGeometry* p,
												 int iMeshIndex)
{
	TurnOffHardwareVBO();
	SendCurrentMatrices();

	p->Draw(iMeshIndex);
}

void
RageDisplay_Legacy::DrawLineStripInternal(const RageSpriteVertex v[],
										  int iNumVerts,
										  float fLineWidth)
{
	if (!(*GetActualVideoModeParams()).bSmoothLines) {
		/* Fall back on the generic polygon-based line strip. */
		RageDisplay::DrawLineStripInternal(v, iNumVerts, fLineWidth);
		return;
	}

	SendCurrentMatrices();

	/* Draw a nice AA'd line loop.  One problem with this is that point and line
	 * sizes don't always precisely match, which doesn't look quite right.
	 * It's worth it for the AA, though. */
	glEnable(GL_LINE_SMOOTH);

	/* fLineWidth is in units relative to object space, but OpenGL line and
	 * point sizes are in raster units (actual pixels).  Scale the line width by
	 * the average ratio; if object space is 640x480, and we have a 1280x960
	 * window, we'll double the width. */
	{
		auto pMat = GetProjectionTop();
		const auto fW = 2 / pMat->m[0][0];
		const auto fH = -2 / pMat->m[1][1];
		const auto fWidthVal =
		  static_cast<float>((*g_pWind->GetActualVideoModeParams()).width) / fW;
		const auto fHeightVal =
		  static_cast<float>((*g_pWind->GetActualVideoModeParams()).height) /
		  fH;
		fLineWidth *= (fWidthVal + fHeightVal) / 2;
	}

	/* Clamp the width to the hardware max for both lines and points (whichever
	 * is more restrictive). */
	fLineWidth = std::clamp(fLineWidth, g_line_range[0], g_line_range[1]);
	fLineWidth = std::clamp(fLineWidth, g_point_range[0], g_point_range[1]);

	/* Hmm.  The granularity of lines and points might be different; for
	 * example, if lines are .5 and points are .25, we might want to snap the
	 * width to the nearest .5, so the hardware doesn't snap them to different
	 * sizes.  Does it matter? */
	glLineWidth(fLineWidth);

	/* Draw the line loop: */
	SetupVertices(v, iNumVerts);
	glDrawArrays(GL_LINE_STRIP, 0, iNumVerts);

	glDisable(GL_LINE_SMOOTH);

	/* Round off the corners.  This isn't perfect; the point is sometimes a
	 * little larger than the line, causing a small bump on the edge.  Not sure
	 * how to fix that. */
	glPointSize(fLineWidth);

	/* Hack: if the points will all be the same, we don't want to draw
	 * any points at all, since there's nothing to connect.  That'll happen
	 * if both scale factors in the matrix are ~0.  (Actually, I think
	 * it's true if two of the three scale factors are ~0, but we don't
	 * use this for anything 3d at the moment anyway ...)  This is needed
	 * because points aren't scaled like regular polys--a zero-size point
	 * will still be drawn. */
	RageMatrix mat;
	glGetFloatv(GL_MODELVIEW_MATRIX, static_cast<float*>(mat));

	if (mat.m[0][0] < 1e-5 && mat.m[1][1] < 1e-5)
		return;

	glEnable(GL_POINT_SMOOTH);

	SetupVertices(v, iNumVerts);
	glDrawArrays(GL_POINTS, 0, iNumVerts);

	glDisable(GL_POINT_SMOOTH);
}

static bool
SetTextureUnit(TextureUnit tu)
{
	// If multitexture isn't supported, ignore all textures except for 0.
	if (!GLEW_ARB_multitexture && tu != TextureUnit_1)
		return false;

	if (static_cast<int>(tu) > g_iMaxTextureUnits)
		return false;
	glActiveTextureARB(enum_add2(GL_TEXTURE0_ARB, tu));
	return true;
}

void
RageDisplay_Legacy::ClearAllTextures()
{
	FOREACH_ENUM(TextureUnit, i)
	SetTexture(i, 0);

	// HACK:  Reset the active texture to 0.
	// TODO:  Change all texture functions to take a stage number.
	if (GLEW_ARB_multitexture)
		glActiveTextureARB(GL_TEXTURE0_ARB);
}

int
RageDisplay_Legacy::GetNumTextureUnits()
{
	if (GLEW_ARB_multitexture)
		return 1;

	return g_iMaxTextureUnits;
}

void
RageDisplay_Legacy::SetTexture(TextureUnit tu, intptr_t iTexture)
{
	if (!SetTextureUnit(tu))
		return;

	if (iTexture != 0u) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, iTexture);
	} else {
		glDisable(GL_TEXTURE_2D);
	}
}

void
RageDisplay_Legacy::SetTextureMode(TextureUnit tu, TextureMode tm)
{
	if (!SetTextureUnit(tu))
		return;

	switch (tm) {
		case TextureMode_Modulate:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			break;
		case TextureMode_Add:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
			break;
		case TextureMode_Glow:
			// the below function is glowmode,brighten:
			if (!GLEW_ARB_texture_env_combine &&
				!GLEW_EXT_texture_env_combine) {
				/* This is changing blend state, instead of texture state, which
				 * isn't great, but it's better than doing nothing. */
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				return;
			}

			// and this is glowmode,whiten:
			// Source color is the diffuse color only:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
			glTexEnvi(GL_TEXTURE_ENV, GLenum(GL_COMBINE_RGB_EXT), GL_REPLACE);
			glTexEnvi(
			  GL_TEXTURE_ENV, GLenum(GL_SOURCE0_RGB_EXT), GL_PRIMARY_COLOR_EXT);

			// Source alpha is texture alpha * diffuse alpha:
			glTexEnvi(
			  GL_TEXTURE_ENV, GLenum(GL_COMBINE_ALPHA_EXT), GL_MODULATE);
			glTexEnvi(
			  GL_TEXTURE_ENV, GLenum(GL_OPERAND0_ALPHA_EXT), GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV,
					  GLenum(GL_SOURCE0_ALPHA_EXT),
					  GL_PRIMARY_COLOR_EXT);
			glTexEnvi(
			  GL_TEXTURE_ENV, GLenum(GL_OPERAND1_ALPHA_EXT), GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV, GLenum(GL_SOURCE1_ALPHA_EXT), GL_TEXTURE);
			break;
		default:
			break;
	}
}

void
RageDisplay_Legacy::SetTextureFiltering(TextureUnit tu, bool b)
{
	glTexParameteri(
	  GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, b ? GL_LINEAR : GL_NEAREST);

	GLint iMinFilter;
	if (b) {
		auto iWidth1 = -1;
		auto iWidth2 = -1;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &iWidth1);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, &iWidth2);
		if (iWidth1 > 1 && iWidth2 != 0) {
			/* Mipmaps are enabled. */
			if ((*g_pWind->GetActualVideoModeParams()).bTrilinearFiltering)
				iMinFilter = GL_LINEAR_MIPMAP_LINEAR;
			else
				iMinFilter = GL_LINEAR_MIPMAP_NEAREST;
		} else {
			iMinFilter = GL_LINEAR;
		}
	} else {
		iMinFilter = GL_NEAREST;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, iMinFilter);
}

void
RageDisplay_Legacy::SetEffectMode(EffectMode effect)
{
	if (!GLEW_ARB_fragment_program || !GLEW_ARB_shading_language_100 ||
		!GLEW_ARB_shader_objects)
		return;

	GLhandleARB hShader = 0;
	switch (effect) {
		case EffectMode_Normal:
			hShader = 0;
			break;
		case EffectMode_Unpremultiply:
			hShader = g_bUnpremultiplyShader;
			break;
		case EffectMode_ColorBurn:
			hShader = g_bColorBurnShader;
			break;
		case EffectMode_ColorDodge:
			hShader = g_bColorDodgeShader;
			break;
		case EffectMode_VividLight:
			hShader = g_bVividLightShader;
			break;
		case EffectMode_HardMix:
			hShader = g_hHardMixShader;
			break;
		case EffectMode_Overlay:
			hShader = g_hOverlayShader;
			break;
		case EffectMode_Screen:
			hShader = g_hScreenShader;
			break;
		case EffectMode_YUYV422:
			hShader = g_hYUYV422Shader;
			break;
		default:
			break;
	}

	DebugFlushGLErrors();
	glUseProgramObjectARB(hShader);
	if (hShader == 0)
		return;
	const auto iTexture1 = glGetUniformLocationARB(hShader, "Texture1");
	const auto iTexture2 = glGetUniformLocationARB(hShader, "Texture2");
	glUniform1iARB(iTexture1, 0);
	glUniform1iARB(iTexture2, 1);

	if (effect == EffectMode_YUYV422) {
		const auto iTextureWidthUniform =
		  glGetUniformLocationARB(hShader, "TextureWidth");
		GLint iWidth;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &iWidth);
		glUniform1iARB(iTextureWidthUniform, iWidth);
	}

	DebugAssertNoGLError();
}

bool
RageDisplay_Legacy::IsEffectModeSupported(EffectMode effect)
{
	switch (effect) {
		case EffectMode_Normal:
			return true;
		case EffectMode_Unpremultiply:
			return g_bUnpremultiplyShader != 0;
		case EffectMode_ColorBurn:
			return g_bColorBurnShader != 0;
		case EffectMode_ColorDodge:
			return g_bColorDodgeShader != 0;
		case EffectMode_VividLight:
			return g_bVividLightShader != 0;
		case EffectMode_HardMix:
			return g_hHardMixShader != 0;
		case EffectMode_Overlay:
			return g_hOverlayShader != 0;
		case EffectMode_Screen:
			return g_hScreenShader != 0;
		case EffectMode_YUYV422:
			return g_hYUYV422Shader != 0;
		default:
			return false;
	}
}

void
RageDisplay_Legacy::SetBlendMode(BlendMode mode)
{
	glEnable(GL_BLEND);

	if (glBlendEquation != nullptr) {
		if (mode == BLEND_INVERT_DEST)
			glBlendEquation(GL_FUNC_SUBTRACT);
		else if (mode == BLEND_SUBTRACT)
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
		else
			glBlendEquation(GL_FUNC_ADD);
	}

	int iSourceRGB, iDestRGB;
	auto iSourceAlpha = GL_ONE, iDestAlpha = GL_ONE_MINUS_SRC_ALPHA;
	switch (mode) {
		case BLEND_NORMAL:
			iSourceRGB = GL_SRC_ALPHA;
			iDestRGB = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case BLEND_ADD:
			iSourceRGB = GL_SRC_ALPHA;
			iDestRGB = GL_ONE;
			break;
		case BLEND_SUBTRACT:
			iSourceRGB = GL_SRC_ALPHA;
			iDestRGB = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case BLEND_MODULATE:
			iSourceRGB = GL_ZERO;
			iDestRGB = GL_SRC_COLOR;
			break;
		case BLEND_COPY_SRC:
			iSourceRGB = GL_ONE;
			iDestRGB = GL_ZERO;
			iSourceAlpha = GL_ONE;
			iDestAlpha = GL_ZERO;
			break;
		case BLEND_ALPHA_MASK:
			iSourceRGB = GL_ZERO;
			iDestRGB = GL_ONE;
			iSourceAlpha = GL_ZERO;
			iDestAlpha = GL_SRC_ALPHA;
			break;
		case BLEND_ALPHA_KNOCK_OUT:
			iSourceRGB = GL_ZERO;
			iDestRGB = GL_ONE;
			iSourceAlpha = GL_ZERO;
			iDestAlpha = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case BLEND_ALPHA_MULTIPLY:
			iSourceRGB = GL_SRC_ALPHA;
			iDestRGB = GL_ZERO;
			break;
		case BLEND_WEIGHTED_MULTIPLY:
			/* output = 2*(dst*src).  0.5,0.5,0.5 is identity; darker colors
			 * darken the image, and brighter colors lighten the image. */
			iSourceRGB = GL_DST_COLOR;
			iDestRGB = GL_SRC_COLOR;
			break;
		case BLEND_INVERT_DEST:
			/* out = src - dst.  The source color should almost always be
			 * #FFFFFF, to make it "1 - dst". */
			iSourceRGB = GL_ONE;
			iDestRGB = GL_ONE;
			break;
		case BLEND_NO_EFFECT:
			iSourceRGB = GL_ZERO;
			iDestRGB = GL_ONE;
			iSourceAlpha = GL_ZERO;
			iDestAlpha = GL_ONE;
			break;
			DEFAULT_FAIL(mode);
	}

	if (GLEW_EXT_blend_equation_separate)
		glBlendFuncSeparateEXT(iSourceRGB, iDestRGB, iSourceAlpha, iDestAlpha);
	else
		glBlendFunc(iSourceRGB, iDestRGB);
}

bool
RageDisplay_Legacy::IsZWriteEnabled() const
{
	bool a;
	glGetBooleanv(GL_DEPTH_WRITEMASK, (unsigned char*)&a);
	return a;
}

bool
RageDisplay_Legacy::IsZTestEnabled() const
{
	GLenum a;
	glGetIntegerv(GL_DEPTH_FUNC, (GLint*)&a);
	return a != GL_ALWAYS;
}

void
RageDisplay_Legacy::ClearZBuffer()
{
	const auto write = IsZWriteEnabled();
	SetZWrite(true);
	glClear(GL_DEPTH_BUFFER_BIT);
	SetZWrite(write);
}

void
RageDisplay_Legacy::SetZWrite(bool b)
{
	glDepthMask(b);
}

void
RageDisplay_Legacy::SetZBias(float f)
{
	const auto fNear = SCALE(f, 0.0f, 1.0f, 0.05f, 0.0f);
	const auto fFar = SCALE(f, 0.0f, 1.0f, 1.0f, 0.95f);

	glDepthRange(fNear, fFar);
}

void
RageDisplay_Legacy::SetZTestMode(ZTestMode mode)
{
	glEnable(GL_DEPTH_TEST);
	switch (mode) {
		case ZTEST_OFF:
			glDepthFunc(GL_ALWAYS);
			break;
		case ZTEST_WRITE_ON_PASS:
			glDepthFunc(GL_LEQUAL);
			break;
		case ZTEST_WRITE_ON_FAIL:
			glDepthFunc(GL_GREATER);
			break;
		default:
			FAIL_M(ssprintf("Invalid ZTestMode: %i", mode));
	}
}

void
RageDisplay_Legacy::SetTextureWrapping(TextureUnit tu, bool b)
{
	/* This should be per-texture-unit state, but it's per-texture state in
	 * OpenGl, so we'll behave incorrectly if the same texture is used in more
	 * than one texture unit simultaneously with different wrapping. */
	SetTextureUnit(tu);

	const GLenum mode = b ? GL_REPEAT : GL_CLAMP_TO_EDGE;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode);
}

void
RageDisplay_Legacy::SetMaterial(const RageColor& emissive,
								const RageColor& ambient,
								const RageColor& diffuse,
								const RageColor& specular,
								float shininess)
{
	// TRICKY:  If lighting is off, then setting the material
	// will have no effect.  Even if lighting is off, we still
	// want Models to have basic color and transparency.
	// We can do this fake lighting by setting the vertex color.
	// XXX: unintended: SetLighting must be called before SetMaterial
	GLboolean bLighting;
	glGetBooleanv(GL_LIGHTING, &bLighting);

	if (bLighting) {
		glMaterialfv(GL_FRONT, GL_EMISSION, emissive);
		glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
		glMaterialf(GL_FRONT, GL_SHININESS, shininess);
	} else {
		auto c = diffuse;
		c.r += emissive.r + ambient.r;
		c.g += emissive.g + ambient.g;
		c.b += emissive.b + ambient.b;
		glColor4fv(c);
	}
}

void
RageDisplay_Legacy::SetLighting(bool b)
{
	if (b)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
}

void
RageDisplay_Legacy::SetLightOff(int index)
{
	glDisable(GL_LIGHT0 + index);
}

void
RageDisplay_Legacy::SetLightDirectional(int index,
										const RageColor& ambient,
										const RageColor& diffuse,
										const RageColor& specular,
										const RageVector3& dir)
{
	// Light coordinates are transformed by the modelview matrix, but
	// we are being passed in world-space coords.
	glPushMatrix();
	glLoadIdentity();

	glEnable(GL_LIGHT0 + index);
	glLightfv(GL_LIGHT0 + index, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0 + index, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0 + index, GL_SPECULAR, specular);
	float position[4] = { dir.x, dir.y, dir.z, 0 };
	glLightfv(GL_LIGHT0 + index, GL_POSITION, position);

	glPopMatrix();
}

void
RageDisplay_Legacy::SetCullMode(CullMode mode)
{
	if (mode != CULL_NONE)
		glEnable(GL_CULL_FACE);
	switch (mode) {
		case CULL_BACK:
			glCullFace(GL_BACK);
			break;
		case CULL_FRONT:
			glCullFace(GL_FRONT);
			break;
		case CULL_NONE:
			glDisable(GL_CULL_FACE);
			break;
		default:
			FAIL_M(ssprintf("Invalid CullMode: %i", mode));
	}
}

const RageDisplay::RagePixelFormatDesc*
RageDisplay_Legacy::GetPixelFormatDesc(RagePixelFormat pf) const
{
	ASSERT(pf < NUM_RagePixelFormat);
	return &PIXEL_FORMAT_DESC[pf];
}

bool
RageDisplay_Legacy::SupportsThreadedRendering()
{
	return g_pWind->SupportsThreadedRendering();
}

void
RageDisplay_Legacy::BeginConcurrentRenderingMainThread()
{
	g_pWind->BeginConcurrentRenderingMainThread();
}

void
RageDisplay_Legacy::EndConcurrentRenderingMainThread()
{
	g_pWind->EndConcurrentRenderingMainThread();
}

void
RageDisplay_Legacy::BeginConcurrentRendering()
{
	g_pWind->BeginConcurrentRendering();
	RageDisplay::BeginConcurrentRendering();
}

void
RageDisplay_Legacy::EndConcurrentRendering()
{
	g_pWind->EndConcurrentRendering();
}

void
RageDisplay_Legacy::DeleteTexture(intptr_t iTexture)
{
	if (iTexture == 0)
		return;

	if (g_mapRenderTargets.find(iTexture) != g_mapRenderTargets.end()) {
		delete g_mapRenderTargets[iTexture];
		g_mapRenderTargets.erase(iTexture);
		return;
	}

	DebugFlushGLErrors();
	glDeleteTextures(1, reinterpret_cast<GLuint*>(&iTexture));
	DebugAssertNoGLError();
}

RagePixelFormat
RageDisplay_Legacy::GetImgPixelFormat(RageSurface*& img,
									  bool& bFreeImg,
									  int width,
									  int height,
									  bool bPalettedTexture)
{
	auto pixfmt = FindPixelFormat(img->fmt.BitsPerPixel,
								  img->fmt.Rmask,
								  img->fmt.Gmask,
								  img->fmt.Bmask,
								  img->fmt.Amask);

	/* If img is paletted, we're setting up a non-paletted texture, and color
	 * indexes are too small, depalettize. */
	auto bSupported = true;
	if (!bPalettedTexture && img->fmt.BytesPerPixel == 1 &&
		!g_bColorIndexTableWorks)
		bSupported = false;

	if (pixfmt == RagePixelFormat_Invalid || !SupportsSurfaceFormat(pixfmt))
		bSupported = false;

	if (!bSupported) {
		/* The source isn't in a supported, known pixel format.  We need to
		 * convert it ourself.  Just convert it to RGBA8, and let OpenGL convert
		 * it back down to whatever the actual pixel format is.  This is a very
		 * slow code path, which should almost never be used. */
		pixfmt = RagePixelFormat_RGBA8;
		ASSERT(SupportsSurfaceFormat(pixfmt));

		auto pfd = DISPLAY->GetPixelFormatDesc(pixfmt);

		const auto imgconv = CreateSurface(img->w,
										   img->h,
										   pfd->bpp,
										   pfd->masks[0],
										   pfd->masks[1],
										   pfd->masks[2],
										   pfd->masks[3]);
		RageSurfaceUtils::Blit(img, imgconv, width, height);
		img = imgconv;
		bFreeImg = true;
	} else {
		bFreeImg = false;
	}

	return pixfmt;
}

/* If we're sending a paletted surface to a non-paletted texture, set the
 * palette. */
void
SetPixelMapForSurface(int glImageFormat,
					  int glTexFormat,
					  const RageSurfacePalette* palette)
{
	if (glImageFormat != GL_COLOR_INDEX || glTexFormat == GL_COLOR_INDEX8_EXT) {
		glPixelTransferi(GL_MAP_COLOR, false);
		return;
	}

	GLushort buf[4][256];
	memset(buf, 0, sizeof(buf));

	for (auto i = 0; i < palette->ncolors; ++i) {
		buf[0][i] = SCALE(palette->colors[i].r, 0, 255, 0, 65535);
		buf[1][i] = SCALE(palette->colors[i].g, 0, 255, 0, 65535);
		buf[2][i] = SCALE(palette->colors[i].b, 0, 255, 0, 65535);
		buf[3][i] = SCALE(palette->colors[i].a, 0, 255, 0, 65535);
	}

	DebugFlushGLErrors();
	glPixelMapusv(GL_PIXEL_MAP_I_TO_R, 256, buf[0]);
	glPixelMapusv(GL_PIXEL_MAP_I_TO_G, 256, buf[1]);
	glPixelMapusv(GL_PIXEL_MAP_I_TO_B, 256, buf[2]);
	glPixelMapusv(GL_PIXEL_MAP_I_TO_A, 256, buf[3]);
	glPixelTransferi(GL_MAP_COLOR, true);
	DebugAssertNoGLError();
}

intptr_t
RageDisplay_Legacy::CreateTexture(RagePixelFormat pixfmt,
								  RageSurface* pImg,
								  bool bGenerateMipMaps)
{
	ASSERT(pixfmt < NUM_RagePixelFormat);

	/* Find the pixel format of the surface we've been given. */
	bool bFreeImg;
	const auto SurfacePixFmt = GetImgPixelFormat(
	  pImg, bFreeImg, pImg->w, pImg->h, pixfmt == RagePixelFormat_PAL);
	ASSERT(SurfacePixFmt != RagePixelFormat_Invalid);

	const auto glTexFormat = g_GLPixFmtInfo[pixfmt].internalfmt;
	const auto glImageFormat = g_GLPixFmtInfo[SurfacePixFmt].format;
	const auto glImageType = g_GLPixFmtInfo[SurfacePixFmt].type;

	/* If the image is paletted, but we're not sending it to a paletted image,
	 * set up glPixelMap. */
	SetPixelMapForSurface(glImageFormat, glTexFormat, pImg->fmt.palette);

	// HACK:  OpenGL 1.2 types aren't available in GLU 1.3.  Don't call GLU for
	// mip mapping if we're using an OGL 1.2 type and don't have >= GLU 1.3.
	// http://pyopengl.sourceforge.net/documentation/manual/gluBuild2DMipmaps.3G.html
	if (bGenerateMipMaps && g_gluVersion < 13) {
		switch (pixfmt) {
			// OpenGL 1.1 types
			case RagePixelFormat_RGBA8:
			case RagePixelFormat_RGB8:
			case RagePixelFormat_PAL:
			case RagePixelFormat_BGR8:
				break;
			// OpenGL 1.2 types
			default:
				Locator::getLogger()->trace("Can't generate mipmaps for type {} because GLU "
						   "version {:.1f} is too old.",
						   GLToString(glImageType).c_str(),
						   g_gluVersion / 10.f);
				bGenerateMipMaps = false;
				break;
		}
	}

	SetTextureUnit(TextureUnit_1);

	// allocate OpenGL texture resource
	intptr_t iTexHandle;
	glGenTextures(1, reinterpret_cast<GLuint*>(&iTexHandle));
	ASSERT(iTexHandle != 0);

	glBindTexture(GL_TEXTURE_2D, iTexHandle);

	if ((*g_pWind->GetActualVideoModeParams()).bAnisotropicFiltering &&
		GLEW_EXT_texture_filter_anisotropic) {
		GLfloat fLargestSupportedAnisotropy;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,
					&fLargestSupportedAnisotropy);
		glTexParameterf(GL_TEXTURE_2D,
						GL_TEXTURE_MAX_ANISOTROPY_EXT,
						fLargestSupportedAnisotropy);
	}

	SetTextureFiltering(TextureUnit_1, true);
	SetTextureWrapping(TextureUnit_1, false);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, pImg->pitch / pImg->fmt.BytesPerPixel);

	if (pixfmt == RagePixelFormat_PAL) {
		/* The texture is paletted; set the texture palette. */
		GLubyte palette[256 * 4];
		memset(palette, 0, sizeof(palette));
		auto p = 0;
		/* Copy the palette to the format OpenGL expects. */
		for (auto i = 0; i < pImg->fmt.palette->ncolors; ++i) {
			palette[p++] = pImg->fmt.palette->colors[i].r;
			palette[p++] = pImg->fmt.palette->colors[i].g;
			palette[p++] = pImg->fmt.palette->colors[i].b;
			palette[p++] = pImg->fmt.palette->colors[i].a;
		}

		/* Set the palette. */
		glColorTableEXT(
		  GL_TEXTURE_2D, GL_RGBA8, 256, GL_RGBA, GL_UNSIGNED_BYTE, palette);

		auto iRealFormat = 0;
		glGetColorTableParameterivEXT(
		  GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT, &iRealFormat);
		ASSERT(iRealFormat == GL_RGBA8);
	}

	if (PREFSMAN->m_verbose_log > 1)
		Locator::getLogger()->trace(
		  "{} (format {}, {}x{}, format {}, type {}, pixfmt {}, imgpixfmt {})",
		  bGenerateMipMaps ? "gluBuild2DMipmaps" : "glTexImage2D",
		  GLToString(glTexFormat).c_str(),
		  pImg->w,
		  pImg->h,
		  GLToString(glImageFormat).c_str(),
		  GLToString(glImageType).c_str(),
		  pixfmt,
		  SurfacePixFmt);

	DebugFlushGLErrors();

	if (bGenerateMipMaps) {
		// We are not allowing creating mipmapped textures with empty buffers for now
		// TODO: Consider crashing when that happens or if we can/want to allow it
		// Would we need to update mipmaps manually in UpdateTexture?
		if (pImg->pixels) {
			glTexImage2D(GL_TEXTURE_2D,
						 0,
						 glTexFormat,
						 power_of_two(pImg->w),
						 power_of_two(pImg->h),
						 0,
						 glImageFormat,
						 glImageType,
						 pImg->pixels);

			glGenerateMipmap(GL_TEXTURE_2D);

			DebugAssertNoGLError();
		}
	} else {
		// if pImg->pixels is null glTexImage2D copies no data
		// but we still need to create the texture for the given size
		// An example of code which does this is background movies
		// which updates the image as it plays but doesn't initialize it
		// when creating the texture handle
		glTexImage2D(GL_TEXTURE_2D,
						0,
						glTexFormat,
						power_of_two(pImg->w),
						power_of_two(pImg->h),
						0,
						glImageFormat,
						glImageType,
						pImg->pixels);
		DebugAssertNoGLError();
	}

	/* Sanity check: */
	if (pixfmt == RagePixelFormat_PAL) {
		auto iSize = 0;
		glGetTexLevelParameteriv(
		  GL_TEXTURE_2D, 0, GLenum(GL_TEXTURE_INDEX_SIZE_EXT), &iSize);
		if (iSize != 8)
			RageException::Throw(
			  "Thought paletted textures worked, but they don't.");
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	if (bFreeImg)
		delete pImg;
	return iTexHandle;
}

struct RageTextureLock_OGL
  : public RageTextureLock
  , public InvalidateObject
{
  public:
	RageTextureLock_OGL()
	{
		m_iTexHandle = 0;
		m_iBuffer = 0;

		CreateObject();
	}

	~RageTextureLock_OGL() override
	{
		ASSERT(m_iTexHandle == 0); // locked!
		glDeleteBuffersARB(1, &m_iBuffer);
	}

	/* This is called when our OpenGL context is invalidated. */
	void Invalidate() override { m_iTexHandle = 0; }

	void Lock(unsigned iTexHandle, RageSurface* pSurface) override
	{
		ASSERT(m_iTexHandle == 0);
		ASSERT(pSurface->pixels == NULL);

		CreateObject();

		m_iTexHandle = iTexHandle;
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_iBuffer);

		const auto iSize = pSurface->h * pSurface->pitch;
		glBufferDataARB(
		  GL_PIXEL_UNPACK_BUFFER_ARB, iSize, nullptr, GL_STREAM_DRAW);

		const auto pSurfaceMemory =
		  glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
		pSurface->pixels = static_cast<uint8_t*>(pSurfaceMemory);
		pSurface->pixels_owned = false;
	}

	void Unlock(RageSurface* pSurface, bool bChanged) override
	{
		glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);

		pSurface->pixels = (uint8_t*)BUFFER_OFFSET(nullptr);

		if (bChanged)
			DISPLAY->UpdateTexture(
			  m_iTexHandle, pSurface, 0, 0, pSurface->w, pSurface->h);

		pSurface->pixels = nullptr;

		m_iTexHandle = 0;
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	}

  private:
	void CreateObject()
	{
		if (m_iBuffer != 0)
			return;

		DebugFlushGLErrors();
		glGenBuffersARB(1, &m_iBuffer);
		DebugAssertNoGLError();
	}

	GLuint m_iBuffer;

	unsigned m_iTexHandle;
};

RageTextureLock*
RageDisplay_Legacy::CreateTextureLock()
{
	if (!GLEW_ARB_pixel_buffer_object)
		return nullptr;

	return new RageTextureLock_OGL;
}

void
RageDisplay_Legacy::UpdateTexture(intptr_t iTexHandle,
								  RageSurface* pImg,
								  int iXOffset,
								  int iYOffset,
								  int iWidth,
								  int iHeight)
{
	glBindTexture(GL_TEXTURE_2D, iTexHandle);

	bool bFreeImg;
	const auto SurfacePixFmt =
	  GetImgPixelFormat(pImg, bFreeImg, iWidth, iHeight, false);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, pImg->pitch / pImg->fmt.BytesPerPixel);

	const auto glImageFormat = g_GLPixFmtInfo[SurfacePixFmt].format;
	const auto glImageType = g_GLPixFmtInfo[SurfacePixFmt].type;

	/* If the image is paletted, but we're not sending it to a paletted image,
	 * set up glPixelMap. */
	if (pImg->fmt.palette) {
		GLenum glTexFormat = 0;
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D,
								 0,
								 GLenum(GL_TEXTURE_INTERNAL_FORMAT),
								 (GLint*)&glTexFormat);
		SetPixelMapForSurface(glImageFormat, glTexFormat, pImg->fmt.palette);
	}

	glTexSubImage2D(GL_TEXTURE_2D,
					0,
					iXOffset,
					iYOffset,
					iWidth,
					iHeight,
					glImageFormat,
					glImageType,
					pImg->pixels);

	/* Must unset PixelStore when we're done! */
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glFlush();

	if (bFreeImg)
		delete pImg;
}

class RenderTarget_FramebufferObject : public RenderTarget
{
  public:
	RenderTarget_FramebufferObject();
	~RenderTarget_FramebufferObject() override;
	void Create(const RenderTargetParam& param,
				int& iTextureWidthOut,
				int& iTextureHeightOut) override;
	unsigned GetTexture() const override { return m_iTexHandle; }
	void StartRenderingTo() override;
	void FinishRenderingTo() override;

	bool InvertY() const override { return true; }

  private:
	unsigned int m_iFrameBufferHandle;
	unsigned int m_iTexHandle;
	unsigned int m_iDepthBufferHandle;
};

RenderTarget_FramebufferObject::RenderTarget_FramebufferObject()
{
	m_iFrameBufferHandle = 0;
	m_iTexHandle = 0;
	m_iDepthBufferHandle = 0;
}

RenderTarget_FramebufferObject::~RenderTarget_FramebufferObject()
{
	if (m_iDepthBufferHandle)
		glDeleteRenderbuffersEXT(
		  1, reinterpret_cast<GLuint*>(&m_iDepthBufferHandle));
	if (m_iFrameBufferHandle)
		glDeleteFramebuffersEXT(
		  1, reinterpret_cast<GLuint*>(&m_iFrameBufferHandle));
	if (m_iTexHandle)
		glDeleteTextures(1, reinterpret_cast<GLuint*>(&m_iTexHandle));
}

void
RenderTarget_FramebufferObject::Create(const RenderTargetParam& param,
									   int& iTextureWidthOut,
									   int& iTextureHeightOut)
{
	m_Param = param;

	DebugFlushGLErrors();

	// Allocate OpenGL texture resource
	glGenTextures(1, reinterpret_cast<GLuint*>(&m_iTexHandle));
	ASSERT(m_iTexHandle != 0);

	const auto iTextureWidth = power_of_two(param.iWidth);
	const auto iTextureHeight = power_of_two(param.iHeight);

	iTextureWidthOut = iTextureWidth;
	iTextureHeightOut = iTextureHeight;

	glBindTexture(GL_TEXTURE_2D, m_iTexHandle);
	GLenum internalformat;
	const GLenum type = param.bWithAlpha ? GL_RGBA : GL_RGB;
	if (param.bFloat && GLEW_ARB_texture_float)
		internalformat = param.bWithAlpha ? GL_RGBA16F_ARB : GL_RGB16F_ARB;
	else
		internalformat = param.bWithAlpha ? GL_RGBA8 : GL_RGB8;

	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 internalformat,
				 iTextureWidth,
				 iTextureHeight,
				 0,
				 type,
				 GL_UNSIGNED_BYTE,
				 nullptr);
	DebugAssertNoGLError();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	/* Create the framebuffer object. */
	glGenFramebuffersEXT(1, reinterpret_cast<GLuint*>(&m_iFrameBufferHandle));
	ASSERT(m_iFrameBufferHandle != 0);

	/* Attach the texture to it. */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_iFrameBufferHandle);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
							  GL_COLOR_ATTACHMENT0_EXT,
							  GL_TEXTURE_2D,
							  m_iTexHandle,
							  0);
	DebugAssertNoGLError();

	/* Attach a depth buffer, if requested. */
	if (param.bWithDepthBuffer) {
		glGenRenderbuffersEXT(1,
							  reinterpret_cast<GLuint*>(&m_iDepthBufferHandle));
		ASSERT(m_iDepthBufferHandle != 0);

		glBindRenderbufferEXT(GL_RENDERBUFFER, m_iDepthBufferHandle);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
								 GL_DEPTH_COMPONENT16,
								 iTextureWidth,
								 iTextureHeight);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
									 GL_DEPTH_ATTACHMENT_EXT,
									 GL_RENDERBUFFER_EXT,
									 m_iDepthBufferHandle);
	}

	const auto status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch (status) {
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			FAIL_M("GL_FRAMEBUFFER_UNSUPPORTED_EXT");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			FAIL_M("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			FAIL_M("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			FAIL_M("GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			FAIL_M("GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			FAIL_M("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			FAIL_M("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT");
			break;
		default:
			FAIL_M(ssprintf("Unexpected GL framebuffer status: %i", status));
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void
RenderTarget_FramebufferObject::StartRenderingTo()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_iFrameBufferHandle);
}

void
RenderTarget_FramebufferObject::FinishRenderingTo()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

bool
RageDisplay_Legacy::SupportsRenderToTexture() const
{
	return GLEW_EXT_framebuffer_object || g_pWind->SupportsRenderToTexture();
}

bool
RageDisplay_Legacy::SupportsFullscreenBorderlessWindow() const
{
	// In order to support FSBW, we're going to need the LowLevelWindow
	// implementation to support creating a fullscreen borderless window, and
	// we're going to need RenderToTexture support in order to render in
	// alternative resolutions
	return g_pWind->SupportsFullscreenBorderlessWindow() &&
		   SupportsRenderToTexture();
}

/*
 * Render-to-texture can be implemented in several ways: the generic
 * GL_ARB_pixel_buffer_object, or platform-specifically.  PBO is not available
 * on all hardware that supports RTT, particularly GeForce 2, but is simpler and
 * faster when available.
 */

intptr_t
RageDisplay_Legacy::CreateRenderTarget(const RenderTargetParam& param,
									   int& iTextureWidthOut,
									   int& iTextureHeightOut)
{
	RenderTarget* pTarget;
	if (GLEW_EXT_framebuffer_object)
		pTarget = new RenderTarget_FramebufferObject;
	else
		pTarget = g_pWind->CreateRenderTarget();

	intptr_t iTexture = 0;
	if (pTarget) {
		pTarget->Create(param, iTextureWidthOut, iTextureHeightOut);

		iTexture = pTarget->GetTexture();

		ASSERT(g_mapRenderTargets.find(iTexture) == g_mapRenderTargets.end());
		g_mapRenderTargets[iTexture] = pTarget;
	}

	return iTexture;
}

intptr_t
RageDisplay_Legacy::GetRenderTarget()
{
	for (const auto& g_mapRenderTarget : g_mapRenderTargets)
		if (g_mapRenderTarget.second == g_pCurrentRenderTarget)
			return g_mapRenderTarget.first;
	return 0;
}

void
RageDisplay_Legacy::SetRenderTarget(intptr_t iTexture, bool bPreserveTexture)
{
	if (iTexture == 0) {
		g_bInvertY = false;
		glFrontFace(GL_CCW);

		/* Pop matrixes affected by SetDefaultRenderStates. */
		DISPLAY->CameraPopMatrix();

		/* Reset the viewport. */
		const auto fWidth = (*g_pWind->GetActualVideoModeParams()).windowWidth;
		const auto fHeight =
		  (*g_pWind->GetActualVideoModeParams()).windowHeight;
		glViewport(0, 0, fWidth, fHeight);

		if (g_pCurrentRenderTarget != nullptr)
			g_pCurrentRenderTarget->FinishRenderingTo();
		g_pCurrentRenderTarget = nullptr;
		return;
	}

	/* If we already had a render target, disable it. */
	if (g_pCurrentRenderTarget != nullptr)
		SetRenderTarget(0, true);

	/* Enable the new render target. */
	ASSERT(g_mapRenderTargets.find(iTexture) != g_mapRenderTargets.end());
	auto pTarget = g_mapRenderTargets[iTexture];
	pTarget->StartRenderingTo();
	g_pCurrentRenderTarget = pTarget;

	/* Set the viewport to the size of the render target. */
	glViewport(0, 0, pTarget->GetParam().iWidth, pTarget->GetParam().iHeight);

	/* If this render target implementation flips Y, compensate.   Inverting
	 * will switch the winding order. */
	g_bInvertY = pTarget->InvertY();
	if (g_bInvertY)
		glFrontFace(GL_CW);

	/* The render target may be in a different OpenGL context, so re-send
	 * state.  Push matrixes affected by SetDefaultRenderStates. */
	DISPLAY->CameraPushMatrix();
	SetDefaultRenderStates();

	/* Clear the texture, if requested.  Always set the associated state, for
	 * consistency. */
	glClearColor(0, 0, 0, 0);
	SetZWrite(true);

	/* If bPreserveTexture is false, clear the render target.  Only clear the
	 * depth buffer if the target has one; otherwise we're clearing the real
	 * depth buffer. */
	if (!bPreserveTexture) {
		auto iBit = GL_COLOR_BUFFER_BIT;
		if (pTarget->GetParam().bWithDepthBuffer)
			iBit |= GL_DEPTH_BUFFER_BIT;
		glClear(iBit);
	}
}

void
RageDisplay_Legacy::SetPolygonMode(PolygonMode pm)
{
	GLenum m;
	switch (pm) {
		case POLYGON_FILL:
			m = GL_FILL;
			break;
		case POLYGON_LINE:
			m = GL_LINE;
			break;
		default:
			FAIL_M(ssprintf("Invalid PolygonMode: %i", pm));
	}
	glPolygonMode(GL_FRONT_AND_BACK, m);
}

void
RageDisplay_Legacy::SetLineWidth(float fWidth)
{
	glLineWidth(fWidth);
}

std::string
RageDisplay_Legacy::GetTextureDiagnostics(unsigned iTexture) const
{
	/*
		s << (bGenerateMipMaps? "gluBuild2DMipmaps":"glTexImage2D");
		s << "(format " << GLToString(glTexFormat) <<
				", " << pImg->w << "x" <<  pImg->h <<
				", format " << GLToString(iFormat) <<
				", type " << GLToString(glImageType) <<
				", pixfmt " << pixfmt <<
				", imgpixfmt " << SurfacePixFmt <<
				")";
		LOG->Trace( "%s", s.str().c_str() );

	glBindTexture( GL_TEXTURE_2D, iTexture );

		GLint iWidth;
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GLenum(GL_TEXTURE_WIDTH),
	(GLint *) &iWidth ); GLint iHeight; glGetTexLevelParameteriv( GL_TEXTURE_2D,
	0, GLenum(GL_TEXTURE_HEIGHT), (GLint *) &iHeight ); GLint iFormat;
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0,
	GLenum(GL_TEXTURE_INTERNAL_FORMAT), (GLint *) &iFormat );

		GL_CHECK_ERROR( "glGetTexLevelParameteriv(GL_TEXTURE_INTERNAL_FORMAT)"
	); if (iFormat != glTexFormat)
		{
			sError = ssprintf( "Expected format %s, got %s instead",
					GLToString(glTexFormat).c_str(), GLToString(iFormat).c_str()
	); break;
		}
*/
	return std::string();
}

/*
 * XXX: Things like this only have to be set once per context - making
 * SetDefault call These kinds of functions is wasteful. -Colby
 */
void
RageDisplay_Legacy::SetAlphaTest(bool b)
{
	// Previously this was 0.01, rather than 0x01.
	glAlphaFunc(GL_GREATER, 0.00390625 /* 1/256 */);
	if (b)
		glEnable(GL_ALPHA_TEST);
	else
		glDisable(GL_ALPHA_TEST);
}

/*
 * Although we pair texture formats (eg. GL_RGB8) and surface formats
 * (pairs of eg. GL_RGB8,GL_UNSIGNED_SHORT_5_5_5_1), it's possible for
 * a format to be supported for a texture format but not a surface
 * format.  This is abstracted, so you don't need to know about this
 * as a user calling CreateTexture.
 *
 * One case of this is if packed pixels aren't supported.  We can still
 * use 16-bit color modes, but we have to send it in 32-bit.  Almost
 * everything supports packed pixels.
 *
 * Another case of this is incomplete packed pixels support.  Some
 * implementations neglect GL_UNSIGNED_SHORT_*_REV.
 */
bool
RageDisplay_Legacy::SupportsSurfaceFormat(RagePixelFormat pixfmt)
{
	switch (g_GLPixFmtInfo[pixfmt].type) {
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			return GLEW_EXT_bgra && g_bReversePackedPixelsWorks;
		default:
			return true;
	}
}

bool
RageDisplay_Legacy::SupportsTextureFormat(RagePixelFormat pixfmt,
										  bool bRealtime)
{
	/* If we support a pixfmt for texture formats but not for surface formats,
	 * then we'll have to convert the texture to a supported surface format
	 * before uploading. This is too slow for dynamic textures. */
	if (bRealtime && !SupportsSurfaceFormat(pixfmt))
		return false;

	switch (g_GLPixFmtInfo[pixfmt].format) {
		case GL_COLOR_INDEX:
			return glColorTableEXT && glGetColorTableParameterivEXT;
		case GL_BGR:
		case GL_BGRA:
			return !!GLEW_EXT_bgra;
		default:
			return true;
	}
}

bool
RageDisplay_Legacy::SupportsPerVertexMatrixScale()
{
	// Intel i915 on OSX 10.4.4 supports vertex programs but not hardware vertex
	// buffers. Our software vertex rendering doesn't support vertex programs.
	return glGenBuffersARB && g_bTextureMatrixShader != 0;
}

void
RageDisplay_Legacy::SetSphereEnvironmentMapping(TextureUnit tu, bool b)
{
	if (!SetTextureUnit(tu))
		return;

	if (b) {
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
	} else {
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}
}

GLint iCelTexture1, iCelTexture2 = 0;

void
RageDisplay_Legacy::SetCelShaded(int stage)
{
	if (!GLEW_ARB_fragment_program && !GL_ARB_shading_language_100)
		return; // not supported

	switch (stage) {
		case 1:
			glUseProgramObjectARB(g_gShellShader);
			break;
		case 2:
			glUseProgramObjectARB(g_gCelShader);
			break;
		default:
			glUseProgramObjectARB(0);
			break;
	}
}

bool
RageDisplay_Legacy::IsD3DInternal()
{
	return false;
}
