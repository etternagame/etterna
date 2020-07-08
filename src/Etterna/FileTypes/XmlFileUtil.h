#ifndef XML_FILE_UTIL_H
#define XML_FILE_UTIL_H

class RageFileBasic;
class XNode;
struct lua_State;

/**
 * @brief A little graphic to the left of the song's text banner in the
 * MusicWheel.
 *
 * This is designed to help work with XML files. */
namespace XmlFileUtil {
auto
LoadFromFileShowErrors(XNode& xml, const std::string& sFile) -> bool;
auto
LoadFromFileShowErrors(XNode& xml, RageFileBasic& f) -> bool;

// Load/Save XML
void
Load(XNode* pNode, const std::string& sXml, std::string& sErrorOut);
auto
GetXML(const XNode* pNode, RageFileBasic& f, bool bWriteTabs = true) -> bool;
auto
GetXML(const XNode* pNode) -> std::string;
auto
SaveToFile(const XNode* pNode,
		   const std::string& sFile,
		   const std::string& sStylesheet = "",
		   bool bWriteTabs = true) -> bool;
auto
SaveToFile(const XNode* pNode,
		   RageFileBasic& f,
		   const std::string& sStylesheet = "",
		   bool bWriteTabs = true) -> bool;

void
AnnotateXNodeTree(XNode* pNode, const std::string& sFile);
void
CompileXNodeTree(XNode* pNode, const std::string& sFile);
auto
XNodeFromTable(lua_State* L) -> XNode*;

void
MergeIniUnder(XNode* pFrom, XNode* pTo);
} // namespace XmlFileUtil

#endif
