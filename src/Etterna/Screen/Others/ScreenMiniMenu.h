/* ScreenMiniMenu - Displays a simple menu over the top of another screen. */

#ifndef SCREEN_MINI_MENU_H
#define SCREEN_MINI_MENU_H

#include <utility>

#include "Etterna/Screen/Options/ScreenOptions.h"
#include "Etterna/Models/Misc/Foreach.h"

using MenuRowUpdateEnabled = bool (*)();

struct MenuRowDef
{
	int iRowCode{ 0 };
	std::string sName;
	bool bEnabled{ false };
	MenuRowUpdateEnabled pfnEnabled{}; // if ! NULL, used instead of bEnabled

	int iDefaultChoice{ 0 };
	vector<std::string> choices;
	bool bThemeTitle{ false };
	bool bThemeItems{ false };

	MenuRowDef()
	  : sName("")
	  , choices(){};
	MenuRowDef(int r,
			   const std::string& n,
			   MenuRowUpdateEnabled pe,

			   bool bTT,
			   bool bTI,
			   int d,
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
			   const char* c19 = nullptr,
			   const char* c20 = nullptr,
			   const char* c21 = nullptr,
			   const char* c22 = nullptr,
			   const char* c23 = nullptr,
			   const char* c24 = nullptr,
			   const char* c25 = nullptr)
	  : iRowCode(r)
	  , sName(n)
	  , bEnabled(true)
	  , pfnEnabled(pe)

	  , iDefaultChoice(d)
	  , choices()
	  , bThemeTitle(bTT)
	  , bThemeItems(bTI)
	{
#define PUSH(c)                                                                \
	if (c)                                                                     \
		choices.push_back(c);
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
		PUSH(c20);
		PUSH(c21);
		PUSH(c22);
		PUSH(c22);
		PUSH(c23);
		PUSH(c23);
		PUSH(c24);
		PUSH(c25);
#undef PUSH
	}

	MenuRowDef(int r,
			   const std::string& n,
			   bool e,

			   bool bTT,
			   bool bTI,
			   int d,
			   vector<std::string>& options)
	  : iRowCode(r)
	  , sName(n)
	  , bEnabled(e)
	  , pfnEnabled(nullptr)

	  , iDefaultChoice(d)
	  , choices()
	  , bThemeTitle(bTT)
	  , bThemeItems(bTI)
	{
		FOREACH(std::string, options, str)
		{
			if (*str != "")
				choices.push_back(*str);
		}
	}

	MenuRowDef(int r,
			   const std::string& n,
			   bool e,

			   bool bTT,
			   bool bTI,
			   int d,
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
			   const char* c19 = nullptr,
			   const char* c20 = nullptr,
			   const char* c21 = nullptr,
			   const char* c22 = nullptr,
			   const char* c23 = nullptr,
			   const char* c24 = nullptr,
			   const char* c25 = nullptr)
	  : iRowCode(r)
	  , sName(n)
	  , bEnabled(e)
	  , pfnEnabled(nullptr)

	  , iDefaultChoice(d)
	  , choices()
	  , bThemeTitle(bTT)
	  , bThemeItems(bTI)
	{
#define PUSH(c)                                                                \
	if (c)                                                                     \
		choices.push_back(c);
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
		PUSH(c20);
		PUSH(c21);
		PUSH(c22);
		PUSH(c22);
		PUSH(c23);
		PUSH(c23);
		PUSH(c24);
		PUSH(c25);
#undef PUSH
	}

	MenuRowDef(int r,
			   const std::string& n,
			   bool e,

			   bool bTT,
			   bool bTI,
			   int d,
			   int low,
			   int high)
	  : iRowCode(r)
	  , sName(n)
	  , bEnabled(e)
	  , pfnEnabled(nullptr)

	  , iDefaultChoice(d)
	  , choices()
	  , bThemeTitle(bTT)
	  , bThemeItems(bTI)
	{
		for (int i = low; i <= high; i++) {
			choices.push_back(IntToString(i).c_str());
		}
	}

	void SetOneUnthemedChoice(const std::string& sChoice)
	{
		choices.resize(1);
		choices[0] = "|" + sChoice;
	}

