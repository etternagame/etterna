#ifndef DisplayResolutions_H
#define DisplayResolutions_H

#include <set>
/** @brief The dimensions of the program. */
class DisplayResolution
{
  public:
	/** @brief The width of the program. */
	unsigned int iWidth;
	/** @brief The height of the program. */
	unsigned int iHeight;
	/** @brief Is this display stretched/used for widescreen? */
	bool bStretched;

	/**
	 * @brief Determine if one DisplayResolution is less than the other.
	 * @param other the other DisplayResolution to check.
	 * @return true if this DisplayResolution is less than the other, or false
	 * otherwise. */
	bool operator<(const DisplayResolution& other) const
	{
/** @brief A quick way to compare the two DisplayResolutions. */
#define COMPARE(x)                                                             \
	if ((x) != other.x)                                                        \
		return (x) < other.x;
		COMPARE(iWidth);
		COMPARE(iHeight);
		COMPARE(bStretched);
#undef COMPARE
		return false;
	}
};
/** @brief The collection of DisplayResolutions available within the program. */
typedef std::set<DisplayResolution> DisplayResolutions;

#endif
