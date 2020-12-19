#include "Etterna/Globals/global.h"
#include "CsvFile.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "RageUtil/File/RageFile.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"

CsvFile::CsvFile() = default;

bool
CsvFile::ReadFile(const std::string& sPath)
{
	m_sPath = sPath;
	Locator::getLogger()->trace("Reading '{}'", m_sPath.c_str());

	RageFile f;
	if (!f.Open(m_sPath)) {
		Locator::getLogger()->trace("Reading '{}' failed: {}", m_sPath.c_str(), f.GetError().c_str());
		m_sError = f.GetError();
		return false;
	}

	return ReadFile(f);
}

bool
CsvFile::ReadFile(RageFileBasic& f)
{
	m_vvs.clear();

	// hi,"hi2,","""hi3"""

	for (;;) {
		std::string line;
		switch (f.GetLine(line)) {
			case -1:
				m_sError = f.GetError();
				return false;
			case 0:
				return true; /* eof */
		}

		utf8_remove_bom(line);

		vector<std::string> vs;

		while (!line.empty()) {
			if (line[0] == '\"') // quoted value
			{
				line.erase(line.begin()); // eat open quote
				std::string::size_type iEnd = 0;
				do {
					iEnd = line.find('\"', iEnd);
					if (iEnd == line.npos) {
						iEnd = line.size() -
							   1; // didn't find an end.  Take the whole line.
						break;
					}

					if (line.size() > iEnd + 1 &&
						line[iEnd + 1] ==
						  '\"') // next char is also double quote
						iEnd = iEnd + 2;
					else
						break;
				} while (true);

				std::string sValue = line;
				sValue = sValue.substr(0, iEnd);
				vs.push_back(sValue);

				line.erase(line.begin(), line.begin() + iEnd);

				if (!line.empty() && line[0] == '\"')
					line.erase(line.begin());
			} else {
				std::string::size_type iEnd = line.find(',');
				if (iEnd == line.npos)
					iEnd =
					  line.size(); // didn't find an end.  Take the whole line

				std::string sValue = line;
				sValue = sValue.substr(0, iEnd);
				vs.push_back(sValue);

				line.erase(line.begin(), line.begin() + iEnd);
			}

			if (!line.empty() && line[0] == ',')
				line.erase(line.begin());
		}

		m_vvs.push_back(vs);
	}
}

bool
CsvFile::WriteFile(const std::string& sPath) const
{
	RageFile f;
	if (!f.Open(sPath, RageFile::WRITE)) {
		Locator::getLogger()->trace("Writing '{}' failed: {}", sPath.c_str(), f.GetError().c_str());
		m_sError = f.GetError();
		return false;
	}

	return CsvFile::WriteFile(f);
}

bool
CsvFile::WriteFile(RageFileBasic& f) const
{
	FOREACH_CONST(StringVector, m_vvs, line)
	{
		std::string sLine;
		FOREACH_CONST(std::string, *line, value)
		{
			std::string sVal = *value;
			s_replace(sVal, "\"", "\"\""); // escape quotes to double-quotes
			sLine += "\"" + sVal + "\"";
			if (value != line->end() - 1)
				sLine += ",";
		}
		if (f.PutLine(sLine) == -1) {
			m_sError = f.GetError();
			return false;
		}
	}
	return true;
}
