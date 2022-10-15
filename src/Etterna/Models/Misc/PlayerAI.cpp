#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "PlayerAI.h"
#include "PlayerState.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RadarValues.h"
#include "PlayerStageStats.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Actor/Gameplay/LifeMeterBar.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Core/Services/Locator.hpp"

#include <map>
#include <algorithm>
#include <set>


