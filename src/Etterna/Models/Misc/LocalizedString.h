#ifndef LocalizedString_H
#define LocalizedString_H

class ILocalizedStringImpl
{
  public:
	virtual ~ILocalizedStringImpl() = default;
	virtual void Load(const RString& sGroup, const RString& sName) = 0;
	virtual const RString& GetLocalized() const = 0;
};
/** @brief Get a String based on the user's natural language. */
class LocalizedString
{
  public:
	LocalizedString(const RString& sGroup = "", const RString& sName = "");
	LocalizedString(LocalizedString const& other);
	~LocalizedString();
	void Load(const RString& sGroup, const RString& sName);
	operator const RString&() const { return GetValue(); }
	const RString& GetValue() const;

	using MakeLocalizer = ILocalizedStringImpl* (*)();
	static void RegisterLocalizer(MakeLocalizer pFunc);

  private:
	void CreateImpl();
	RString m_sGroup, m_sName;
	ILocalizedStringImpl* m_pImpl;
	// Swallow up warnings. If they must be used, define them.
	LocalizedString& operator=(const LocalizedString& rhs) = delete;
};

#endif
