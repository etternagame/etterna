/** @brief IniFile - Reading and writing .INI files. */

#ifndef INIFILE_H
#define INIFILE_H

#include "XmlFile.h"

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
	std::string GetPath() const { return m_sPath; }
	/**
	 * @brief Retrieve any errors that have occurred.
	 * @return the latest error. */
	const std::string& GetError() const { return m_sError; }

	bool ReadFile(const std::string& sPath);
	bool ReadFile(RageFileBasic& sFile);
	bool WriteFile(const std::string& sPath) const;
	bool WriteFile(RageFileBasic& sFile) const;

	template<typename T>
	bool GetValue(const std::string& sKey,
				  const std::string& sValueName,
				  T& value) const
	{
		const XNode* pNode = GetChild(sKey);
		if (pNode == NULL)
			return false;
		return pNode->GetAttrValue<T>(sValueName, value);
	}
	template<typename T>
	void SetValue(const std::string& sKey,
				  const std::string& sValueName,
				  const T& value)
	{
		XNode* pNode = GetChild(sKey);
		if (pNode == NULL)
			pNode = AppendChild(sKey);
		pNode->AppendAttr<T>(sValueName, value);
	}
	template<typename T>
	void SetKeyValue(XNode* keynode,
					 const std::string& sValueName,
					 const T& value)
	{
		keynode->AppendAttr<T>(sValueName, value);
	}

	bool DeleteKey(const std::string& keyname);
	bool DeleteValue(const std::string& keyname, const std::string& valuename);

	/**
	 * @brief Rename a key.
	 *
	 * For example, call RenameKey("foo", "main") after reading an INI
	 * where [foo] is an alias to [main].  If to already exists,
	 * nothing happens.
	 * @param from the key to rename.
	 * @param to the new key name.
	 * @return its success or failure. */
	bool RenameKey(const std::string& from, const std::string& to);

  private:
	std::string m_sPath;

	mutable std::string m_sError;
};

#endif
