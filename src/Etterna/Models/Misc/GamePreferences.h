#ifndef GAME_PREFERENCES_H
#define GAME_PREFERENCES_H

#include "GameConstantsAndTypes.h"
#include "Preference.h"
/** @brief Quick access to other variables. */
namespace GamePreferences {
extern Preference<PlayerController> m_AutoPlay;
extern Preference<bool> m_AxisFix;
};

#endif
