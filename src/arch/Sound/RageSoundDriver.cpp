#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageSoundDriver.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "arch/arch_default.h"

DriverList RageSoundDriver::m_pDriverList;

RageSoundDriver*
RageSoundDriver::Create(const std::string& drivers)
{
	vector<std::string> drivers_to_try;
	if (drivers.empty()) {
		split(DEFAULT_SOUND_DRIVER_LIST, ",", drivers_to_try);
	} else {
		split(drivers, ",", drivers_to_try);
		size_t to_try = 0;
		bool had_to_erase = false;
		while (to_try < drivers_to_try.size()) {
			if (m_pDriverList.m_pRegistrees->find(
				  istring(drivers_to_try[to_try].c_str())) ==
				m_pDriverList.m_pRegistrees->end()) {
				LOG->Warn("Removed unusable sound driver %s",
						  drivers_to_try[to_try].c_str());
				drivers_to_try.erase(drivers_to_try.begin() + to_try);
				had_to_erase = true;
			} else {
				++to_try;
			}
		}
		if (had_to_erase) {
			SOUNDMAN->fix_bogus_sound_driver_pref(join(",", drivers_to_try));
		}
		if (drivers_to_try.empty()) {
			split(DEFAULT_SOUND_DRIVER_LIST, ",", drivers_to_try);
		}
	}

	FOREACH_CONST(std::string, drivers_to_try, Driver)
	{
		RageDriver* pDriver = m_pDriverList.Create(*Driver);
		if (pDriver == NULL) {
			LOG->Trace("Unknown sound driver: %s", Driver->c_str());
			continue;
		}

		RageSoundDriver* pRet = dynamic_cast<RageSoundDriver*>(pDriver);
		ASSERT(pRet != NULL);

		const std::string sError = pRet->Init();
		if (sError.empty()) {
			if (PREFSMAN->m_verbose_log > 1)
				LOG->Info("Sound driver: %s", Driver->c_str());
			return pRet;
		}
		LOG->Info(
		  "Couldn't load driver %s: %s", Driver->c_str(), sError.c_str());
		SAFE_DELETE(pRet);
	}
	return NULL;
}

std::string
RageSoundDriver::GetDefaultSoundDriverList()
{
	return DEFAULT_SOUND_DRIVER_LIST;
}
