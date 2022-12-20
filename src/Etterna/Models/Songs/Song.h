#ifndef SONG_H
#define SONG_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Utils/RageUtil_CachedObject.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/Misc/TimingData.h"

#include <set>

class Calc;
class Style;
class StepsID;
struct lua_State;
struct BackgroundChange;

void
FixupPath(std::string& path, const std::string& sSongPath);
auto
GetSongAssetPath(const std::string& sPath, const std::string& sSongPath)
  -> std::string;

/** @brief The version of the .ssc file format. */
const static float STEPFILE_VERSION_NUMBER = 0.83F;

/** @brief The different background layers available. */
enum BackgroundLayer
{
	BACKGROUND_LAYER_1,
	BACKGROUND_LAYER_2,
	// BACKGROUND_LAYER_3, // StepCollection get
	NUM_BackgroundLayer,
	BACKGROUND_LAYER_Invalid
};

/** @brief A custom foreach loop for the different background layers. */
#define FOREACH_BackgroundLayer(bl) FOREACH_ENUM(BackgroundLayer, bl)

/** @brief Different instrument tracks for band type games. */
enum InstrumentTrack
{
	InstrumentTrack_Guitar,
	InstrumentTrack_Rhythm,
	InstrumentTrack_Bass,
	NUM_InstrumentTrack,
	InstrumentTrack_Invalid
};
auto
InstrumentTrackToString(InstrumentTrack it) -> const std::string&;
auto
StringToInstrumentTrack(const std::string& s) -> InstrumentTrack;

/** @brief The collection of lyrics for the Song. */
struct LyricSegment
{
	float m_fStartTime;	  /** @brief When does the lyric show up? */
	std::string m_sLyric; /** @brief The lyrics themselves. */
	RageColor m_Color;	  /** @brief The color of the lyrics. */
};

/** @brief Holds all music metadata and steps for one song. */
class Song
{
	std::string m_sSongDir;

  public:
	void SetSongDir(const std::string& sDir) { m_sSongDir = sDir; }
	auto GetSongDir() -> const std::string& { return m_sSongDir; }

	/** @brief When should this song be displayed in the music wheel? */
	enum SelectionDisplay
	{
		SHOW_ALWAYS, /**< always show on the wheel. */
		SHOW_NEVER	 /**< never show on the wheel (unless song hiding is turned
						off). */
	} m_SelectionDisplay;

	Song();
	~Song();
	void Reset();
	void DetachSteps();

	/**
	 * @brief Load a song from the chosen directory.
	 *
	 * This assumes that there is no song present right now.
	 * @param sDir the song directory from which to load. */
	void FinalizeLoading();
	auto LoadFromSongDir(std::string sDir, Calc* calc = nullptr) -> bool;
	// This one takes the effort to reuse Steps pointers as best as it can
	auto ReloadFromSongDir(const std::string& sDir) -> bool;
	auto ReloadFromSongDir() -> bool { return ReloadFromSongDir(GetSongDir()); }
	void ReloadIfNoMusic();

	std::string m_sFileHash;
	auto GetFileHash() -> std::string;

	/**
	 * @brief Call this after loading a song to clean up invalid data.
	 * @param fromCache was this data loaded from the cache file?
	 * @param duringCache was this data loaded during the cache process? */
	void TidyUpData(bool fromCache = false,
					bool duringCache = false,
					Calc* calc = nullptr);

	/**
	 * @brief Get the new radar values, and determine the last second at the
	 * same time. This is called by TidyUpData, after saving the Song.
	 * @param fromCache was this data loaded from the cache file?
	 * @param duringCache was this data loaded during the cache process? */
	void ReCalculateRadarValuesAndLastSecond(bool fromCache = false,
											 bool duringCache = false,
											 Calc* calc = nullptr);
	/**
	 * @brief Translate any titles that aren't in english.
	 * This is called by TidyUpData. */
	void TranslateTitles();

	/**
	 * @brief Save to the new SSC file format.
	 * @param sPath the path where we're saving the file.
	 * @param bSavingCache a flag to determine if we're saving cache data.
	 */
	auto SaveToSSCFile(const std::string& sPath, bool bSavingCache) -> bool;
	auto SaveToETTFile(const std::string& sPath, bool bSavingCache) -> bool;

