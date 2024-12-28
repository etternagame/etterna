#ifndef MODS_GROUP_H
#define MODS_GROUP_H

#include "EnumHelper.h"
#include "RageUtil/Misc/RageTimer.h"
// ReSharper disable once CppUnusedIncludeDirective MACROS LUL
#include "Etterna/Models/Songs/SongOptions.h"

#include <cassert>

enum ModsLevel
{
	ModsLevel_Preferred, // user-chosen player options.  Does not include any
						 // forced mods.
	ModsLevel_Stage,	 // Preferred + forced stage mods
	ModsLevel_Song,		 // Stage + forced attack mods
	ModsLevel_Current,	 // Approaches Song
	NUM_ModsLevel,
	ModsLevel_Invalid
};
LuaDeclareType(ModsLevel);

#define MODS_GROUP_ASSIGN(group, level, member, val)                           \
	(group).Assign((level), member, (val))
#define MODS_GROUP_ASSIGN_N(group, level, member, n, val)                      \
	(group).Assign_n((level), member, (n), (val))
#define MODS_GROUP_CALL(group, level, fun) (group).Call((level), fun)

#define PO_GROUP_ASSIGN(group, level, member, val)                             \
	MODS_GROUP_ASSIGN((group), (level), &PlayerOptions::member, (val))
#define PO_GROUP_ASSIGN_N(group, level, member, n, val)                        \
	MODS_GROUP_ASSIGN_N((group), (level), &PlayerOptions::member, (n), (val))
#define PO_GROUP_CALL(group, level, fun)                                       \
	MODS_GROUP_CALL((group), (level), &PlayerOptions::fun)
#define SO_GROUP_ASSIGN(group, level, member, val)                             \
	MODS_GROUP_ASSIGN((group), (level), &SongOptions::member, (val))
#define SO_GROUP_ASSIGN_N(group, level, member, n, val)                        \
	MODS_GROUP_ASSIGN_N((group), (level), &SongOptions::member, (n), (val))
#define SO_GROUP_CALL(group, level, fun)                                       \
	MODS_GROUP_CALL((group), (level), &SongOptions::fun)

template<class T>
class ModsGroup
{
	T m_[NUM_ModsLevel]{};
	RageTimer m_Timer;

  public:
	void Init() { Call(ModsLevel_Preferred, &T::Init); }

	void Update(float fDelta)
	{
		// Don't let the mod approach speed be affected by Tab.
		// TODO(Sam): Find a more elegant way of handling this.
		fDelta = m_Timer.GetDeltaTime();
		m_[ModsLevel_Current].Approach(m_[ModsLevel_Song], fDelta);
	}

	template<typename U>
	void Assign(ModsLevel level, U T::*member, const U& val)
	{
		if (level != ModsLevel_Song)
			m_[ModsLevel_Current].*member = val;
		for (; level < ModsLevel_Current; enum_add(level, 1))
			m_[level].*member = val;
	}

	template<typename U, int n>
	void Assign_n(ModsLevel level,
				  U (T::*member)[n],
				  size_t index,
				  const U& val)
	{
		assert(index < n);
		if (level != ModsLevel_Song)
			(m_[ModsLevel_Current].*member)[index] = val;
		for (; level < ModsLevel_Current; enum_add(level, 1))
			(m_[level].*member)[index] = val;
	}

	void Assign(ModsLevel level, const T& val)
	{
		if (level != ModsLevel_Song)
			m_[ModsLevel_Current] = val;
		for (; level < ModsLevel_Current; enum_add(level, 1))
			m_[level] = val;
	}

	void Call(ModsLevel level, void (T::*fun)())
	{
		if (level != ModsLevel_Song)
			(m_[ModsLevel_Current].*fun)();
		for (; level < ModsLevel_Current; enum_add(level, 1))
			(m_[level].*fun)();
	}

	void FromString(ModsLevel level, const std::string& str)
	{
		if (level != ModsLevel_Song)
			m_[ModsLevel_Current].FromString(str);
		for (; level < ModsLevel_Current; enum_add(level, 1))
			m_[level].FromString(str);
	}

	void SetCurrentToLevel(ModsLevel level)
	{
		m_[ModsLevel_Current] = m_[level];
	}

	[[nodiscard]] auto Get(ModsLevel l) const -> const T& { return m_[l]; }
	[[nodiscard]] auto GetPreferred() const -> const T&
	{
		return m_[ModsLevel_Preferred];
	}
	[[nodiscard]] auto GetStage() const -> const T&
	{
		return m_[ModsLevel_Stage];
	}
	[[nodiscard]] auto GetSong() const -> const T&
	{
		return m_[ModsLevel_Song];
	}
	[[nodiscard]] auto GetCurrent() const -> const T&
	{
		return m_[ModsLevel_Current];
	}
	auto Get(ModsLevel l) -> T& { return m_[l]; }
	auto GetPreferred() -> T& { return m_[ModsLevel_Preferred]; }
	auto GetStage() -> T& { return m_[ModsLevel_Stage]; }
	auto GetSong() -> T& { return m_[ModsLevel_Song]; }
	auto GetCurrent() -> T& { return m_[ModsLevel_Current]; }
};

#endif
