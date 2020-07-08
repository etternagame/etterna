#ifndef FONT_CHARMAPS_H
#define FONT_CHARMAPS_H
#include <string>

/** @brief Defines common frame to character mappings for Fonts. */
namespace FontCharmaps {
extern const wchar_t M_SKIP;
const wchar_t*
get_char_map(std::string name);
};

#endif