	/** @brief Save to the SSC and SM files no matter what. */
	void Save();
	/**
	 * @brief Save the current Song to a cache file using the preferred format.
	 * @return its success or failure. */
	auto SaveToCacheFile() -> bool;
	/**
	 * @brief Save the current Song to a SM file.
	 * @return its success or failure. */
	auto SaveToSMFile() -> bool;
	/**
	 * @brief Save the current Song to a DWI file if possible.
	 * @return its success or failure. */
	auto SaveToDWIFile() -> bool;

	[[nodiscard]] auto GetSongFilePath() const -> const std::string&;
	[[nodiscard]] auto GetCacheFilePath() const -> std::string;

	// Directory this song data came from:
	[[nodiscard]] auto GetSongDir() const -> const std::string&
	{
		return m_sSongDir;
	}

	/**
	 * @brief Filename associated with this file.
	 * This will always have a .SSC extension. If we loaded a .SSC, this will
	 * point to it, but if we loaded any other type, this will point to
	 * a generated .SSC filename. */
	std::string m_sSongFileName;

	/** @brief The group this Song is in. */
	std::string m_sGroupName;

	/**
	 * @brief the Profile this came from.
	 * This is ProfileSlot_Invalid if it wasn't loaded from a profile. */
	ProfileSlot m_LoadedFromProfile;
	/** @brief Is the song file itself a symlink to another file? */
	bool m_bIsSymLink;
	bool m_bEnabled;

	/** @brief The title of the Song. */
	std::string m_sMainTitle;
	/** @brief The subtitle of the Song, if it exists. */
	std::string m_sSubTitle;
	/** @brief The artist of the Song, if it exists. */
	std::string m_sArtist;
	/** @brief The transliterated title of the Song, if it exists. */
	std::string m_sMainTitleTranslit;
	/** @brief The transliterated subtitle of the Song, if it exists. */
	std::string m_sSubTitleTranslit;
	/** @brief The transliterated artist of the Song, if it exists. */
	std::string m_sArtistTranslit;

	/* If PREFSMAN->m_bShowNative is off, these are the same as GetTranslit*
	 * below. Otherwise, they return the main titles. */
	[[nodiscard]] auto GetDisplayMainTitle() const -> const std::string&;
	[[nodiscard]] auto GetDisplaySubTitle() const -> const std::string&;
	[[nodiscard]] auto GetDisplayArtist() const -> const std::string&;
	[[nodiscard]] auto GetMainTitle() const -> const std::string&;

	/**
	 * @brief Retrieve the transliterated title, or the main title if there is
	 * no translit.
	 * @return the proper title. */
	[[nodiscard]] auto GetTranslitMainTitle() const -> const std::string&
	{
		return static_cast<unsigned int>(!m_sMainTitleTranslit.empty()) != 0U
				 ? m_sMainTitleTranslit
				 : m_sMainTitle;
	}

	auto GetStepsToSave(bool bSavingCache = true, const std::string& path = "")
	  -> std::vector<Steps*>;

	/**
	 * @brief Retrieve the transliterated subtitle, or the main subtitle if
	 * there is no translit.
	 * @return the proper subtitle. */
	[[nodiscard]] auto GetTranslitSubTitle() const -> const std::string&
	{
		return static_cast<unsigned int>(!m_sSubTitleTranslit.empty()) != 0U
				 ? m_sSubTitleTranslit
				 : m_sSubTitle;
	}
	/**
	 * @brief Retrieve the transliterated artist, or the main artist if there is
	 * no translit.
	 * @return the proper artist. */
	[[nodiscard]] auto GetTranslitArtist() const -> const std::string&
	{
		return static_cast<unsigned int>(!m_sArtistTranslit.empty()) != 0U
				 ? m_sArtistTranslit
				 : m_sArtist;
	}

	// "title subtitle"
	std::string displayfulltitle;
	std::string translitfulltitle;
	[[nodiscard]] auto GetDisplayFullTitle() const -> const std::string&
	{
		return displayfulltitle;
	}
	[[nodiscard]] auto GetTranslitFullTitle() const -> const std::string&
	{
		return translitfulltitle;
	}

