#ifndef STYLEUTIL_H
#define STYLEUTIL_H

class Style;
class Song;
class XNode;

class StyleID
{
	std::string sGame;
	std::string sStyle;

  public:
	StyleID()
	  : sGame("")
	  , sStyle("")
	{
	}
	void Unset() { FromStyle(nullptr); }
	void FromStyle(const Style* p);
	const Style* ToStyle() const;
	bool operator<(const StyleID& rhs) const;

	XNode* CreateNode() const;
	void LoadFromNode(const XNode* pNode);
	bool IsValid() const;
	static void FlushCache(Song* pStaleSong);
};

#endif
