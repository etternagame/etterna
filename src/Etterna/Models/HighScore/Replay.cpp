#include "Etterna/Globals/global.h"
#include "Replay.h"
#include "HighScore.h"

Replay::Replay() {

}

Replay::Replay(HighScore* hs)
  : scoreKey(hs->GetScoreKey())
  , chartKey(hs->GetChartKey())
{

}

Replay::~Replay() {
	Unload();
}
