#include "Etterna/Globals/global.h"

#include "RageTexture.h"
#include "RageUtil/Utils/RageUtil.h"

RageTexture::RageTexture(const RageTextureID& name)
  : m_iRefCount(1)
  , m_bWasUsed(false)
  , m_ID(name)
  , m_iSourceWidth(0)
  , m_iSourceHeight(0)
  , m_iTextureWidth(0)
  , m_iTextureHeight(0)
  , m_iImageWidth(0)
  , m_iImageHeight(0)
  , m_iFramesWide(1)
  , m_iFramesHigh(1)
{
}

RageTexture::~RageTexture() = default;

void
RageTexture::CreateFrameRects()
{
	GetFrameDimensionsFromFileName(GetID().filename,
								   &m_iFramesWide,
								   &m_iFramesHigh,
								   m_iSourceWidth,
								   m_iSourceHeight);

	// Fill in the m_FrameRects with the bounds of each frame in the animation.
	m_TextureCoordRects.clear();

	for (auto j = 0; j < m_iFramesHigh; j++) // traverse along Y
	{
		for (auto i = 0; i < m_iFramesWide;
			 i++) // traverse along X (important that this is the inner loop)
		{
			RectF frect(
			  (i + 0) / static_cast<float>(m_iFramesWide) * m_iImageWidth /
				static_cast<float>(
				  m_iTextureWidth), // these will all be between 0.0 and 1.0
			  (j + 0) / static_cast<float>(m_iFramesHigh) * m_iImageHeight /
				static_cast<float>(m_iTextureHeight),
			  (i + 1) / static_cast<float>(m_iFramesWide) * m_iImageWidth /
				static_cast<float>(m_iTextureWidth),
			  (j + 1) / static_cast<float>(m_iFramesHigh) * m_iImageHeight /
				static_cast<float>(m_iTextureHeight));
			m_TextureCoordRects.push_back(frect); // the index of this array
												  // element will be (i +
												  // j*m_iFramesWide)

			// LOG->Trace( "Adding frect%d %f %f %f %f", (i + j*m_iFramesWide),
			// frect.left, frect.top, frect.right, frect.bottom );
		}
	}
}

void
RageTexture::GetFrameDimensionsFromFileName(const std::string& sPath,
											int* piFramesWide,
											int* piFramesHigh,
											int source_width,
											int source_height)
{
	static Regex match(" ([0-9]+)x([0-9]+)([\\. ]|$)");
	vector<std::string> asMatch;
	if (!match.Compare(sPath, asMatch)) {
		*piFramesWide = *piFramesHigh = 1;
		return;
	}
	// Check for nonsense values.  Some people might not intend the hint. -Kyz
	const auto maybe_width = StringToInt(asMatch[0]);
	const auto maybe_height = StringToInt(asMatch[1]);
	if (maybe_width <= 0 || maybe_height <= 0) {
		*piFramesWide = *piFramesHigh = 1;
		return;
	}
	// Font.cpp uses this function, but can't pass in a texture size.  Other
	// textures can pass in a size though, and having more frames than pixels
	// makes no sense. -Kyz
	if (source_width > 0 && source_height > 0) {
		if (maybe_width > source_width || maybe_height > source_height) {
			*piFramesWide = *piFramesHigh = 1;
			return;
		}
	}
	*piFramesWide = maybe_width;
	*piFramesHigh = maybe_height;
}