	bool SetDefaultChoiceIfPresent(const std::string& sChoice)
	{
		iDefaultChoice = 0;
		FOREACH_CONST(std::string, choices, s)
		{
			if (sChoice == *s) {
				iDefaultChoice = s - choices.begin();
				return true;
			}
		}
		return false;
	}
};

struct MenuDef
{
	std::string sClassName;
	vector<MenuRowDef> rows;

	MenuDef(std::string c,
			const MenuRowDef& r0 = MenuRowDef(),
			const MenuRowDef& r1 = MenuRowDef(),
			const MenuRowDef& r2 = MenuRowDef(),
			const MenuRowDef& r3 = MenuRowDef(),
			const MenuRowDef& r4 = MenuRowDef(),
			const MenuRowDef& r5 = MenuRowDef(),
			const MenuRowDef& r6 = MenuRowDef(),
			const MenuRowDef& r7 = MenuRowDef(),
			const MenuRowDef& r8 = MenuRowDef(),
			const MenuRowDef& r9 = MenuRowDef(),
			const MenuRowDef& r10 = MenuRowDef(),
			const MenuRowDef& r11 = MenuRowDef(),
			const MenuRowDef& r12 = MenuRowDef(),
			const MenuRowDef& r13 = MenuRowDef(),
			const MenuRowDef& r14 = MenuRowDef(),
			const MenuRowDef& r15 = MenuRowDef(),
			const MenuRowDef& r16 = MenuRowDef(),
			const MenuRowDef& r17 = MenuRowDef(),
			const MenuRowDef& r18 = MenuRowDef(),
			const MenuRowDef& r19 = MenuRowDef(),
			const MenuRowDef& r20 = MenuRowDef(),
			const MenuRowDef& r21 = MenuRowDef(),
			const MenuRowDef& r22 = MenuRowDef(),
			const MenuRowDef& r23 = MenuRowDef(),
			const MenuRowDef& r24 = MenuRowDef(),
			const MenuRowDef& r25 = MenuRowDef(),
			const MenuRowDef& r26 = MenuRowDef(),
			const MenuRowDef& r27 = MenuRowDef(),
			const MenuRowDef& r28 = MenuRowDef(),
			const MenuRowDef& r29 = MenuRowDef())
	  : sClassName(std::move(c))
	  , rows()
	{
#define PUSH(r)                                                                \
	if (!r.sName.empty())                                                      \
		rows.push_back(r);
		PUSH(r0);
		PUSH(r1);
		PUSH(r2);
		PUSH(r3);
		PUSH(r4);
		PUSH(r5);
		PUSH(r6);
		PUSH(r7);
		PUSH(r8);
		PUSH(r9);
		PUSH(r10);
		PUSH(r11);
		PUSH(r12);
		PUSH(r13);
		PUSH(r14);
		PUSH(r15);
		PUSH(r16);
		PUSH(r17);
		PUSH(r18);
		PUSH(r19);
		PUSH(r20);
		PUSH(r21);
		PUSH(r22);
		PUSH(r23);
		PUSH(r24);
		PUSH(r25);
		PUSH(r26);
		PUSH(r27);
		PUSH(r28);
		PUSH(r29);
#undef PUSH
	}
};

class ScreenMiniMenu : public ScreenOptions
{
  public:
	static void MiniMenu(const MenuDef* pDef,
						 ScreenMessage smSendOnOK,
						 ScreenMessage smSendOnCancel = SM_None,
						 float fX = 0,
						 float fY = 0);

	void Init() override;
	void BeginScreen() override;
	void HandleScreenMessage(const ScreenMessage& SM) override;

  protected:
	void AfterChangeValueOrRow(PlayerNumber pn) override;
	void ImportOptions(int iRow, const PlayerNumber& vpns) override;
	void ExportOptions(int iRow, const PlayerNumber& vpns) override;

	bool FocusedItemEndsScreen(PlayerNumber pn) const override;

	void LoadMenu(const MenuDef* pDef);

	ScreenMessage m_SMSendOnOK;
	ScreenMessage m_SMSendOnCancel;

	vector<MenuRowDef> m_vMenuRows;

  public:
	ScreenMiniMenu()
	  : m_SMSendOnOK()
	  , m_SMSendOnCancel()
	  , m_vMenuRows()
	{
	}

	static bool s_bCancelled;
	static int s_iLastRowCode;
	static vector<int> s_viLastAnswers;
};

#endif
