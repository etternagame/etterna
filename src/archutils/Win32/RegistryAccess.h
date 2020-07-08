/* RegistryAccess - Windows registry helpers */

#ifndef REGISTRY_ACCESS_H
#define REGISTRY_ACCESS_H

namespace RegistryAccess {
bool
GetRegValue(const std::string& sKey,
			const std::string& sName,
			std::string& val);
bool
GetRegValue(const std::string& sKey,
			const std::string& sName,
			int& val,
			bool bWarnOnError = true);
bool
GetRegValue(const std::string& sKey, const std::string& sName, bool& val);

bool
GetRegSubKeys(const std::string& sKey,
			  vector<std::string>& asList,
			  const std::string& sRegex = ".*",
			  bool bReturnPathToo = true);

bool
SetRegValue(const std::string& sKey,
			const std::string& sName,
			const std::string& val);
bool
SetRegValue(const std::string& sKey, const std::string& sName, int val);
bool
SetRegValue(const std::string& sKey, const std::string& sName, bool val);

bool
CreateKey(const std::string& sKey);
}

#endif
