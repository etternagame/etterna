#ifndef CHARACTER_MANAGER_H
#define CHARACTER_MANAGER_H

class Character;
struct lua_State;

/** @brief Manage all of the Characters. */
class CharacterManager
{
  public:
	/** @brief Set up the character manager. */
	CharacterManager();
	/** @brief Destroy the character manager. */
	~CharacterManager();

	void GetCharacters(vector<Character*>& vpCharactersOut);
	/** @brief Get one installed character at random.
	 * @return The random character. */
	Character* GetRandomCharacter();
	/** @brief Get the character assigned as the default.
	 * @return The default character. */
	Character* GetDefaultCharacter();
	Character* GetCharacterFromID(const RString& sCharacterID);

	void DemandGraphics();
	void UndemandGraphics();

	// Lua
	void PushSelf(lua_State* L);

  private:
	vector<Character*> m_pCharacters;
};

extern CharacterManager*
  CHARMAN; // global and accessible from anywhere in our program

#endif
