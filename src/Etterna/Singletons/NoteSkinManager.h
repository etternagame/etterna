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
	void GetNoteSkinNames(const Game* game, vector<std::string>& AddTo);
	void GetNoteSkinNames(
	  vector<std::string>& AddTo); // looks up current const Game* in GAMESTATE
	bool NoteSkinNameInList(const std::string& name,
							const vector<std::string>& name_list);
	bool DoesNoteSkinExist(
	  const std::string&
		sNoteSkin); // looks up current const Game* in GAMESTATE
	bool DoNoteSkinsExistForGame(const Game* pGame);
	std::string
	GetDefaultNoteSkinName(); // looks up current const Game* in GAMESTATE

	void ValidateNoteSkinName(std::string& name);

	void SetCurrentNoteSkin(const std::string& sNoteSkin)
	{
		m_sCurrentNoteSkin = sNoteSkin;
	}
	const std::string& GetCurrentNoteSkin() { return m_sCurrentNoteSkin; }

	void SetLastSeenColor(std::string Color) { LastColor = Color; }
	const std::string& GetLastSeenColor() { return LastColor; }

	void SetPlayerNumber(PlayerNumber pn) { m_PlayerNumber = pn; }
	void SetGameController(GameController gc) { m_GameController = gc; }
	std::string GetPath(const std::string& sButtonName,
						const std::string& sElement);
	bool PushActorTemplate(Lua* L,
						   const std::string& sButton,
						   const std::string& sElement,
						   bool bSpriteOnly,
						   std::string Color);
	Actor* LoadActor(const std::string& sButton,
					 const std::string& sElement,
					 Actor* pParent = nullptr,
					 bool bSpriteOnly = false,
					 std::string Color = "4th");

	std::string GetMetric(const std::string& sButtonName,
						  const std::string& sValue);
	int GetMetricI(const std::string& sButtonName,
				   const std::string& sValueName);
	float GetMetricF(const std::string& sButtonName,
					 const std::string& sValueName);
	bool GetMetricB(const std::string& sButtonName,
					const std::string& sValueName);
	apActorCommands GetMetricA(const std::string& sButtonName,
							   const std::string& sValueName);

	// Lua
	void PushSelf(lua_State* L);

  protected:
	std::string GetPathFromDirAndFile(const std::string& sDir,
									  const std::string& sFileName);
	void GetAllNoteSkinNamesForGame(const Game* pGame,
									vector<std::string>& AddTo);

	bool LoadNoteSkinData(const std::string& sNoteSkinName,
						  NoteSkinData& data_out);
	bool LoadNoteSkinDataRecursive(const std::string& sNoteSkinName,
								   NoteSkinData& data_out);
	std::string m_sCurrentNoteSkin;
	const Game* m_pCurGame;
	std::string LastColor;

	// xxx: is this the best way to implement this? -freem
	PlayerNumber m_PlayerNumber;
	GameController m_GameController;
};

extern NoteSkinManager*
  NOTESKIN; // global and accessible from anywhere in our program

class LockNoteSkin
{
  public:
	LockNoteSkin(std::string sNoteSkin, PlayerNumber pn)
	{
		ASSERT(NOTESKIN->GetCurrentNoteSkin().empty());
		NOTESKIN->SetCurrentNoteSkin(sNoteSkin);
	}
	~LockNoteSkin() { NOTESKIN->SetCurrentNoteSkin(""); }
};

#endif
