/* CsvFile - Reading and writing .CSV files. */

#ifndef CsvFile_H
#define CsvFile_H

class RageFileBasic;

class CsvFile
{
  public:
	CsvFile();

	bool ReadFile(const RString& sPath);
	bool ReadFile(RageFileBasic& sFile);
	bool WriteFile(const RString& sPath) const;
	bool WriteFile(RageFileBasic& sFile) const;

	typedef vector<RString> StringVector;
	vector<StringVector> m_vvs;

  private:
	RString m_sPath;
	mutable RString m_sError;
};

#endif
