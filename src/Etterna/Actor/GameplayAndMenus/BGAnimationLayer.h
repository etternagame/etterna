#ifndef BGANIMATIONLAYER_H
#define BGANIMATIONLAYER_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"

class XNode;

/** @brief Layer elements used by BGAnimation. */
class BGAnimationLayer : public ActorFrame
{
  public:
	BGAnimationLayer();
	~BGAnimationLayer() override;

	void LoadFromAniLayerFile(const std::string& sPath);
	void LoadFromNode(const XNode* pNode) override;

	void UpdateInternal(float fDeltaTime) override;

	float GetMaxTweenTimeLeft() const;

  protected:
	vector<RageVector3> m_vParticleVelocity;

	enum Type
	{
		TYPE_SPRITE,
		TYPE_PARTICLES,
		TYPE_TILES,
		NUM_TYPES,
	} m_Type;

	// stretch stuff
	float m_fTexCoordVelocityX;
	float m_fTexCoordVelocityY;

	// particles stuff
	bool m_bParticlesBounce;

	// tiles stuff
	int m_iNumTilesWide;
	int m_iNumTilesHigh;
	float m_fTilesStartX;
	float m_fTilesStartY;
	float m_fTilesSpacingX;
	float m_fTilesSpacingY;
	float m_fTileVelocityX;
	float m_fTileVelocityY;
};

#endif