	/** @brief The version of the song/file. */
	float m_fVersion;
	/** @brief The genre of the song/file. */
	std::string m_sGenre;

	/**
	 * @brief The person who worked with the song file who should be credited.
	 * This is read and saved, but never actually used. */
	std::string m_sCredit;

	std::string m_sOrigin; // song origin (for .ssc format)

	std::string m_sMusicFile;
	std::string m_PreviewFile;
	std::string m_sInstrumentTrackFile[NUM_InstrumentTrack];

	/** @brief The length of the music file. */
	float m_fMusicLengthSeconds;
	float m_fMusicSampleStartSeconds;
	float m_fMusicSampleLengthSeconds;
	DisplayBPM m_DisplayBPMType;
	float m_fSpecifiedBPMMin;
	float m_fSpecifiedBPMMax; // if a range, then Min != Max

	std::string m_sBannerFile; // typically a 16:5 ratio graphic (e.g. 256x80)
	std::string m_sJacketFile; // typically square (e.g. 192x192, 256x256)
	std::string m_sCDFile;	   // square (e.g. 128x128 [DDR 1st-3rd])
	std::string
	  m_sDiscFile; // rectangular (e.g. 256x192 [Pump], 200x150 [MGD3])
	std::string m_sLyricsFile;
	std::string m_sBackgroundFile;
	std::string m_sCDTitleFile;
	std::string m_sPreviewVidFile;

	std::string m_sMusicPath;
	std::string m_PreviewPath;
	std::string m_sInstrumentTrackPath[NUM_InstrumentTrack];
	std::string m_sBannerPath; // typically a 16:5 ratio graphic (e.g. 256x80)
	std::string m_sJacketPath; // typically square (e.g. 192x192, 256x256)
	std::string m_sCDPath;	   // square (e.g. 128x128 [DDR 1st-3rd])
	std::string
	  m_sDiscPath; // rectangular (e.g. 256x192 [Pump], 200x150 [MGD3])
	std::string m_sLyricsPath;
	std::string m_sBackgroundPath;
	std::string m_sCDTitlePath;
	std::string m_sPreviewVidPath;

	std::vector<std::string> ImageDir;

	static auto GetSongAssetPath(std::string sPath,
								 const std::string& sSongPath) -> std::string;
	[[nodiscard]] auto GetMusicPath() const -> const std::string&
	{
		return m_sMusicPath;
	}
	[[nodiscard]] auto GetInstrumentTrackPath(InstrumentTrack it) const
	  -> const std::string&

	{
		return m_sInstrumentTrackPath[it];
	}
	[[nodiscard]] auto GetBannerPath() const -> const std::string&
	{
		return m_sBannerPath;
	}
	[[nodiscard]] auto GetJacketPath() const -> const std::string&
	{
		return m_sJacketPath;
	}
	[[nodiscard]] auto GetCDImagePath() const -> const std::string&
	{
		return m_sCDPath;
	}
	[[nodiscard]] auto GetDiscPath() const -> const std::string&
	{
		return m_sDiscPath;
	}
	[[nodiscard]] auto GetLyricsPath() const -> const std::string&
	{
		return m_sLyricsPath;
	}
	[[nodiscard]] auto GetBackgroundPath() const -> const std::string&
	{
		return m_sBackgroundPath;
	}
	[[nodiscard]] auto GetCDTitlePath() const -> const std::string&
	{
		return m_sCDTitlePath;
	}
	[[nodiscard]] auto GetPreviewVidPath() const -> const std::string&
	{
		return m_sPreviewVidPath;
	}
	[[nodiscard]] auto GetPreviewMusicPath() const -> const std::string&
	{
		return m_PreviewPath;
	}
	[[nodiscard]] auto GetPreviewStartSeconds() const -> float;
	auto GetCacheFile(const std::string& sType) -> std::string;

	// how have i not jammed anything here yet - mina

