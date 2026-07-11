#include "RageAnimatedTexture.h"
#include "RageDisplay.h"
#include <stb/stb_image.h>
#include <RageUtil/File/RageFile.h>
#include <Core/Services/Locator.hpp>
#include "RageSurface.h"
#include "RageSurfaceUtils_Zoom.h"
#include "RageTextureManager.h"

RageAnimatedTexture::RageAnimatedTexture(const RageTextureID& id)
  : RageTexture(id)
  , m_CurrentFrame{ 0 }
  , m_TextureHandle{ 0 }
  , m_Pixels{ nullptr }
  , m_Delays{ nullptr }
  , m_Time{ 0.0f }

{
	Create();
}

RageAnimatedTexture::~RageAnimatedTexture()
{
	Destroy();
}

void
RageAnimatedTexture::Reload()
{
	Destroy();
	Create();
}

void
RageAnimatedTexture::Update(float time)
{
	if (m_Surfaces.size() == 1) {
		return;
	}

	m_Time += time;

	int delay = m_Delays[m_CurrentFrame];
	if (delay < 20) {
		delay = 100;
	}

	float currentDelay = delay / 1000.0f;
	m_Time += time;

	bool frameChanged = false;
	while (m_Time >= currentDelay) {
		m_Time -= currentDelay;
		m_CurrentFrame = (m_CurrentFrame + 1) % m_iFramesHigh;

		delay = m_Delays[m_CurrentFrame];
		if (delay < 20) {
			delay = 100;
		}

		currentDelay = delay / 1000.0f;

		frameChanged = true;
	}

	if (!frameChanged) {
		return;
	}

	DISPLAY->UpdateTexture(m_TextureHandle,
						   m_Surfaces[m_CurrentFrame],
						   0,
						   0,
						   m_iImageWidth,
						   m_iImageHeight);
}

void
RageAnimatedTexture::Create()
{
	auto& id = GetID();

	RageFile f;
	if (!f.Open(id.filename)) {
		auto error = f.GetError();
		Locator::getLogger()->error(
		  "Failed to load RageAnimatedTexture {}: {}", id.filename, error);
		throw std::runtime_error(error);
	}

	std::vector<stbi_uc> buffer(f.GetFileSize());
	f.Read(&buffer[0], f.GetFileSize());

	int width = 0;
	int height = 0;
	int frames = 0;
	int comp = 0;

	m_Pixels = stbi_load_gif_from_memory(
	  &buffer[0], buffer.size(), &m_Delays, &width, &height, &frames, &comp, 4);

	if (m_Pixels == nullptr) {
		auto error = stbi_failure_reason();
		Locator::getLogger()->error(
		  "Failed to load RageAnimatedTexture {}: {}", id.filename, error);
		throw std::runtime_error(error);
	}

	size_t frameSize = static_cast<size_t>(width) * height * 4;
	for (int i = 0; i < frames; i++) {
		stbi_uc* frame = m_Pixels + i * frameSize;

		auto surface = CreateSurfaceFrom(width,
										 height,
										 32,
										 Swap32BE(0xFF000000),
										 Swap32BE(0x00FF0000),
										 Swap32BE(0x0000FF00),
										 Swap32BE(0x000000FF),
										 frame,
										 width * 4);
		if (surface == nullptr) {
			auto error = stbi_failure_reason();
			stbi_image_free(m_Pixels);
			Locator::getLogger()->error(
			  "Failed to load RageAnimatedTexture {}: {}", id.filename, error);
			throw std::runtime_error(error);
		}

		surface->stb_loadpoint = false;
		surface->pixels_owned = false;

		m_Surfaces.push_back(surface);
	}

	m_TextureHandle =
	  DISPLAY->CreateTexture(RagePixelFormat_RGBA8, m_Surfaces[0], false);

	m_iSourceWidth = width;
	m_iSourceHeight = height;
	m_iImageWidth = m_iSourceWidth;
	m_iImageHeight = m_iSourceHeight;

	m_TextureCoordRects.clear();

	for (int i = 0; i < m_Surfaces.size(); i++) {
		m_TextureCoordRects.push_back(RectF(0, 0, 1, 1));
	}

	m_iFramesHigh = frames;
	m_iFramesWide = 1;
	TEXTUREMAN->RegisterTextureForUpdating(id, this);
}

void
RageAnimatedTexture::Destroy()
{
	for (auto& surface : m_Surfaces) {
		delete surface;
	}
	m_Surfaces.clear();
	m_Time = 0;
	m_CurrentFrame = 0;

	if (m_Pixels != nullptr) {
		stbi_image_free(m_Pixels);
		m_Pixels = nullptr;
	}

	if (m_Delays != nullptr) {
		delete m_Delays;
		m_Delays = nullptr;
	}

	if (DISPLAY != nullptr) {
		DISPLAY->DeleteTexture(m_TextureHandle);
		m_TextureHandle = 0;
	}
}
