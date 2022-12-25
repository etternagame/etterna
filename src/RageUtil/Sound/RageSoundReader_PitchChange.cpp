/*
 * Implements properties:
 *   "Speed" - cause the sound to play faster or slower
 *   "Pitch" - raise or lower the pitch of the sound
 *
 * Pitch changing is implemented by combining speed changing and rate changing
 * (via resampling).  The resampler needs to be changed later than the speed
 * changer; this class just controls parameters on the other two real filters.
 */

#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageSoundReader_PitchChange.h"
#include "RageSoundReader_Resample_Good.h"
#include "RageSoundReader_SpeedChange.h"

RageSoundReader_PitchChange::RageSoundReader_PitchChange(
  RageSoundReader* pSource)
  : RageSoundReader_Filter(nullptr)
{
	m_pSpeedChange = new RageSoundReader_SpeedChange(pSource);
	m_pResample = new RageSoundReader_Resample_Good(
	  m_pSpeedChange, m_pSpeedChange->GetSampleRate());
	m_pSource = std::unique_ptr<RageSoundReader_Resample_Good>(m_pResample);
	m_fSpeedRatio = 1.0f;
	m_fPitchRatio = 1.0f;
	m_fLastSetSpeedRatio = m_fSpeedRatio;
	m_fLastSetPitchRatio = m_fPitchRatio;
}

RageSoundReader_PitchChange::RageSoundReader_PitchChange(
  const RageSoundReader_PitchChange& cpy)
  : RageSoundReader_Filter(cpy)
{
	/* The source tree has already been copied.  Our source is m_pResample; its
	 * source is m_pSpeedChange (and its source is a copy of the pSource we were
	 * initialized with). */
	auto prsmpl = dynamic_cast<RageSoundReader_Resample_Good*>(&*m_pSource);
	ASSERT_M(prsmpl != nullptr,
			 "Dynamic cast to RageSoundReader Resample failed at runtime.");
	m_pResample = prsmpl;
	auto pspdchng =
	  dynamic_cast<RageSoundReader_SpeedChange*>(m_pResample->GetSource());
	ASSERT_M(pspdchng != nullptr,
			 "Dynamic cast to RageSoundReader SpeedChange failed at runtime.");
	m_pSpeedChange = pspdchng;
	m_fSpeedRatio = cpy.m_fSpeedRatio;
	m_fPitchRatio = cpy.m_fPitchRatio;
	m_fLastSetSpeedRatio = cpy.m_fLastSetSpeedRatio;
	m_fLastSetPitchRatio = cpy.m_fLastSetPitchRatio;
}

int
RageSoundReader_PitchChange::Read(float* pBuf, int iFrames)
{
	/* m_pSpeedChange->NextReadWillStep is true if speed changes will be applied
	 * immediately on the next Read().  When this is true, apply the ratio to
	 * the resampler and the speed changer simultaneously, so they take effect
	 * as closely together as possible. */
	if ((m_fLastSetSpeedRatio != m_fSpeedRatio ||
		 m_fLastSetPitchRatio != m_fPitchRatio) &&
		m_pSpeedChange->NextReadWillStep()) {
		float fRate = GetStreamToSourceRatio();

		/* This is the simple way: */
		// m_pResample->SetRate( m_fPitchRatio );
		// m_pSpeedChange->SetSpeedRatio( m_fSpeedRatio / m_fPitchRatio );

		/* However, the resampler has a limited granularity due to internal
		 * fixed- point math, and the actual ratio will be slightly different
		 * than what we tell it to use.  The actual ratio used is
		 * fActualPitchRatio. */
		m_pResample->SetRate(m_fPitchRatio);
		float fActualPitchRatio = m_pResample->GetRate();
		float fRequestedSpeedRatio = m_fSpeedRatio / fActualPitchRatio;
		m_pSpeedChange->SetSpeedRatio(fRequestedSpeedRatio);

		m_fLastSetSpeedRatio = m_fSpeedRatio;
		m_fLastSetPitchRatio = m_fPitchRatio;

		/* If we just applied a new speed and it caused the ratio to change,
		 * return no data, so the caller can see the new ratio. */
		if (fRate != GetStreamToSourceRatio())
			return 0;
	}

	return RageSoundReader_Filter::Read(pBuf, iFrames);
}

bool
RageSoundReader_PitchChange::SetProperty(const std::string& sProperty,
										 float fValue)
{
	if (sProperty == "Rate") {
		/* Don't propagate this.  m_pResample will take it, but it's under
		 * our control. */
		return false;
	}
	if (sProperty == "Speed") {
		/* HACK: Put rate functions back together,
		   this needs to be refactored. */
		SetSpeedRatio(fValue);
		if (PREFSMAN->EnablePitchRates)
			SetPitchRatio(fValue);
		return true;
	}

	if (sProperty == "Pitch") {
		SetPitchRatio(fValue);
		return true;
	}

	return RageSoundReader_Filter::SetProperty(sProperty, fValue);
}
