#ifndef SAMPLE_HISTORY_H
#define SAMPLE_HISTORY_H
/** @brief Store a trailing history of values, and retrieve values with
 * interpolation. */
class SampleHistory
{
  public:
	SampleHistory();
	void AddSample(float fSample, float fDeltaTime);
	[[nodiscard]] auto GetSample(float fSecondsAgo) const -> float;

  private:
	[[nodiscard]] auto GetSampleNum(float fSamplesAgo) const -> float;

	std::vector<float> m_afHistory{};
	int m_iLastHistory;
	int m_iHistorySamplesPerSecond;
	float m_fHistorySeconds;
	float m_fToSample;
};

#endif
