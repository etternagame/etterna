#ifndef RAGE_DISPLAY_OGL_HELPERS_H
#define RAGE_DISPLAY_OGL_HELPERS_H

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glew.h>

/* Import RageDisplay, for types.  Do not include RageDisplay_Legacy.h. */
#include "RageDisplay.h"

/* Windows defines GL_EXT_paletted_texture incompletely: */
#ifndef GL_TEXTURE_INDEX_SIZE_EXT
#define GL_TEXTURE_INDEX_SIZE_EXT 0x80ED
#endif

/** @brief Utilities for working with the RageDisplay. */
namespace RageDisplay_Legacy_Helpers {
void
Init();
std::string
GLToString(GLenum e);
};

class RenderTarget
{
  public:
	virtual ~RenderTarget() = default;
	virtual void Create(const RenderTargetParam& param,
						int& iTextureWidthOut,
						int& iTextureHeightOut) = 0;

	virtual unsigned GetTexture() const = 0;

	/* Render to this RenderTarget. */
	virtual void StartRenderingTo() = 0;

	/* Stop rendering to this RenderTarget.  Update the texture, if necessary,
	 * and make it available. */
	virtual void FinishRenderingTo() = 0;

	virtual bool InvertY() const { return false; }

	const RenderTargetParam& GetParam() const { return m_Param; }

  protected:
	RenderTargetParam m_Param;
};

#endif
