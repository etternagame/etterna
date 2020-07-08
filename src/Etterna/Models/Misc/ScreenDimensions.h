/** @brief ScreenDimensions - defines for screen resolutions. */

#ifndef SCREEN_DIMENSIONS_H
#define SCREEN_DIMENSIONS_H

namespace ScreenDimensions {
auto
GetThemeAspectRatio() -> float;
auto
GetScreenWidth() -> float;
auto
GetScreenHeight() -> float;
void
ReloadScreenDimensions();
};

#define SCREEN_WIDTH ScreenDimensions::GetScreenWidth()
#define SCREEN_HEIGHT ScreenDimensions::GetScreenHeight()

#define SCREEN_LEFT (0)
#define SCREEN_RIGHT (SCREEN_WIDTH)
#define SCREEN_TOP (0)
#define SCREEN_BOTTOM (SCREEN_HEIGHT)

#define SCREEN_CENTER_X (SCREEN_LEFT + (SCREEN_RIGHT - SCREEN_LEFT) / 2.0F)
#define SCREEN_CENTER_Y (SCREEN_TOP + (SCREEN_BOTTOM - SCREEN_TOP) / 2.0f)

#define THEME_NATIVE_ASPECT (THEME_SCREEN_WIDTH / THEME_SCREEN_HEIGHT)
#define ASPECT_SCALE_FACTOR                                                    \
	((SCREEN_WIDTH / SCREEN_HEIGHT) / THEME_NATIVE_ASPECT)

#define FullScreenRectF                                                        \
	RectF(SCREEN_LEFT, SCREEN_TOP, SCREEN_RIGHT, SCREEN_BOTTOM)

/**
 * @brief The size of the arrows.
 *
 * This is referenced in ArrowEffects, GameManager, NoteField, and SnapDisplay.
 * XXX: doesn't always have to be 64. -aj
 */
#define ARROW_SIZE (64)

#endif
