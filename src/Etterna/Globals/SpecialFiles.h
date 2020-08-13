#ifndef SpecialFiles_H
#define SpecialFiles_H

/** @brief The listing of the special files and directories in use. */
namespace SpecialFiles {
/**
 * @brief The user packages directory.
 *
 * This should be separate from system packages so that
 * we can write to it (installing a package).
 */
extern const std::string USER_PACKAGES_DIR;
/** @brief The system packages directory.
 *
 * This is not the user packages directory. */
extern const std::string PACKAGES_DIR;
extern const std::string KEYMAPS_PATH;
/** @brief Edit Mode keymaps are separate from standard keymaps because
 * it should not change with the gametype, and to avoid possible
 * interference with the normal keymaps system. -Kyz */
extern const std::string EDIT_MODE_KEYMAPS_PATH;
extern const std::string PREFERENCES_INI_PATH;
/** @brief The directory that contains the themes. */
extern const std::string THEMES_DIR;
/** @brief The directory that contains the different languages. */
extern const std::string LANGUAGES_SUBDIR;
/** @brief The base language for most users of this program. */
extern const std::string BASE_LANGUAGE;
extern const std::string METRICS_FILE;
extern const std::string CACHE_DIR;
extern const std::string BASE_THEME_NAME;
extern const std::string DEFAULTS_INI_PATH;
extern const std::string STATIC_INI_PATH;
extern const std::string TYPE_TXT_FILE;
/** @brief The default Songs directory. */
extern const std::string SONGS_DIR;
/** @brief The default noteskins directory. */
extern const std::string NOTESKINS_DIR;
}

#endif
