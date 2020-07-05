#ifndef FONT_CHAR_ALIASES
#define FONT_CHAR_ALIASES

/** @brief Provides support for nonstandard characters in text. */
namespace FontCharAliases {
void
ReplaceMarkers(std::string& sText);
bool
GetChar(std::string& codepoint, wchar_t& ch);
};

#endif
