#include "Etterna/Screen/Others/ScreenEvaluation.h"
#include "Etterna/Screen/Others/ScreenMessage.h"
#include "Etterna/Actor/GameplayAndMenus/StepsDisplay.h"

class ScreenNetEvaluation : public ScreenEvaluation
{
  public:
	void Init() override;

	// sm-ssc:
	int GetNumActivePlayers() { return m_iActivePlayers; }
	bool Input(const InputEventPlus& input) override;
	// Lua
	void PushSelf(lua_State* L) override;

	int m_iCurrentPlayer;
	void UpdateStats();

  protected:
	void HandleScreenMessage(const ScreenMessage& SM) override;
	void TweenOffScreen() override;

  private:
	int m_iActivePlayers;
	PlayerNumber m_pActivePlayer;
};
