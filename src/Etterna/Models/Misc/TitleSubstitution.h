#ifndef TITLE_SUBSTITUTION_H
#define TITLE_SUBSTITUTION_H

/** @brief The different fields to potentially translate. */
struct TitleFields
{
	void SaveToStrings(std::string& sTitle,
					   std::string& sSubtitle,
					   std::string& sArtist,
					   std::string& sTitleTranslit,
					   std::string& sSubtitleTranslit,
					   std::string& sArtistTranslit) const
	{
		sTitle = Title;
		sSubtitle = Subtitle;
		sArtist = Artist;
		sTitleTranslit = TitleTranslit;
		sSubtitleTranslit = SubtitleTranslit;
		sArtistTranslit = ArtistTranslit;
	}

	void LoadFromStrings(const std::string& sTitle,
						 const std::string& sSubtitle,
						 const std::string& sArtist,
						 const std::string& sTitleTranslit,
						 const std::string& sSubtitleTranslit,
						 const std::string& sArtistTranslit)
	{
		Title = sTitle;
		Subtitle = sSubtitle;
		Artist = sArtist;
		TitleTranslit = sTitleTranslit;
		SubtitleTranslit = sSubtitleTranslit;
		ArtistTranslit = sArtistTranslit;
	}
	std::string Title, Subtitle, Artist;
	std::string TitleTranslit, SubtitleTranslit, ArtistTranslit;
};
struct TitleTrans;
/** @brief Automatic translation for Song titles. */
class TitleSubst
{
	vector<TitleTrans*> ttab;

	void AddTrans(const TitleTrans& tr);

  public:
	TitleSubst(const std::string& section);
	~TitleSubst();

	void Load(const std::string& filename, const std::string& section);

	void Subst(TitleFields& tf);
};

#endif
