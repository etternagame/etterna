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
	auto operator=(const pos_map_queue& rhs) -> pos_map_queue&;

	/* Insert a mapping from iSourceFrame to iDestFrame, containing iFrames. */
	void Insert(int64_t iSourceFrame,
				int iFrames,
				int64_t iDestFrame,
				float fSourceToDestRatio = 1.0F);

	/* Return the iDestFrame for the given iSourceFrame. */
	auto Search(int64_t iSourceFrame, bool* bApproximate) const -> int64_t;

	/* Erase all mappings. */
	void Clear();

	[[nodiscard]] auto IsEmpty() const -> bool;

  private:
	pos_map_impl* m_pImpl;
};

#endif
