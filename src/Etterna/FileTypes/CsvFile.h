/* CsvFile - Reading and writing .CSV files. */

#ifndef CsvFile_H
#define CsvFile_H

class RageFileBasic;

class CsvFile
{
  public:
	CsvFile();

	bool ReadFile(const std::string& sPath);
	bool ReadFile(RageFileBasic& sFile);
	bool WriteFile(const std::string& sPath) const;
	bool WriteFile(RageFileBasic& sFile) const;

	typedef vector<std::string> StringVector;
	vector<StringVector> m_vvs;

  private:
	std::string m_sPath;
	mutable std::string m_sError;
};

#endif