	// Get the highest value for a specific skillset across all the steps
	// objects for the song at a given rate
	[[nodiscard]] auto HighestMSDOfSkillset(Skillset x,
											float rate,
											bool filtered_charts_only) const
	  -> float;
	[[nodiscard]] auto IsSkillsetHighestOfChart(Steps* chart,
												Skillset skill,
												float rate) const -> bool;
	/** @brief This functions returns whether it has any chart of the given
	   types with the given rate. If no type is given  it checks all charts.*/
	[[nodiscard]] auto MatchesFilter(
	  float rate,
	  std::vector<Steps*>* vMatchingStepsOut = nullptr) const -> bool;
	[[nodiscard]] auto ChartMatchesFilter(Steps* chart, float rate) const
	  -> bool;
	[[nodiscard]] auto IsChartHighestDifficulty(Steps* chart,
												Skillset skill,
												float rate) const -> bool;
	auto HasChartByHash(const std::string& hash) -> bool;

	// For loading only:
	bool m_bHasMusic, m_bHasBanner, m_bHasBackground;

	[[nodiscard]] auto HasMusic() const -> bool;
	[[nodiscard]] auto HasInstrumentTrack(InstrumentTrack it) const -> bool;
	[[nodiscard]] auto HasBanner() const -> bool;
	[[nodiscard]] auto HasBackground() const -> bool;
	[[nodiscard]] auto HasJacket() const -> bool;
	[[nodiscard]] auto HasCDImage() const -> bool;
	[[nodiscard]] auto HasDisc() const -> bool;
	[[nodiscard]] auto HasCDTitle() const -> bool;
	[[nodiscard]] auto HasBGChanges() const -> bool;
	[[nodiscard]] auto HasLyrics() const -> bool;
	[[nodiscard]] auto HasPreviewVid() const -> bool;

	[[nodiscard]] auto Matches(const std::string& sGroup,
							   const std::string& sSong) const -> bool;

	/** @brief The Song's TimingData. */
	TimingData m_SongTiming;

	[[nodiscard]] auto GetFirstBeat() const -> float;
	[[nodiscard]] auto GetFirstSecond() const -> float;
	[[nodiscard]] auto GetLastBeat() const -> float;
	[[nodiscard]] auto GetLastSecond() const -> float;
	[[nodiscard]] auto GetSpecifiedLastBeat() const -> float;
	[[nodiscard]] auto GetSpecifiedLastSecond() const -> float;

	void SetFirstSecond(float f);
	void SetLastSecond(float f);
	void SetSpecifiedLastSecond(float f);

	using VBackgroundChange = std::vector<BackgroundChange>;

  private:
	/** @brief The first second that a note is hit. */
	float firstSecond;
	/** @brief The last second that a note is hit. */
	float lastSecond;
	/** @brief The last second of the song for playing purposes. */
	float specifiedLastSecond;
	/**
	 * @brief The background changes (sorted by layer) that are for this Song.
	 * This uses a shared_ptr instead of a raw pointer so that the
	 * auto gen'd copy constructor works correctly.
	 * This must be sorted before gameplay. */
	std::shared_ptr<VBackgroundChange>
	  m_BackgroundChanges[NUM_BackgroundLayer];
	/**
	 * @brief The foreground changes that are for this Song.
	 * This uses a shared_ptr instead of a raw pointer so that the
	 * auto gen'd copy constructor works correctly.
	 * This must be sorted before gameplay. */
	std::shared_ptr<VBackgroundChange> m_ForegroundChanges;

	[[nodiscard]] auto GetChangesToVectorString(
	  const std::vector<BackgroundChange>& changes) const
	  -> std::vector<std::string>;

  public:
	[[nodiscard]] auto GetBackgroundChanges(BackgroundLayer bl) const
	  -> const std::vector<BackgroundChange>&;
	auto GetBackgroundChanges(BackgroundLayer bl)
	  -> std::vector<BackgroundChange>&;
	[[nodiscard]] auto GetForegroundChanges() const
	  -> const std::vector<BackgroundChange>&;
	auto GetForegroundChanges() -> std::vector<BackgroundChange>&;

	[[nodiscard]] auto GetBGChanges1ToVectorString() const
	  -> std::vector<std::string>;
	[[nodiscard]] auto GetBGChanges2ToVectorString() const
	  -> std::vector<std::string>;
	[[nodiscard]] auto GetFGChanges1ToVectorString() const
	  -> std::vector<std::string>;

	[[nodiscard]] auto GetInstrumentTracksToVectorString() const
	  -> std::vector<std::string>;

