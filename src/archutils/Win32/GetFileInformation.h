/* Windows-specific file helpers. */

#ifndef GET_FILE_INFORMATION_H
#define GET_FILE_INFORMATION_H

bool
GetFileVersion(const std::string& fsFile, std::string& sOut);
std::string
FindSystemFile(const std::string& sFile);
bool
GetProcessFileName(uint32_t iProcessID, std::string& sName);

#endif
