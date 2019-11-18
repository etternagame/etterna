#ifndef NOTE_SKIN_MANAGER_H
#define NOTE_SKIN_MANAGER_H

#include "Etterna/Actor/Base/Actor.h"
#include "Etterna/Models/Misc/GameInput.h"
#include "Etterna/Models/Misc/PlayerNumber.h"

struct Game;
struct NoteSkinData;

/** @brief Loads note skins. */
class NoteSkinManager
{
  public:
	NoteSkinManager();
	~NoteSkinManager();

	void RefreshNoteSkinData(const Game* game);
	void GetNoteSkinNames(const Game* game, vector<RString>& AddTo);
	void GetNoteSkinNames(
	  vector<RString>& AddTo); // looks up current const Game* in GAMESTATE
	bool NoteSkinNameInList(const RString& name,
							const vector<RString>& name_list);
	bool DoesNoteSkinExist(
	  const RString& sNoteSkin); // looks up current const Game* in GAMESTATE
	bool DoNoteSkinsExistForGame(const Game* pGame);
	RString
	GetDefaultNoteSkinName(); // looks up current const Game* in GAMESTATE

	void ValidateNoteSkinName(RString& name);

	void SetCurrentNoteSkin(const RString& sNoteSkin)
	{
		m_sCurrentNoteSkin = sNoteSkin;
	}
	const RString& GetCurrentNoteSkin() { return m_sCurrentNoteSkin; }

	void SetLastSeenColor(RString Color) { LastColor = Color; }
	RString GetLastSeenColor() { return LastColor; }

	void SetPlayerNumber(PlayerNumber pn) { m_PlayerNumber = pn; }
	void SetGameController(GameController gc) { m_GameController = gc; }
	RString GetPath(const RString& sButtonName, const RString& sElement);
	bool PushActorTemplate(Lua* L,
						   const RString& sButton,
						   const RString& sElement,
						   bool bSpriteOnly,
						   RString Color);
	Actor* LoadActor(const RString& sButton,
					 const RString& sElement,
					 Actor* pParent = NULL,
					 bool bSpriteOnly = false,
					 RString Color = "4th");

	RString GetMetric(const RString& sButtonName, const RString& sValue);
	int GetMetricI(const RString& sButtonName, const RString& sValueName);
	float GetMetricF(const RString& sButtonName, const RString& sValueName);
	bool GetMetricB(const RString& sButtonName, const RString& sValueName);
	apActorCommands GetMetricA(const RString& sButtonName,
							   const RString& sValueName);

	// Lua
	void PushSelf(lua_State* L);

  protected:
	RString GetPathFromDirAndFile(const RString& sDir,
								  const RString& sFileName);
	void GetAllNoteSkinNamesForGame(const Game* pGame, vector<RString>& AddTo);

	bool LoadNoteSkinData(const RString& sNoteSkinName, NoteSkinData& data_out);
	bool LoadNoteSkinDataRecursive(const RString& sNoteSkinName,
								   NoteSkinData& data_out);
	RString m_sCurrentNoteSkin;
	const Game* m_pCurGame;
	RString LastColor;

	// xxx: is this the best way to implement this? -freem
	PlayerNumber m_PlayerNumber;
	GameController m_GameController;
};

extern NoteSkinManager*
  NOTESKIN; // global and accessible from anywhere in our program

class LockNoteSkin
{
  public:
	LockNoteSkin(RString sNoteSkin, PlayerNumber pn)
	{
		ASSERT(NOTESKIN->GetCurrentNoteSkin().empty());
		NOTESKIN->SetCurrentNoteSkin(sNoteSkin);
	}
	~LockNoteSkin() { NOTESKIN->SetCurrentNoteSkin(""); }
};

#endif
