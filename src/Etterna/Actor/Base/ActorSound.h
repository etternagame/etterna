#ifndef ACTOR_SOUND_H
#define ACTOR_SOUND_H

#include "Actor.h"
#include "RageUtil/Sound/RageSound.h"
/** @brief RageSound Actor interface. */
class ActorSound : public Actor
{
  public:
	ActorSound()

	  = default;
	~ActorSound() override = default;
	ActorSound* Copy() const override;

	void Load(const std::string& sPath);
	void Play();
	void Pause(bool bPause);
	void Stop();
	void Update(float) override;
	void LoadFromNode(const XNode* pNode) override;
	void PushSound(lua_State* L) { m_Sound.PushSelf(L); }

	bool m_is_action{ false };

	//
	// Lua
	//
	void PushSelf(lua_State* L) override;

  private:
	RageSound m_Sound;
};

#endif
