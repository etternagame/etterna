#ifndef SAMPLE_HISTORY_H
#define SAMPLE_HISTORY_H
/** @brief Store a trailing history of values, and retrieve values with
 * interpolation. */
class SampleHistory
{
  public:
	SampleHistory();
	void AddSample(float fSample, float fDeltaTime);
	float GetSample(float fSecondsAgo) const;

  private:
	float GetSampleNum(float fSamplesAgo) const;

	vector<float> m_afHistory;
	int m_iLastHistory;
	int m_iHistorySamplesPerSecond;
	float m_fHistorySeconds;
	float m_fToSample;
};

#endif
