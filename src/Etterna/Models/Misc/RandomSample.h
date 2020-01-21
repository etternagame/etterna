/* RandomSample - Holds multiple sounds samples and can play a random sound
 * easily. */

#ifndef RANDOM_SAMPLE_H
#define RANDOM_SAMPLE_H

class RageSound;

class RandomSample
{
  public:
	RandomSample();
	virtual ~RandomSample();

	bool Load(const RString& sFilePath, int iMaxToLoad = 1000 /*load all*/);
	void UnloadAll();
	void PlayRandom();
	void PlayCopyOfRandom();
	void Stop();

  private:
	bool LoadSoundDir(RString sDir, int iMaxToLoad);
	bool LoadSound(const RString& sSoundFilePath);
	int GetNextToPlay();

	vector<RageSound*> m_pSamples;
	int m_iIndexLastPlayed;
};

#endif
