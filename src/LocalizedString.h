#ifndef LocalizedString_H
#define LocalizedString_H

class ILocalizedStringImpl
{
public:
	virtual ~ILocalizedStringImpl() = default;
	virtual void Load( const RString& sGroup, const RString& sName ) = 0;
	virtual const RString &GetLocalized() const = 0;
};
/** @brief Get a String based on the user's natural language. */
class LocalizedString
{
public:
	LocalizedString( const RString& sGroup = "", const RString& sName = "" );
	LocalizedString(LocalizedString const& other);
	~LocalizedString();
	void Load( const RString& sGroup, const RString& sName );
	operator const RString &() const { return GetValue(); }
	const RString &GetValue() const;

	using MakeLocalizer = ILocalizedStringImpl *(*)();
	static void RegisterLocalizer( MakeLocalizer pFunc );

private:
	void CreateImpl();
	RString m_sGroup, m_sName;
	ILocalizedStringImpl *m_pImpl;
	// Swallow up warnings. If they must be used, define them.
	LocalizedString& operator=(const LocalizedString& rhs) = delete;
};

#endif

/**
 * @file
 * @author Chris Danford, Glenn Maynard (c) 2001-2005
 * @section LICENSE
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
