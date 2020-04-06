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
extern const RString USER_PACKAGES_DIR;
/** @brief The system packages directory.
 *
 * This is not the user packages directory. */
extern const RString PACKAGES_DIR;
extern const RString KEYMAPS_PATH;
/** @brief Edit Mode keymaps are separate from standard keymaps because
 * it should not change with the gametype, and to avoid possible
 * interference with the normal keymaps system. -Kyz */
extern const RString EDIT_MODE_KEYMAPS_PATH;
extern const RString PREFERENCES_INI_PATH;
/** @brief The directory that contains the themes. */
extern const RString THEMES_DIR;
/** @brief The directory that contains the different languages. */
extern const RString LANGUAGES_SUBDIR;
/** @brief The base language for most users of this program. */
extern const RString BASE_LANGUAGE;
extern const RString METRICS_FILE;
extern const RString CACHE_DIR;
extern const RString BASE_THEME_NAME;
extern const RString DEFAULTS_INI_PATH;
extern const RString STATIC_INI_PATH;
extern const RString TYPE_TXT_FILE;
/** @brief The default Songs directory. */
extern const RString SONGS_DIR;
/** @brief The default noteskins directory. */
extern const RString NOTESKINS_DIR;
}

#endif
