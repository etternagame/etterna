#ifndef HOLD_JUDGMENT_H
#define HOLD_JUDGMENT_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/PlayerNumber.h"

class HoldJudgment : public ActorFrame
{
  public:
	HoldJudgment();
	[[nodiscard]] auto Copy() const -> HoldJudgment* override;
	void Load(const std::string& sPath);
	void LoadFromNode(const XNode* pNode) override;

	void SetHoldJudgment(HoldNoteScore hns);
	void LoadFromMultiPlayer(MultiPlayer mp);
	void HandleMessage(const Message& msg) override;

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	void ResetAnimation();
	AutoActor m_sprJudgment;
	MultiPlayer m_mpToTrack;
};

#endif
