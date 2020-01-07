#ifndef TITLE_SUBSTITUTION_H
#define TITLE_SUBSTITUTION_H

#include <vector>

/** @brief The different fields to potentially translate. */
struct TitleFields
{
	void SaveToStrings(RString& sTitle,
					   RString& sSubtitle,
					   RString& sArtist,
					   RString& sTitleTranslit,
					   RString& sSubtitleTranslit,
					   RString& sArtistTranslit) const
	{
		sTitle = Title;
		sSubtitle = Subtitle;
		sArtist = Artist;
		sTitleTranslit = TitleTranslit;
		sSubtitleTranslit = SubtitleTranslit;
		sArtistTranslit = ArtistTranslit;
	}

	void LoadFromStrings(RString sTitle,
						 RString sSubtitle,
						 RString sArtist,
						 RString sTitleTranslit,
						 RString sSubtitleTranslit,
						 RString sArtistTranslit)
	{
		Title = sTitle;
		Subtitle = sSubtitle;
		Artist = sArtist;
		TitleTranslit = sTitleTranslit;
		SubtitleTranslit = sSubtitleTranslit;
		ArtistTranslit = sArtistTranslit;
	}
	RString Title, Subtitle, Artist;
	RString TitleTranslit, SubtitleTranslit, ArtistTranslit;
};
struct TitleTrans;
/** @brief Automatic translation for Song titles. */
class TitleSubst
{
	std::vector<TitleTrans*> ttab;

	void AddTrans(const TitleTrans& tr);

  public:
	TitleSubst(const RString& section);
	~TitleSubst();

	void Load(const RString& filename, const RString& section);

	void Subst(TitleFields& tf);
};

#endif
