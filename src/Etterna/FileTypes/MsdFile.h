#ifndef MSDFILE_H
#define MSDFILE_H

/** @brief The class that reads the various .SSC, .SM, .SMA, .DWI, and .MSD
 * files. */
class MsdFile
{
  public:
	/**
	 * @brief The list of params found in the files.
	 *
	 * Note that &#35;param:param:param:param; is one whole value. */
	struct value_t
	{
		/** @brief The list of parameters. */
		vector<std::string> params;
		/** @brief Set up the parameters with default values. */
		value_t()
		  : params()
		{
		}

		/**
		 * @brief Access the proper parameter.
		 * @param i the index.
		 * @return the proper parameter.
		 */
		std::string operator[](unsigned i) const
		{
			if (i >= params.size())
				return std::string();
			return params[i];
		}
	};

	MsdFile()
	  : values()
	  , error("")
	{
	}

	/** @brief Remove the MSDFile. */
	virtual ~MsdFile() = default;

	/**
	 * @brief Attempt to read an MSD file.
	 * @param sFilePath the path to the file.
	 * @param bUnescape a flag to see if we need to unescape values.
	 * @return its success or failure.
	 */
	bool ReadFile(const std::string& sFilePath, bool bUnescape);
	/**
	 * @brief Attempt to read an MSD file.
	 * @param sString the path to the file.
	 * @param bUnescape a flag to see if we need to unescape values.
	 * @return its success or failure.
	 */
	void ReadFromString(const std::string& sString, bool bUnescape);

	/**
	 * @brief Should an error take place, have an easy place to get it.
	 * @return the current error. */
	std::string GetError() const { return error; }

	/**
	 * @brief Retrieve the number of values for each tag.
	 * @return the nmber of values. */
	unsigned GetNumValues() const { return values.size(); }
	/**
	 * @brief Get the number of parameters for the current index.
	 * @param val the current value index.
	 * @return the number of params.
	 */
	unsigned GetNumParams(unsigned val) const
	{
		if (val >= GetNumValues())
			return 0;
		return values[val].params.size();
	}
	/**
	 * @brief Get the specified value.
	 * @param val the current value index.
	 * @return The specified value.
	 */
	const value_t& GetValue(unsigned val) const
	{
		ASSERT(val < GetNumValues());
		return values[val];
	}
	/**
	 * @brief Retrieve the specified parameter.
	 * @param val the current value index.
	 * @param par the current parameter index.
	 * @return the parameter in question.
	 */
	std::string GetParam(unsigned val, unsigned par) const;

  private:
	/**
	 * @brief Attempt to read an MSD file from the buffer.
	 * @param buf the buffer containing the MSD file.
	 * @param len the length of the buffer.
	 * @param bUnescape a flag to see if we need to unescape values.
	 */
	void ReadBuf(const char* buf, int len, bool bUnescape);
	/**
	 * @brief Add a new parameter.
	 * @param buf the new parameter.
	 * @param len the length of the new parameter.
	 */
	void AddParam(const char* buf, int len);
	/**
	 * @brief Add a new value.
	 */
	void AddValue();

	/** @brief The list of values. */
	vector<value_t> values;
	/** @brief The error string. */
	std::string error;
};

#endif
