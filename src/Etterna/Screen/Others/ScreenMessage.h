#ifndef ScreenMessage_H
#define ScreenMessage_H
/** @brief Definition of common ScreenMessages and helpers. */
using ScreenMessage = std::string;

extern const ScreenMessage SM_Invalid;
extern const ScreenMessage SM_None;
extern const ScreenMessage SM_MenuTimer;
extern const ScreenMessage SM_DoneFadingIn;
extern const ScreenMessage SM_BeginFadingOut;
extern const ScreenMessage SM_GoToNextScreen;
extern const ScreenMessage SM_GoToDisconnectScreen;
extern const ScreenMessage SM_GoToPrevScreen;
extern const ScreenMessage SM_GainFocus;
extern const ScreenMessage SM_LoseFocus;
extern const ScreenMessage SM_Pause;
extern const ScreenMessage SM_Success;
extern const ScreenMessage SM_Failure;
/** @brief Helpers for the ScreenMessages. */
namespace ScreenMessageHelpers {
auto
ToScreenMessage(const std::string& Name) -> ScreenMessage;
auto
ScreenMessageToString(ScreenMessage SM) -> std::string;
};

/** @brief Automatically generate a unique ScreenMessage value */
#define AutoScreenMessage(x)                                                   \
	const ScreenMessage x = ScreenMessageHelpers::ToScreenMessage(#x)

#endif
