#ifndef AutoActor_H
#define AutoActor_H

class Actor;
class XNode;

/**
 * @brief A smart pointer for Actor.
 *
 * This creates the appropriate Actor derivative on load and
 * automatically deletes the Actor on deconstruction. */
class AutoActor
{
  public:
	AutoActor() = default;
	~AutoActor() { Unload(); }
	AutoActor(const AutoActor& cpy);
	auto operator=(const AutoActor& cpy) -> AutoActor&;
	operator const Actor*() const { return m_pActor; }
	operator Actor*() { return m_pActor; }
	auto operator->() const -> const Actor* { return m_pActor; }
	auto operator->() -> Actor* { return m_pActor; }
	void Unload();
	/**
	 * @brief Determine if this actor is presently loaded.
	 * @return true if it is loaded, or false otherwise. */
	[[nodiscard]] auto IsLoaded() const -> bool { return m_pActor != nullptr; }
	void Load(Actor* pActor); // transfer pointer
	void Load(const std::string& sPath);
	void LoadB(const std::string& sMetricsGroup,
			   const std::string& sElement); // load a background and set up
											 // LuaThreadVariables for recursive
											 // loading
	void LoadActorFromNode(const XNode* pNode, Actor* pParent);
	void LoadAndSetName(const std::string& sScreenName,
						const std::string& sActorName);

  protected:
	/** @brief the Actor for which there is a smart pointer to. */
	Actor* m_pActor{ nullptr };
};

#endif
