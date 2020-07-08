#include "GamePreferences.h"

// This options has weird interactions depending on m_bEventMode;
// use GameState::GetCoinMode().
Preference<PlayerController> GamePreferences::m_AutoPlay("AutoPlay", PC_HUMAN);
Preference<bool> GamePreferences::m_AxisFix("AxisFix", false);