const RectF*
RageTexture::GetTextureCoordRect(int iFrameNo) const
{
	return &m_TextureCoordRects[iFrameNo % GetNumFrames()];
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the RageTexture. */
class LunaRageTexture : public Luna<RageTexture>
{
  public:
	static int position(T* p, lua_State* L)
	{
		p->SetPosition(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int loop(T* p, lua_State* L)
	{
		p->SetLooping(BIArg(1));
		COMMON_RETURN_SELF;
	}
	static int rate(T* p, lua_State* L)
	{
		p->SetPlaybackRate(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int GetTextureCoordRect(T* p, lua_State* L)
	{
		const auto pRect = p->GetTextureCoordRect(IArg(1));
		lua_pushnumber(L, pRect->left);
		lua_pushnumber(L, pRect->top);
		lua_pushnumber(L, pRect->right);
		lua_pushnumber(L, pRect->bottom);
		return 4;
	}
	static int GetNumFrames(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNumFrames());
		return 1;
	}
	static int Reload(T* p, lua_State* L)
	{
		p->Reload();
		COMMON_RETURN_SELF;
	}
	DEFINE_METHOD(GetSourceWidth, GetSourceWidth());
	DEFINE_METHOD(GetSourceHeight, GetSourceHeight());
	DEFINE_METHOD(GetTextureWidth, GetTextureWidth());
	DEFINE_METHOD(GetTextureHeight, GetTextureHeight());
	DEFINE_METHOD(GetImageWidth, GetImageWidth());
	DEFINE_METHOD(GetImageHeight, GetImageHeight());
	DEFINE_METHOD(GetPath, GetID().filename);

	LunaRageTexture()
	{
		ADD_METHOD(position);
		ADD_METHOD(loop);
		ADD_METHOD(rate);
		ADD_METHOD(GetTextureCoordRect);
		ADD_METHOD(GetNumFrames);
		ADD_METHOD(Reload);
		ADD_METHOD(GetSourceWidth);
		ADD_METHOD(GetSourceHeight);
		ADD_METHOD(GetTextureWidth);
		ADD_METHOD(GetTextureHeight);
		ADD_METHOD(GetImageWidth);
		ADD_METHOD(GetImageHeight);
		ADD_METHOD(GetPath);
	}
};

LUA_REGISTER_CLASS(RageTexture)
// lua end

class LunaRTPtrContainer : public Luna<RTPtrContainer>
{
public:

	static int position(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		p->handle->SetPosition(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int loop(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		p->handle->SetLooping(BIArg(1));
		COMMON_RETURN_SELF;
	}
	static int rate(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		p->handle->SetPlaybackRate(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int GetTextureCoordRect(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		const auto pRect = p->handle->GetTextureCoordRect(IArg(1));
		lua_pushnumber(L, pRect->left);
		lua_pushnumber(L, pRect->top);
		lua_pushnumber(L, pRect->right);
		lua_pushnumber(L, pRect->bottom);
		return 4;
	}
	static int GetNumFrames(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		lua_pushnumber(L, p->handle->GetNumFrames());
		return 1;
	}
	static int Reload(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		p->handle->Reload();
		COMMON_RETURN_SELF;
	}
	static int GetSourceWidth(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		lua_pushnumber(L, p->handle->GetSourceWidth());
		return 1;
	}
	static int GetSourceHeight(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		lua_pushnumber(L, p->handle->GetSourceHeight());
		return 1;
	}
	static int GetTextureWidth(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		lua_pushnumber(L, p->handle->GetTextureWidth());
		return 1;
	}
	static int GetTextureHeight(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		lua_pushnumber(L, p->handle->GetTextureHeight());
		return 1;
	}
	static int GetImageWidth(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		lua_pushnumber(L, p->handle->GetImageWidth());
		return 1;
	}
	static int GetImageHeight(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		lua_pushnumber(L, p->handle->GetImageHeight());
		return 1;
	}
	static int GetPath(T* p, lua_State* L)
	{
		if (p->handle == nullptr) {
			lua_pushnil(L);
			return 1;
		}
		lua_pushstring(L, p->handle->GetID().filename.c_str());
		return 1;
	}
	
	LunaRTPtrContainer()
	{
		ADD_METHOD(position);
		ADD_METHOD(loop);
		ADD_METHOD(rate);
		ADD_METHOD(GetTextureCoordRect);
		ADD_METHOD(GetNumFrames);
		ADD_METHOD(Reload);
		ADD_METHOD(GetSourceWidth);
		ADD_METHOD(GetSourceHeight);
		ADD_METHOD(GetTextureWidth);
		ADD_METHOD(GetTextureHeight);
		ADD_METHOD(GetImageWidth);
		ADD_METHOD(GetImageHeight);
		ADD_METHOD(GetPath);
	}
};
LUA_REGISTER_CLASS(RTPtrContainer)
