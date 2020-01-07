/* CsvFile - Reading and writing .CSV files. */

#ifndef CsvFile_H
#define CsvFile_H

#include <vector>

class RageFileBasic;

class CsvFile
{
  public:
	CsvFile();

	bool ReadFile(const RString& sPath);
	bool ReadFile(RageFileBasic& sFile);
	bool WriteFile(const RString& sPath) const;
	bool WriteFile(RageFileBasic& sFile) const;

	typedef std::vector<RString> StringVector;
	std::vector<StringVector> m_vvs;

  private:
	RString m_sPath;
	mutable RString m_sError;
};

#endif
