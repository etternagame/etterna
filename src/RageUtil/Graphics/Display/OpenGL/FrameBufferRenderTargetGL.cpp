#include "FrameBufferRenderTargetGL.hpp"

#include "Etterna/Globals/global.h"

RenderTarget_FramebufferObject::RenderTarget_FramebufferObject() {
	m_iFrameBufferHandle = 0;
	m_iTexHandle = 0;
	m_iDepthBufferHandle = 0;
}

RenderTarget_FramebufferObject::~RenderTarget_FramebufferObject() {
	if (m_iDepthBufferHandle) glDeleteRenderbuffersEXT(1, reinterpret_cast<GLuint*>(&m_iDepthBufferHandle));
	if (m_iFrameBufferHandle) glDeleteFramebuffersEXT(1, reinterpret_cast<GLuint*>(&m_iFrameBufferHandle));
	if (m_iTexHandle) glDeleteTextures(1, reinterpret_cast<GLuint*>(&m_iTexHandle));
}

void RenderTarget_FramebufferObject::Create(const RenderTargetParam& param, int& iTextureWidthOut,
                                            int& iTextureHeightOut) {
	m_Param = param;

//	DebugFlushGLErrors();

	// Allocate OpenGL texture resource
	glGenTextures(1, reinterpret_cast<GLuint*>(&m_iTexHandle));
//	ASSERT(m_iTexHandle != 0);

	const auto iTextureWidth = power_of_two(param.iWidth);
	const auto iTextureHeight = power_of_two(param.iHeight);

	iTextureWidthOut = iTextureWidth;
	iTextureHeightOut = iTextureHeight;

	glBindTexture(GL_TEXTURE_2D, m_iTexHandle);
	GLenum internalformat;
	const GLenum type = param.bWithAlpha ? GL_RGBA : GL_RGB;
	if (param.bFloat && GLAD_GL_ARB_texture_float){
        internalformat = param.bWithAlpha ? GL_RGBA16F_ARB : GL_RGB16F_ARB;
	} else {
        internalformat = param.bWithAlpha ? GL_RGBA8 : GL_RGB8;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, iTextureWidth, iTextureHeight, 0, type, GL_UNSIGNED_BYTE, nullptr);
//	DebugAssertNoGLError();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	/* Create the framebuffer object. */
	glGenFramebuffersEXT(1, reinterpret_cast<GLuint*>(&m_iFrameBufferHandle));
//	ASSERT(m_iFrameBufferHandle != 0);

	/* Attach the texture to it. */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_iFrameBufferHandle);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_iTexHandle, 0);
//	DebugAssertNoGLError();

	/* Attach a depth buffer, if requested. */
	if (param.bWithDepthBuffer) {
		glGenRenderbuffersEXT(1, reinterpret_cast<GLuint*>(&m_iDepthBufferHandle));
//		ASSERT(m_iDepthBufferHandle != 0);

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

void RenderTarget_FramebufferObject::StartRenderingTo() {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_iFrameBufferHandle);
}

void RenderTarget_FramebufferObject::FinishRenderingTo() {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}
