#include "CrashpadHandler.hpp"
#include "Core/Services/Locator.hpp"

bool Core::Crash::initCrashpad() {
	Locator::getLogger()->warn("This build was compiled without crashpad. Crash dumps will not be generated.");
	return false;
}

void Core::Crash::generateMinidump() {
	Locator::getLogger()->warn("This build was compiled without crashpad. A minidump cannot be generated.");
}
