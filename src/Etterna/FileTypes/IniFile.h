/** @brief IniFile - Reading and writing .INI files. */

#ifndef INIFILE_H
#define INIFILE_H

#include "XmlFile.h"
using namespace std;

class RageFileBasic;
/** @brief The functions to read and write .INI files. */
class IniFile : public XNode
{
  public:
	/** @brief Set up an INI file with the defaults. */
	IniFile();

	/**
	 * @brief Retrieve the filename of the last file loaded.
	 * @return the filename. */
	RString GetPath() const { return m_sPath; }
	/**
	 * @brief Retrieve any errors that have occurred.
	 * @return the latest error. */
	const RString& GetError() const { return m_sError; }

	bool ReadFile(const RString& sPath);
	bool ReadFile(RageFileBasic& sFile);
	bool WriteFile(const RString& sPath) const;
	bool WriteFile(RageFileBasic& sFile) const;

	template<typename T>
	bool GetValue(const RString& sKey,
				  const RString& sValueName,
				  T& value) const
	{
		const XNode* pNode = GetChild(sKey);
		if (pNode == NULL)
			return false;
		return pNode->GetAttrValue<T>(sValueName, value);
	}
	template<typename T>
	void SetValue(const RString& sKey,
				  const RString& sValueName,
				  const T& value)
	{
		XNode* pNode = GetChild(sKey);
		if (pNode == NULL)
			pNode = AppendChild(sKey);
		pNode->AppendAttr<T>(sValueName, value);
	}
	template<typename T>
	void SetKeyValue(XNode* keynode, const RString& sValueName, const T& value)
	{
		keynode->AppendAttr<T>(sValueName, value);
	}

	bool DeleteKey(const RString& keyname);
	bool DeleteValue(const RString& keyname, const RString& valuename);

	/**
	 * @brief Rename a key.
	 *
	 * For example, call RenameKey("foo", "main") after reading an INI
	 * where [foo] is an alias to [main].  If to already exists,
	 * nothing happens.
	 * @param from the key to rename.
	 * @param to the new key name.
	 * @return its success or failure. */
	bool RenameKey(const RString& from, const RString& to);

  private:
	RString m_sPath;

	mutable RString m_sError;
};

#endif
