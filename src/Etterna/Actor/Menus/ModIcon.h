#ifndef ModIcon_H
#define ModIcon_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
/** @brief Shows PlayerOptions and SongOptions in icon form. */
class ModIcon : public ActorFrame
{
  public:
	ModIcon();
	ModIcon(const ModIcon& cpy);
	void Load(const std::string& sMetricsGroup);
	void Set(const std::string& sText);

  protected:
	BitmapText m_text;
	AutoActor m_sprFilled;
	AutoActor m_sprEmpty;

	ThemeMetric<int> CROP_TEXT_TO_WIDTH;
	ThemeMetric<std::string> STOP_WORDS;
	vector<std::string> m_vStopWords;
};

#endif
