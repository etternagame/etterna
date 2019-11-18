#ifndef AdjustSync_H
#define AdjustSync_H

class TimingData;
/**
 * @brief Allows for adjusting the sync of a song.
 *
 * This class defines two ways of adjusting the sync of a song.
 * The first adjusts only the offset, "on the fly". It can adjust either the
 * song itself or the machine. The other style adjusts both the BPM and the
 * offset of the song, but it needs more data.
 */
class AdjustSync
{
  public:
	/**
	 * @brief The original TimingData before adjustments were made.
	 *
	 * This is designed to work with Split Timing. */
	static vector<TimingData> s_vpTimingDataOriginal;

	static float s_fGlobalOffsetSecondsOriginal;
	/* We only want to call the Reset methods before a song, not immediately
	 * after a song. If we reset it at the end of a song, we have to carefully
	 * check the logic to make sure we never reset it before the user gets a
	 * chance to save or revert the change. Resetting at the start of the song
	 * is sufficient. */
	static void ResetOriginalSyncData();
	static void ResetAutosync();
	static bool IsSyncDataChanged();

	static void SaveSyncChanges();
	static void RevertSyncChanges();
	static void HandleAutosync(float fNoteOffBySeconds, float fStepTime);
	static void HandleSongEnd();
	static void AutosyncOffset();
	static void AutosyncTempo();
	static void GetSyncChangeTextGlobal(vector<RString>& vsAddTo);
	static void GetSyncChangeTextSong(vector<RString>& vsAddTo);

	/** @brief The minimum number of steps to hit for syncing purposes. */
	static const int OFFSET_SAMPLE_COUNT = 24;

	static float s_fAutosyncOffset[OFFSET_SAMPLE_COUNT];
	static int s_iAutosyncOffsetSample;
	static float s_fStandardDeviation;

	// Measured in seconds.  If the average error is too high, we
	// reject the recorded data for the Least Squares Regression.
	static const float ERROR_TOO_HIGH;

	static vector<pair<float, float>> s_vAutosyncTempoData;
	static float s_fAverageError;
	static int s_iStepsFiltered;
};

#endif
