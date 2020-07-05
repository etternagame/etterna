#ifndef BGANIMATION_H
#define BGANIMATION_H

#include "Etterna/Actor/Base/ActorFrame.h"

class XNode;

/** @brief An ActorFrame that loads itself. */
class BGAnimation : public ActorFrameAutoDeleteChildren
{
  public:
	BGAnimation();
	~BGAnimation() override;

	void LoadFromAniDir(const std::string& sAniDir);
	void LoadFromNode(const XNode* pNode) override;

	BGAnimation* Copy() const override;

  protected:
	void AddLayersFromAniDir(const std::string& _sAniDir, const XNode* pNode);
};

#endif
