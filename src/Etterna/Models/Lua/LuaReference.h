#ifndef LUA_REFERENCE_H
#define LUA_REFERENCE_H

#include "Etterna/Singletons/LuaManager.h"

struct lua_State;
using Lua = lua_State;
/** @brief A self-cleaning Lua reference. */
class LuaReference
{
  public:
	LuaReference();
	virtual ~LuaReference();

	/* Copying a reference makes a new reference pointing to the same object. */
	LuaReference(const LuaReference& cpy);
	/* Moving a reference does not make a new reference and leaves the 
	 * moved-from value with LUA_NOREF. */
	LuaReference(LuaReference&& cpy);
	auto operator=(const LuaReference& cpy) -> LuaReference&;

	// Convenience constructor.
	LuaReference(Lua* L)
	  : m_iReference(LUA_NOREF)
	{
		SetFromStack(L);
	}

	void swap(LuaReference& other)
	{
		LuaReference temp = *this;
		*this = other;
		other = temp;
	}

	/* Create a reference pointing to the item at the top of the stack, and pop
	 * the stack. */
	void SetFromStack(Lua* L);
	void SetFromNil();

	/* Evaluate an expression that returns an object; store the object in a
	 * reference. For example, evaluating "{ 1, 2, 3 }" will result in a
	 * reference to a table. On success, return true.  On error, set to nil and
	 * return false. */
	auto SetFromExpression(const std::string& sExpression) -> bool;

	/** @brief Deep-copy tables, detaching this reference from any others. */
	void DeepCopy();

	/* Push the referenced object onto the stack.  If not set (or set to nil),
	 * push nil. */
	virtual void PushSelf(Lua* L) const;

	/**
	 * @brief Determine if the reference is set.
	 *
	 * SetFromNil() counts as being set.
	 * @return true if it's set. */
	[[nodiscard]] auto IsSet() const -> bool;
	/**
	 * @brief Determine if the reference is nil.
	 * @return true if it's nil. */
	[[nodiscard]] auto IsNil() const -> bool;
	void Unset() { Unregister(); }

	/* Return the referenced type, or LUA_TNONE if not set. */
	[[nodiscard]] auto GetLuaType() const -> int;

	auto GetIdentifier() -> int { return m_iReference; }

	[[nodiscard]] auto Serialize() const -> std::string;

	template<typename T>
	static auto Create(const T& val) -> LuaReference
	{
		Lua* L = LUA->Get();
		LuaReference ref;
		LuaHelpers::Push(L, val);
		ref.SetFromStack(L);
		LUA->Release(L);

		return ref;
	}

	template<class T>
	static auto CreateFromPush(T& obj) -> LuaReference
	{
		Lua* L = LUA->Get();
		LuaReference ref;
		obj.PushSelf(L);
		ref.SetFromStack(L);
		LUA->Release(L);

		return ref;
	}

  private:
	void Unregister();
	int m_iReference;
};

using apActorCommands = std::shared_ptr<LuaReference>;

class LuaTable : public LuaReference
{
  public:
	LuaTable();

	/* Get the key with the given name, and push it on the stack. */
	void Get(Lua* L, const std::string& sKey);

	/* Set a key by the given name to a value on the stack, and pop the value
	 * off the stack. */
	void Set(Lua* L, const std::string& sKey);
};

#endif
