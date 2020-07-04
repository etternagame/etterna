#ifndef SCREEN_OPTIONS_MASTER_PREFS_H
#define SCREEN_OPTIONS_MASTER_PREFS_H

#include "Etterna/Models/Misc/EnumHelper.h"

static const int MAX_OPTIONS = 16;
enum OptEffect
{
	OPT_SAVE_PREFERENCES = (1 << 0),
	OPT_APPLY_GRAPHICS = (1 << 1),
	OPT_APPLY_THEME = (1 << 2),
	OPT_CHANGE_GAME = (1 << 3),
	OPT_APPLY_SOUND = (1 << 4),
	OPT_APPLY_SONG = (1 << 5),
	OPT_APPLY_ASPECT_RATIO = (1 << 6),
	NUM_OptEffect = 7,
	OptEffect_Invalid = MAX_OPTIONS + 1
};
const std::string&
OptEffectToString(OptEffect e);
OptEffect
StringToOptEffect(const std::string& e);
LuaDeclareType(OptEffect);

struct ConfOption
{
	static struct ConfOption* Find(const std::string& name);

	// Name of this option.
	std::string name;

	// Name of the preference this option affects.
	std::string m_sPrefName;

	using MoveData_t = void (*)(int&, bool, const ConfOption*);
	MoveData_t MoveData;
	int m_iEffects;
	bool m_bAllowThemeItems;

	/* For dynamic options, update the options. Since this changes the available
	 * options, this may invalidate the offsets returned by Get() and Put(). */
	void UpdateAvailableOptions();

	/* Return the list of available selections; Get() and Put() use indexes into
	 * this array. UpdateAvailableOptions() should be called before using this.
	 */
	void MakeOptionsList(vector<std::string>& out) const;

	inline int Get() const
	{
		int sel;
		MoveData(sel, true, this);
		return sel;
	}
	inline void Put(int sel) const { MoveData(sel, false, this); }
	int GetEffects() const;

	ConfOption(const char* n,
			   MoveData_t m,
			   const char* c0 = nullptr,
			   const char* c1 = nullptr,
			   const char* c2 = nullptr,
			   const char* c3 = nullptr,
			   const char* c4 = nullptr,
			   const char* c5 = nullptr,
			   const char* c6 = nullptr,
			   const char* c7 = nullptr,
			   const char* c8 = nullptr,
			   const char* c9 = nullptr,
			   const char* c10 = nullptr,
			   const char* c11 = nullptr,
			   const char* c12 = nullptr,
			   const char* c13 = nullptr,
			   const char* c14 = nullptr,
			   const char* c15 = nullptr,
			   const char* c16 = nullptr,
			   const char* c17 = nullptr,
			   const char* c18 = nullptr,
			   const char* c19 = nullptr)
	{
		name = n;
		m_sPrefName = name; // copy from name (not n), to allow refcounting
		MoveData = m;
		MakeOptionsListCB = nullptr;
		m_iEffects = 0;
		m_bAllowThemeItems = true;
#define PUSH(c)                                                                \
	if (c)                                                                     \
		names.push_back(c);
		PUSH(c0);
		PUSH(c1);
		PUSH(c2);
		PUSH(c3);
		PUSH(c4);
		PUSH(c5);
		PUSH(c6);
		PUSH(c7);
		PUSH(c8);
		PUSH(c9);
		PUSH(c10);
		PUSH(c11);
		PUSH(c12);
		PUSH(c13);
		PUSH(c14);
		PUSH(c15);
		PUSH(c16);
		PUSH(c17);
		PUSH(c18);
		PUSH(c19);
	}
	void AddOption(const std::string& sName) { PUSH(sName.c_str()); }
#undef PUSH

	ConfOption(const char* n,
			   MoveData_t m,
			   void (*lst)(vector<std::string>& out))
	{
		name = n;
		MoveData = m;
		MakeOptionsListCB = lst;
		m_iEffects = 0;
		m_bAllowThemeItems = false; // don't theme dynamic choices
	}

	// private:
	vector<std::string> names;
	void (*MakeOptionsListCB)(vector<std::string>& out);
};

#endif
