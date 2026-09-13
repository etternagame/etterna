# macOS OpenGL window regression

Etterna's macOS renderer uses OpenGL. Native window changes must update its
context before the next frame, using the content view's drawable dimensions.
Cocoa window-frame insets change when entering native full screen; subtracting
a fixed titled-window border produces an undersized viewport. Window moves,
backing-scale changes and display changes also require a context update, even
when the logical window size stays the same.

The macOS backend applies these updates synchronously at a frame boundary and
coalesces notifications. The view's existing Retina policy is preserved: backing
pixels are used only when `wantsBestResolutionOpenGLSurface` is enabled. This
avoids silently increasing GPU workload on Retina displays. Display refresh
rate is read from the window's screen, with the ProMotion maximum as fallback
when Core Graphics does not provide a fixed refresh rate.

VSync must be applied to the actual windowed GL contexts as well as reported in
`ActualVideoModeParams`. Previously the windowed path only changed the reported
preference, leaving the driver's swap interval untouched. A resolution change
also used to present an extra blank frame through a nested `BeginFrame/EndFrame`;
the next ordinary frame now clears and redraws the updated drawable.

## Native integration test

Use an isolated checkout/build with its own `Save` directory. The test opens a
real game window, changes its size, maximizes/restores it, enters/exits native
full screen, captures the game's framebuffer and exits the process. It must not
be run during a playing session. It needs no Accessibility or screen-recording
permission: it operates only on the process into which it is loaded.

Build Etterna for ARM64 in Release mode as described in `Building.md`, with
`WITH_VULKAN=OFF` and optionally `WITH_CRASHPAD=OFF`. The test does not exercise
legacy exclusive display capture or multi-monitor configurations.

Compile the test library from the repository root:

```sh
clang++ -std=c++20 -dynamiclib \
  -framework Cocoa -framework OpenGL -framework ImageIO \
  -DGL_SILENCE_DEPRECATION -Wno-deprecated-declarations \
  GameTools/macOS/window-regression.mm -o /tmp/etterna-window-regression.dylib
```

In the isolated game's `Save/Preferences.ini`, use `Windowed=1`,
`FullscreenIsBorderlessWindow=0` and set `Vsync` to the value to test (`0` or `1`).
Disable `ResetVideoSettingsWithNewGPU` so first-run defaults do not replace the
test settings. Run from the directory containing `Etterna.app` and the resources:

```sh
DYLD_INSERT_LIBRARIES=/tmp/etterna-window-regression.dylib \
ETTERNA_WINDOW_TEST_LOG=/tmp/etterna-window-test.log \
  ./Etterna.app/Contents/MacOS/Etterna
```

The exit code is zero only when all seven transitions pass. The log checks
viewport dimensions, actual GL swap interval, continued frame submission, and
native full-screen state. PNG captures are written beside the log. A watchdog
fails the test if window/render synchronization hangs. Run once for each VSync
setting. Frame counts cover transitions and are diagnostic, not a gameplay FPS
benchmark; captures also introduce GPU readback overhead.

Apple's documentation explains [context updates and swap intervals](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_contexts/opengl_contexts.html)
and [the requirement to serialize access to a context](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_threading/opengl_threading.html).
