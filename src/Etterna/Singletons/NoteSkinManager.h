#ifndef NOTE_SKIN_MANAGER_H
#define NOTE_SKIN_MANAGER_H

#include "Etterna/Actor/Base/Actor.h"
#include "Etterna/Models/Misc/GameInput.h"
#include "Etterna/Models/Misc/PlayerNumber.h"

#include <utility>

struct Game;
struct NoteSkinData;

/** @brief Loads note skins. */
class NoteSkinManager
{
  public:
	NoteSkinManager();
	~NoteSkinManager();

	void RefreshNoteSkinData(const Game* game);
	void GetNoteSkinNames(const Game* game, std::vector<std::string>& AddTo);
	void GetNoteSkinNames(std::vector<std::string>&
							AddTo); // looks up current const Game* in GAMESTATE
	auto NoteSkinNameInList(const std::string& name,
							const std::vector<std::string>& name_list) -> bool;
	auto DoesNoteSkinExist(const std::string& sNoteSkin)
	  -> bool; // looks up current const Game* in GAMESTATE
	auto DoNoteSkinsExistForGame(const Game* pGame) -> bool;
	auto GetDefaultNoteSkinName()
	  -> std::string; // looks up current const Game* in GAMESTATE

	void ValidateNoteSkinName(std::string& name);

	auto GetFirstWorkingNoteSkin() -> std::string;

	void SetCurrentNoteSkin(const std::string& sNoteSkin)
	{
		m_sCurrentNoteSkin = sNoteSkin;
	}
	[[nodiscard]] auto GetCurrentNoteSkin() const -> const std::string&
	{
		return m_sCurrentNoteSkin;
	}

	void SetLastSeenColor(std::string Color) { LastColor = std::move(Color); }
	[[nodiscard]] auto GetLastSeenColor() const -> const std::string&
	{
		return LastColor;
	}

	void SetPlayerNumber(PlayerNumber pn) { m_PlayerNumber = pn; }
	void SetGameController(GameController gc) { m_GameController = gc; }
	auto GetPath(const std::string& sButtonName, const std::string& sElement)
	  -> std::string;
	auto PushActorTemplate(Lua* L,
						   const std::string& sButton,
						   const std::string& sElement,
						   bool bSpriteOnly,
						   std::string Color) -> bool;
	auto LoadActor(const std::string& sButton,
				   const std::string& sElement,
				   Actor* pParent = nullptr,
				   bool bSpriteOnly = false,
				   std::string Color = "4th") -> Actor*;

	auto GetMetric(const std::string& sButtonName, const std::string& sValue)
	  -> std::string;
	auto GetMetric(const std::string& sButtonName,
				   const std::string& sValue,
				   const std::string& sFallbackValue) -> std::string;
	auto GetMetricI(const std::string& sButtonName,
					const std::string& sValueName) -> int;
	auto GetMetricI(const std::string& sButtonName,
				   const std::string& sValueName,
				   const std::string& sDefaultValue) -> int;
	auto GetMetricF(const std::string& sButtonName,
					const std::string& sValueName) -> float;
	auto GetMetricF(const std::string& sButtonName,
					 const std::string& sValueName,
					 const std::string& sDefaultValue) -> float;
	auto GetMetricB(const std::string& sButtonName,
					const std::string& sValueName) -> bool;
	auto GetMetricB(const std::string& sButtonName,
					const std::string& sValueName,
					const std::string& sDefaultValue) -> bool;
	auto GetMetricA(const std::string& sButtonName,
					const std::string& sValueName) -> apActorCommands;

	// Lua
	void PushSelf(lua_State* L);

  protected:
	auto GetPathFromDirAndFile(const std::string& sDir,
							   const std::string& sFileName) -> std::string;
	void GetAllNoteSkinNamesForGame(const Game* pGame,
									std::vector<std::string>& AddTo);

	auto LoadNoteSkinData(const std::string& sNoteSkinName,
						  NoteSkinData& data_out) -> bool;
	auto LoadNoteSkinDataRecursive(const std::string& sNoteSkinName,
								   NoteSkinData& data_out) -> bool;
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
	LockNoteSkin(const std::string& sNoteSkin)
	{
		NOTESKIN->SetCurrentNoteSkin(sNoteSkin);
	}
	~LockNoteSkin() { NOTESKIN->SetCurrentNoteSkin(""); }
};

#endif
