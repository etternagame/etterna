/* RegistryAccess - Windows registry helpers */

#ifndef REGISTRY_ACCESS_H
#define REGISTRY_ACCESS_H

#include <vector>

namespace RegistryAccess {
bool
GetRegValue(const RString& sKey, const RString& sName, RString& val);
bool
GetRegValue(const RString& sKey,
			const RString& sName,
			int& val,
			bool bWarnOnError = true);
bool
GetRegValue(const RString& sKey, const RString& sName, bool& val);

bool
GetRegSubKeys(const RString& sKey,
			  std::vector<RString>& asList,
			  const RString& sRegex = ".*",
			  bool bReturnPathToo = true);

bool
SetRegValue(const RString& sKey, const RString& sName, const RString& val);
bool
SetRegValue(const RString& sKey, const RString& sName, int val);
bool
SetRegValue(const RString& sKey, const RString& sName, bool val);

bool
CreateKey(const RString& sKey);
}

#endif
