#ifndef BackgroundUtil_H
#define BackgroundUtil_H

class Song;
class XNode;

extern const std::string RANDOM_BACKGROUND_FILE;
extern const std::string NO_SONG_BG_FILE;
extern const std::string SONG_BACKGROUND_FILE;

extern const std::string SBE_UpperLeft;
extern const std::string SBE_Centered;
extern const std::string SBE_StretchNormal;
extern const std::string SBE_StretchNoLoop;
extern const std::string SBE_StretchRewind;
extern const std::string SBT_CrossFade;

struct BackgroundDef
{
	bool operator<(const BackgroundDef& other) const;
	bool operator==(const BackgroundDef& other) const;
	[[nodiscard]] bool IsEmpty() const { return m_sFile1.empty() && m_sFile2.empty(); }
	std::string m_sEffect; // "" == automatically choose
	std::string m_sFile1;  // must not be ""
	std::string m_sFile2;  // may be ""
	std::string m_sColor1; // "" == use default
	std::string m_sColor2; // "" == use default

	[[nodiscard]] XNode* CreateNode() const;

	/** @brief Set up the BackgroundDef with default values. */
	BackgroundDef()
	  : m_sEffect("")
	  , m_sFile1("")
	  , m_sFile2("")
	  , m_sColor1("")
	  , m_sColor2("")
	{
	}

	/**
	 * @brief Set up the BackgroundDef with some defined values.
	 * @param effect the intended effect.
	 * @param f1 the primary filename for the definition.
	 * @param f2 the secondary filename (optional). */
	BackgroundDef(const std::string& effect,
				  const std::string& f1,
				  const std::string& f2)
	  : m_sEffect(effect)
	  , m_sFile1(f1)
	  , m_sFile2(f2)
	  , m_sColor1("")
	  , m_sColor2("")
	{
	}
};

struct BackgroundChange
{
	BackgroundChange()
	  : m_def()
	  , m_sTransition("")
	{
	}

	BackgroundChange(float s,
					 std::string f1,
					 std::string f2 = std::string(),
					 float r = 1.f,
					 std::string e = SBE_Centered,
					 std::string t = std::string())
	  : m_def(e, f1, f2)
	  , m_fStartBeat(s)
	  , m_fRate(r)
	  , m_sTransition(t)
	{
	}

	BackgroundDef m_def;
	float m_fStartBeat{ -1 };
	float m_fRate{ 1 };
	std::string m_sTransition;

	[[nodiscard]] std::string GetTextDescription() const;

	/**
	 * @brief Get the string representation of the change.
	 * @return the string representation. */
	[[nodiscard]] std::string ToString() const;
};
/** @brief Shared background-related routines. */
namespace BackgroundUtil {
void
AddBackgroundChange(vector<BackgroundChange>& vBackgroundChanges,
					const BackgroundChange& seg);
void
SortBackgroundChangesArray(vector<BackgroundChange>& vBackgroundChanges);

void
GetBackgroundEffects(const std::string& sName,
					 vector<std::string>& vsPathsOut,
					 vector<std::string>& vsNamesOut);
void
GetBackgroundTransitions(const std::string& sName,
						 vector<std::string>& vsPathsOut,
						 vector<std::string>& vsNamesOut);

void
GetSongBGAnimations(const Song* pSong,
					const std::string& sMatch,
					vector<std::string>& vsPathsOut,
					vector<std::string>& vsNamesOut);
void
GetSongMovies(const Song* pSong,
			  const std::string& sMatch,
			  vector<std::string>& vsPathsOut,
			  vector<std::string>& vsNamesOut);
void
GetSongBitmaps(const Song* pSong,
			   const std::string& sMatch,
			   vector<std::string>& vsPathsOut,
			   vector<std::string>& vsNamesOut);
void
GetGlobalBGAnimations(const Song* pSong,
					  const std::string& sMatch,
					  vector<std::string>& vsPathsOut,
					  vector<std::string>& vsNamesOut);
void
BakeAllBackgroundChanges(Song* pSong);
};

#endif
