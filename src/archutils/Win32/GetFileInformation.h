/* Windows-specific file helpers. */

#ifndef GET_FILE_INFORMATION_H
#define GET_FILE_INFORMATION_H

bool
GetFileVersion(const RString& fsFile, RString& sOut);
RString
FindSystemFile(const RString& sFile);
bool
GetProcessFileName(uint32_t iProcessID, RString& sName);

#endif
