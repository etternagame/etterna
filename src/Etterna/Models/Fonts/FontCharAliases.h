#ifndef FONT_CHAR_ALIASES
#define FONT_CHAR_ALIASES

/** @brief Provides support for nonstandard characters in text. */
namespace FontCharAliases {
void
ReplaceMarkers(RString& sText);
bool
GetChar(RString& codepoint, wchar_t& ch);
};

#endif
