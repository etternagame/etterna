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
bool
LoadFromFileShowErrors(XNode& xml, const std::string& sFile);
bool
LoadFromFileShowErrors(XNode& xml, RageFileBasic& f);

// Load/Save XML
void
Load(XNode* pNode, const std::string& sXml, std::string& sErrorOut);
bool
GetXML(const XNode* pNode, RageFileBasic& f, bool bWriteTabs = true);
std::string
GetXML(const XNode* pNode);
bool
SaveToFile(const XNode* pNode,
		   const std::string& sFile,
		   const std::string& sStylesheet = "",
		   bool bWriteTabs = true);
bool
SaveToFile(const XNode* pNode,
		   RageFileBasic& f,
		   const std::string& sStylesheet = "",
		   bool bWriteTabs = true);

void
AnnotateXNodeTree(XNode* pNode, const std::string& sFile);
void
CompileXNodeTree(XNode* pNode, const std::string& sFile);
XNode*
XNodeFromTable(lua_State* L);

void
MergeIniUnder(XNode* pFrom, XNode* pTo);
}

#endif
