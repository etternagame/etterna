/* TextBanner - Shows song title, subtitle, and artist.  Displayed on the
 * MusicWheel. */

#ifndef TEXTBANNER_H
#define TEXTBANNER_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/BitmapText.h"
class Song;

class TextBanner : public ActorFrame
{
  public:
	TextBanner();
	TextBanner(const TextBanner& cpy);
	TextBanner* Copy() const override;

	void LoadFromNode(const XNode* pNode) override;
	void Load(const std::string& sMetricsGroup); // load metrics
	void SetFromSong(const Song* pSong);
	void SetFromString(const string& sDisplayTitle,
					   const string& sTranslitTitle,
					   const string& sDisplaySubTitle,
					   const string& sTranslitSubTitle,
					   const string& sDisplayArtist,
					   const string& sTranslitArtist);

	// Lua
	void PushSelf(lua_State* L) override;

  private:
	bool m_bInitted;
	BitmapText m_textTitle, m_textSubTitle, m_textArtist;
	std::string m_sArtistPrependString;
};

#endif
