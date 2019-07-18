#include "Etterna/Globals/global.h"
#include "CsvFile.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"

CsvFile::CsvFile() = default;

bool
CsvFile::ReadFile(const RString& sPath)
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
CsvFile::ReadFile(RageFileBasic& f)
{
	m_vvs.clear();

	// hi,"hi2,","""hi3"""

	for (;;) {
		RString line;
		switch (f.GetLine(line)) {
			case -1:
				m_sError = f.GetError();
				return false;
			case 0:
				return true; /* eof */
		}

		utf8_remove_bom(line);

		vector<RString> vs;

		while (!line.empty()) {
			if (line[0] == '\"') // quoted value
			{
				line.erase(line.begin()); // eat open quote
				RString::size_type iEnd = 0;
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

				RString sValue = line;
				sValue = sValue.Left(iEnd);
				vs.push_back(sValue);

				line.erase(line.begin(), line.begin() + iEnd);

				if (!line.empty() && line[0] == '\"')
					line.erase(line.begin());
			} else {
				RString::size_type iEnd = line.find(',');
				if (iEnd == line.npos)
					iEnd =
					  line.size(); // didn't find an end.  Take the whole line

				RString sValue = line;
				sValue = sValue.Left(iEnd);
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
CsvFile::WriteFile(const RString& sPath) const
{
	RageFile f;
	if (!f.Open(sPath, RageFile::WRITE)) {
		LOG->Trace(
		  "Writing '%s' failed: %s", sPath.c_str(), f.GetError().c_str());
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
		RString sLine;
		FOREACH_CONST(RString, *line, value)
		{
			RString sVal = *value;
			sVal.Replace("\"", "\"\""); // escape quotes to double-quotes
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
