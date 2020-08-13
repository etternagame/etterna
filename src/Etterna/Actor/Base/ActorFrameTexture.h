#ifndef ACTOR_FRAME_TEXTURE_H
#define ACTOR_FRAME_TEXTURE_H

#include "ActorFrame.h"
class RageTextureRenderTarget;

class ActorFrameTexture : public ActorFrame
{
  public:
	ActorFrameTexture();
	ActorFrameTexture(const ActorFrameTexture& cpy);
	~ActorFrameTexture() override;
	[[nodiscard]] ActorFrameTexture* Copy() const override;

	/**
	 * @brief Set the texture name.
	 *
	 * This can be used with RageTextureManager (and users, eg. Sprite)
	 * to load the texture.  If no name is supplied, a unique one will
	 * be generated.  In that case, the only way to access the texture
	 * is via GetTextureName.
	 * @param sName the new name. */
	void SetTextureName(const std::string& sName) { m_sTextureName = sName; }
	/**
	 * @brief Retrieve the texture name.
	 * @return the texture name. */
	[[nodiscard]] std::string GetTextureName() const { return m_sTextureName; }
	[[nodiscard]] std::shared_ptr<RageTextureRenderTarget> GetTexture() const
	{
		return m_pRenderTarget;
	}

	void EnableDepthBuffer(bool b) { m_bDepthBuffer = b; }
	void EnableAlphaBuffer(bool b) { m_bAlphaBuffer = b; }
	void EnableFloat(bool b) { m_bFloat = b; }
	void EnablePreserveTexture(bool b) { m_bPreserveTexture = b; }

	void Create();

	void DrawPrimitives() override;

	// Commands
	void PushSelf(lua_State* L) override;

  private:
	std::shared_ptr<RageTextureRenderTarget> m_pRenderTarget;

	bool m_bDepthBuffer;
	bool m_bAlphaBuffer;
	bool m_bFloat;
	bool m_bPreserveTexture;
	/** @brief the name of this ActorFrameTexture. */
	std::string m_sTextureName;
};

class ActorFrameTextureAutoDeleteChildren : public ActorFrameTexture
{
  public:
	ActorFrameTextureAutoDeleteChildren() { DeleteChildrenWhenDone(true); }
	[[nodiscard]] bool AutoLoadChildren() const override { return true; }
	[[nodiscard]] ActorFrameTextureAutoDeleteChildren* Copy() const override;
};

#endif
