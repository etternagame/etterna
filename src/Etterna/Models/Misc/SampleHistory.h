#ifndef SAMPLE_HISTORY_H
#define SAMPLE_HISTORY_H
/** @brief Store a trailing history of values, and retrieve values with
 * interpolation. */
class SampleHistory
{
  public:
	SampleHistory();
	void AddSample(float fSample, float fDeltaTime);
	[[nodiscard]] float GetSample(float fSecondsAgo) const;

  private:
	[[nodiscard]] float GetSampleNum(float fSamplesAgo) const;

	std::vector<float> m_afHistory;
	int m_iLastHistory;
	int m_iHistorySamplesPerSecond;
	float m_fHistorySeconds;
	float m_fToSample;
};

#endif
