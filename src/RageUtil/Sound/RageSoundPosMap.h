/* pos_map_queue - A container that maps one set of frame numbers to another. */

#ifndef RAGE_SOUND_POS_MAP_H
#define RAGE_SOUND_POS_MAP_H

struct pos_map_impl;
class pos_map_queue
{
  public:
	pos_map_queue();
	~pos_map_queue();
	pos_map_queue(const pos_map_queue& cpy);
	pos_map_queue& operator=(const pos_map_queue& rhs);

	/* Insert a mapping from iSourceFrame to iDestFrame, containing iFrames. */
	void Insert(int64_t iSourceFrame,
				int iFrames,
				int64_t iDestFrame,
				float fSourceToDestRatio = 1.0f);

	/* Return the iDestFrame for the given iSourceFrame. */
	int64_t Search(int64_t iSourceFrame, bool* bApproximate) const;

	/* Erase all mappings. */
	void Clear();

	bool IsEmpty() const;

  private:
	pos_map_impl* m_pImpl;
};

#endif