	/**
	 * @brief The list of LyricSegments.
	 * This must be sorted before gameplay. */
	std::vector<LyricSegment> m_LyricSegments;

	void AddBackgroundChange(BackgroundLayer blLayer,
							 const BackgroundChange& seg);
	void AddForegroundChange(const BackgroundChange& seg);
	void AddLyricSegment(const LyricSegment& seg);

	void GetDisplayBpms(DisplayBpms& AddTo,
						bool bIgnoreCurrentRate = false) const;
	[[nodiscard]] auto GetBackgroundAtBeat(BackgroundLayer iLayer,
										   float fBeat) const
	  -> const BackgroundChange&;

	auto CreateSteps() -> Steps*;
	void InitSteps(Steps* pSteps);

	auto GetOrTryAtLeastToGetSimfileAuthor() -> std::string;

	[[nodiscard]] auto HasSignificantBpmChangesOrStops() const -> bool;
	[[nodiscard]] auto GetStepsSeconds() const -> float;
	[[nodiscard]] auto IsLong() const -> bool;
	[[nodiscard]] auto IsMarathon() const -> bool;

	// plays music for chart preview and is available to lua -mina
	void PlaySampleMusicExtended();

	auto SongCompleteForStyle(const Style* st) const -> bool;
	[[nodiscard]] auto HasStepsType(StepsType st) const -> bool;
	[[nodiscard]] auto HasStepsTypeAndDifficulty(StepsType st,
												 Difficulty dc) const -> bool;
	[[nodiscard]] auto GetAllSteps() const -> const std::vector<Steps*>&
	{
		return m_vpSteps;
	}
	[[nodiscard]] auto GetStepsByStepsType(StepsType st) const
	  -> const std::vector<Steps*>&
	{
		return m_vpStepsByType[st];
	}
	/** @brief Get the steps of all types within the current game mode */
	[[nodiscard]] auto GetChartsOfCurrentGameMode() const
	  -> std::vector<Steps*>;
	[[nodiscard]] auto GetChartsMatchingFilter() const -> std::vector<Steps*>;
	[[nodiscard]] auto HasEdits(StepsType st) const -> bool;

	auto IsFavorited() const -> bool { return isfavorited; }
	void SetFavorited(bool b) { isfavorited = b; }
	auto HasGoal() const -> bool { return hasgoal; }
	void SetHasGoal(bool b) { hasgoal = b; }
	auto IsPermaMirror() const -> bool { return permamirror; }
	void SetPermaMirror(bool b) { permamirror = b; }

	void SetEnabled(bool b) { m_bEnabled = b; }
	[[nodiscard]] auto GetEnabled() const -> bool { return m_bEnabled; }

	/**
	 * @brief Add the chosen Steps to the Song.
	 * We are responsible for deleting the memory pointed to by pSteps!
	 * @param pSteps the new steps. */
	void AddSteps(Steps* pSteps);
	void DeleteSteps(const Steps* pSteps, bool bReAutoGen = true);

	auto IsEditAlreadyLoaded(Steps* pSteps) const -> bool;

	auto IsStepsUsingDifferentTiming(Steps* pSteps) const -> bool;
	[[nodiscard]] auto AnyChartUsesSplitTiming() const -> bool;
	void UnloadAllCalcDebugOutput();
	/**
	 * @brief An array of keysound file names (e.g. "beep.wav").
	 * The index in this array corresponds to the index in TapNote.
	 * If you  change the index in here, you must change all NoteData too.
	 * Any note that doesn't have a value in the range of this array
	 * means "this note doesn't have a keysound". */
	std::vector<std::string> m_vsKeysoundFile;

	CachedObject<Song> m_CachedObject;

	// Lua
	void PushSelf(lua_State* L);

	std::vector<Steps*> m_vpSteps;
	std::vector<Steps*> m_UnknownStyleSteps;

  private:
	bool isfavorited = false;
	bool permamirror = false;
	bool hasgoal = false;
	/** @brief the Steps that belong to this Song. */
	/** @brief the Steps of a particular StepsType that belong to this Song. */
	std::vector<Steps*> m_vpStepsByType[NUM_StepsType];
	/** @brief the Steps that are of unrecognized Styles. */
};

#endif
