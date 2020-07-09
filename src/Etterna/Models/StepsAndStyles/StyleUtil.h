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
	[[nodiscard]] auto ToStyle() const -> const Style*;
	auto operator<(const StyleID& rhs) const -> bool;

	[[nodiscard]] auto CreateNode() const -> XNode*;
	void LoadFromNode(const XNode* pNode);
	[[nodiscard]] auto IsValid() const -> bool;
	static void FlushCache(Song* pStaleSong);
};

#endif
