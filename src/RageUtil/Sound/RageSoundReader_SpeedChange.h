/* RageSoundReader_SpeedChange - change the speed of an audio stream without
 * changing its pitch. */

#ifndef RAGE_SOUND_READER_SPEED_CHANGE_H
#define RAGE_SOUND_READER_SPEED_CHANGE_H

#include "RageSoundReader_Filter.h"

class RageSoundReader_SpeedChange : public RageSoundReader_Filter
{
  public:
	RageSoundReader_SpeedChange(RageSoundReader* pSource);

	virtual int SetPosition(int iFrame);
	virtual int Read(float* pBuf, int iFrames);
	virtual RageSoundReader_SpeedChange* Copy() const
	{
		return new RageSoundReader_SpeedChange(*this);
	}
	virtual bool SetProperty(const std::string& sProperty, float fValue);
	virtual int GetNextSourceFrame() const;
	virtual float GetStreamToSourceRatio() const;

	void SetSpeedRatio(float fRatio);

	/* Return true if the next Read() will start a new block, allowing
	 * GetRatio() to be updated to a new value.  Used by
	 * RageSoundReader_PitchChange. */
	bool NextReadWillStep() const { return GetCursorAvail() == 0; }

	/* Get the ratio last set by SetSpeedRatio. */
	float GetRatio() const { return m_fSpeedRatio; }

  protected:
	int FillData(int iMax);
	void EraseData(int iToDelete);
	int Step();
	void Reset();

	int GetCursorAvail() const;

	int GetWindowSizeFrames() const;
	int GetToleranceFrames() const { return GetWindowSizeFrames() / 4; }

	int m_iDataBufferAvailFrames;
	struct ChannelInfo
	{
		vector<float> m_DataBuffer;
		int m_iCorrelatedPos;
		int m_iLastCorrelatedPos;
	};
	vector<ChannelInfo> m_Channels;

	int m_iUncorrelatedPos;
	int m_iPos;

	float m_fSpeedRatio;
	float m_fTrailingSpeedRatio;
	float m_fErrorFrames;
};

#endif
