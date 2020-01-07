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
	void Load(const RString& sMetricsGroup);
	void Set(const RString& sText);

  protected:
	BitmapText m_text;
	AutoActor m_sprFilled;
	AutoActor m_sprEmpty;

	ThemeMetric<int> CROP_TEXT_TO_WIDTH;
	ThemeMetric<RString> STOP_WORDS;
	std::vector<RString> m_vStopWords;
};

#endif
