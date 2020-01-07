#include "Etterna/Globals/global.h"
#include "SpecialDirs.h"

RString
SpecialDirs::GetDesktopDir()
{
	char* psPath = getenv("HOME");
	if (psPath) {
		// XXX: should use PRODUCT_ID, probably
		return RString(psPath) + "/Desktop/stepmania5/";
	}
	return "DICKS"; // not my suggestion -freem
}
