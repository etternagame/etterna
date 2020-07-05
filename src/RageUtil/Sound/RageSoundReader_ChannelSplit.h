/* RageSoundReader_ChannelSplit - Split a sound channels into separate sounds.
 */

#ifndef RAGE_SOUND_READER_CHANNEL_SPLIT
#define RAGE_SOUND_READER_CHANNEL_SPLIT

#include "RageSoundReader.h"

class RageSoundSplitterImpl;

class RageSoundReader_Split : public RageSoundReader
{
  public:
	RageSoundReader_Split(const RageSoundReader_Split& cpy);
	~RageSoundReader_Split() override;
	RageSoundReader_Split* Copy() const override
	{
		return new RageSoundReader_Split(*this);
	}

	int GetLength() const override;
	int GetLength_Fast() const override;
	int SetPosition(int iFrame) override;
	int Read(float* pBuf, int iFrames) override;
	int GetSampleRate() const override;
	unsigned GetNumChannels() const override;
	bool SetProperty(const std::string& sProperty, float fValue) override;
	int GetNextSourceFrame() const override;
	float GetStreamToSourceRatio() const override;
	std::string GetError() const override;

	void AddSourceChannelToSound(int iFromChannel, int iToChannel);

  private:
	RageSoundReader_Split(
	  RageSoundSplitterImpl* pImpl); // create with RageSoundSplitter
	friend class RageSoundSplitterImpl;
	friend class RageSoundSplitter;

	RageSoundSplitterImpl* m_pImpl;
	struct ChannelMap
	{
		int m_iFromChannel;
		int m_iToChannel;
		ChannelMap(int iFromChannel, int iToChannel)
		{
			m_iFromChannel = iFromChannel;
			m_iToChannel = iToChannel;
		}
	};
	vector<ChannelMap> m_aChannels;

	int m_iPositionFrame;
	int m_iRequestFrames;
	int m_iNumOutputChannels;
};

class RageSoundSplitter
{
  public:
	RageSoundSplitter(RageSoundReader* pSource);
	~RageSoundSplitter();
	RageSoundReader_Split* CreateSound();

  private:
	RageSoundSplitterImpl* m_pImpl;
};

#endif
