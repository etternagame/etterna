/* CsvFile - Reading and writing .CSV files. */

#ifndef CsvFile_H
#define CsvFile_H

#include <string>
#include <vector>

class RageFileBasic;

class CsvFile
{
  public:
	CsvFile();

	bool ReadFile(const std::string& sPath);
	bool ReadFile(RageFileBasic& sFile);
	bool WriteFile(const std::string& sPath) const;
	bool WriteFile(RageFileBasic& sFile) const;

	typedef std::vector<std::string> StringVector;
	std::vector<StringVector> m_vvs;

  private:
	std::string m_sPath;
	mutable std::string m_sError;
};

#endif
