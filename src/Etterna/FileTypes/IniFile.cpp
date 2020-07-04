/*
http://en.wikipedia.org/wiki/INI_file
 - names and values are trimmed on both sides
 - semicolons start a comment line
 - backslash followed by a newline doesn't break the line
*/
#include "Etterna/Globals/global.h"
#include "IniFile.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"

IniFile::IniFile()
  : XNode("IniFile")
{
}

bool
IniFile::ReadFile(const std::string& sPath)
{
	m_sPath = sPath;
	CHECKPOINT_M(ssprintf("Reading '%s'", m_sPath.c_str()));

	RageFile f;
	if (!f.Open(m_sPath)) {
		LOG->Trace(
		  "Reading '%s' failed: %s", m_sPath.c_str(), f.GetError().c_str());
		m_sError = f.GetError();
		return false;
	}

	return ReadFile(f);
}

bool
IniFile::ReadFile(RageFileBasic& f)
{
	std::string keyname;
	// keychild is used to cache the node that values are being added to. -Kyz
	XNode* keychild = NULL;
	for (;;) {
		std::string line;
		// Read lines until we reach a line that doesn't end in a backslash
		for (;;) {
			std::string s;
			switch (f.GetLine(s)) {
				case -1:
					m_sError = f.GetError();
					return false;
				case 0:
					return true; // eof
			}

			utf8_remove_bom(s);

			line += s;

			if (line.empty() || line[line.size() - 1] != '\\') {
				break;
			}
			line.erase(line.end() - 1);
		}

		if (line.empty())
			continue;
		switch (line[0]) {
			case ';':
			case '#':
				continue; // comment
			case '/':
			case '-':
				if (line.size() > 1 && line[0] == line[1]) {
					continue;
				} // comment (Lua or C++ style)
				goto keyvalue;
			case '[':
				if (line[line.size() - 1] == ']') {
					// New section.
					keyname = line.substr(1, line.size() - 2);
					keychild = GetChild(keyname);
					if (keychild == NULL) {
						keychild = AppendChild(keyname);
					}
					break;
				}
			default:
			keyvalue:
				if (keychild == NULL) {
					break;
				}
				// New value.
				size_t iEqualIndex = line.find("=");
				if (iEqualIndex != string::npos) {
					std::string valuename =
					  line.Left(static_cast<int>(iEqualIndex));
					std::string value =
					  line.Right(line.size() - valuename.size() - 1);
					Trim(valuename);
					if (!valuename.empty()) {
						SetKeyValue(keychild, valuename, value);
					}
				}
				break;
		}
	}
}

bool
IniFile::WriteFile(const std::string& sPath) const
{
	RageFile f;
	if (!f.Open(sPath, RageFile::WRITE)) {
		LOG->Warn(
		  "Writing '%s' failed: %s", sPath.c_str(), f.GetError().c_str());
		m_sError = f.GetError();
		return false;
	}

	bool bSuccess = IniFile::WriteFile(f);
	int iFlush = f.Flush();
	bSuccess &= static_cast<int>(iFlush != -1);
	return bSuccess;
}

bool
IniFile::WriteFile(RageFileBasic& f) const
{
	FOREACH_CONST_Child(this, pKey)
	{
		std::string keyName = "[" + pKey->GetName() + "]";
		if (f.PutLine(keyName) == -1) {
			m_sError = f.GetError();
			return false;
		}

		FOREACH_CONST_Attr(pKey, pAttr)
		{
			const std::string& sName = pAttr->first;
			const std::string& sValue = pAttr->second->GetValue<std::string>();

			// TODO: Are there escape rules for these?
			// take a cue from how multi-line Lua functions are parsed
			DEBUG_ASSERT(sName.find('\n') == sName.npos);
			DEBUG_ASSERT(sName.find('=') == sName.npos);

			std::string iniSetting = sName + "=" + sValue;
			if (f.PutLine(iniSetting) == -1) {
				m_sError = f.GetError();
				return false;
			}
		}

		if (f.PutLine("") == -1) {
			m_sError = f.GetError();
			return false;
		}
	}
	return true;
}

bool
IniFile::DeleteValue(const std::string& keyname, const std::string& valuename)
{
	XNode* pNode = GetChild(keyname);
	if (pNode == NULL)
		return false;
	return pNode->RemoveAttr(valuename);
}

bool
IniFile::DeleteKey(const std::string& keyname)
{
	XNode* pNode = GetChild(keyname);
	if (pNode == NULL)
		return false;
	return RemoveChild(pNode);
}

bool
IniFile::RenameKey(const std::string& from, const std::string& to)
{
	// If to already exists, do nothing.
	if (GetChild(to) != NULL)
		return false;

	XNode* pNode = GetChild(from);
	if (pNode == NULL)
		return false;

	pNode->SetName(to);
	RenameChildInByName(pNode);

	return true;
}
