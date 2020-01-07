#include "Etterna/Globals/global.h"
#include "Foreach.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "SampleHistory.h"

inline float
sample_step_size(int samples_per_second)
{
	return 1.0f / samples_per_second;
}

SampleHistory::SampleHistory()
{
	m_iLastHistory = 0;
	m_iHistorySamplesPerSecond = 60;
	m_fHistorySeconds = 0.0f;
	m_fToSample = sample_step_size(m_iHistorySamplesPerSecond);
	m_fHistorySeconds = 10.0f;
	int iSamples = lround(m_iHistorySamplesPerSecond * m_fHistorySeconds);
	m_afHistory.resize(iSamples);
}

float
SampleHistory::GetSampleNum(float fSamplesAgo) const
{
	fSamplesAgo = std::min(fSamplesAgo, static_cast<float>(m_afHistory.size() - 1));
	if (fSamplesAgo < 0)
		fSamplesAgo = 0;
	if (m_afHistory.size() == 0)
		return 0.0f;

	float fSample = m_iLastHistory - fSamplesAgo - 1;

	float f = floorf(fSample);
	int iSample = lround(f);
	int iNextSample = iSample + 1;
	wrap(iSample, m_afHistory.size());
	wrap(iNextSample, m_afHistory.size());

	float p = fSample - f;
	float fRet = lerp(p, m_afHistory[iSample], m_afHistory[iNextSample]);
	//	LOG->Trace( "%.3f: %i, %i, %.3f (f %.3f, %.3f)", fSample, iSample,
	// iNextSample, fRet, f, p );
	return fRet;
}

float
SampleHistory::GetSample(float fSecondsAgo) const
{
	float fSamplesAgo = fSecondsAgo * m_iHistorySamplesPerSecond;
	return GetSampleNum(fSamplesAgo);
}

void
SampleHistory::AddSample(float fSample, float fDeltaTime)
{
	while (fDeltaTime > 0.0001f) {
		float fTime = std::min(m_fToSample, fDeltaTime);
		m_fToSample -= fTime;
		fDeltaTime -= fTime;

		if (m_fToSample < 0.0001f) {
			++m_iLastHistory;
			m_iLastHistory %= m_afHistory.size();
			m_fToSample += sample_step_size(m_iHistorySamplesPerSecond);
		}

		m_afHistory[m_iLastHistory] = fSample;
	}
}
