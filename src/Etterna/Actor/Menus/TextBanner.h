/* TextBanner - Shows song title, subtitle, and artist.  Displayed on the
 * MusicWheel. */

#ifndef TEXTBANNER_H
#define TEXTBANNER_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/BitmapText.h"
class Song;

class TextBanner final : public ActorFrame
{
  public:
	TextBanner();
	TextBanner(const TextBanner& cpy);
	[[nodiscard]] auto Copy() const -> TextBanner* override;

	void LoadFromNode(const XNode* pNode) override;
	void Load(const std::string& sMetricsGroup); // load metrics
	void SetFromSong(const Song* pSong);
	void SetFromString(const std::string& sDisplayTitle,
					   const std::string& sTranslitTitle,
					   const std::string& sDisplaySubTitle,
					   const std::string& sTranslitSubTitle,
					   const std::string& sDisplayArtist,
					   const std::string& sTranslitArtist);

	// Lua
	void PushSelf(lua_State* L) override;

  private:
	bool m_bInitted;
	BitmapText m_textTitle, m_textSubTitle, m_textArtist;
	std::string m_sArtistPrependString;
};

#endif
