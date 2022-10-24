#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/File/RageFile.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Sound/RageSoundReader_FileReader.h"
#include "RageUtil.h"
#include "RageUtil/Misc/RageUnicode.h"

#include <algorithm>
#include <ctime>
#include <map>
#include <numeric>
#include <sstream>
#include <cassert>
#include <cctype>

#ifdef _WIN32
#include <Windows.h>
#endif

using std::max;
using std::min;
using std::vector;
using std::wstring;

std::string
head(std::string const& source, int32_t const length)
{
	if (static_cast<size_t>(std::abs(length)) >= source.size()) {
		return source;
	}
	if (length < 0) {
		return source.substr(0, source.size() + length);
	}
	return source.substr(0, length);
}

std::string
tail(std::string const& source, int32_t const length)
{
	if (static_cast<size_t>(std::abs(length)) >= source.size()) {
		return source;
	}
	if (length < 0) {
		return source.substr(-length);
	}
	return source.substr(source.size() - length);
}

bool
starts_with(std::string const& source, std::string const& target)
{
	return source.length() >= target.length() &&
		   head(source, target.size()) == target;
}

bool
ends_with(std::string const& source, std::string const& target)
{
	return source.length() >= target.length() &&
		   tail(source, target.size()) == target;
}

void
s_replace(std::string& target, std::string const& from, std::string const& to)
{
	std::string newString;
	newString.reserve(target.length()); // avoids a few memory allocations

	std::string::size_type lastPos = 0;
	std::string::size_type findPos;

	while (std::string::npos != (findPos = target.find(from, lastPos))) {
		newString.append(target, lastPos, findPos - lastPos);
		newString += to;
		lastPos = findPos + from.length();
	}

	// Care for the rest after last occurrence
	newString += target.substr(lastPos);

	target.swap(newString);
}

int
unicode_do_casing(char* p, size_t iLen, const unsigned char pMapping[256])
{
	// Note: this has problems with certain accented characters. -aj
	auto wc = L'\0';
	unsigned iStart = 0;
	if (!Rage::utf8_to_wchar(p, iLen, iStart, wc))
		return 1;

	auto iUpper = wc;
	if (wc < 256)
		iUpper = pMapping[wc];
	if (iUpper != wc) {
		std::string sOut;
		Rage::wchar_to_utf8(iUpper, sOut);
		if (sOut.size() == iStart) {
			strcpy(p, sOut.data());
		} else {
			// TODO: Find another
			// WARN( fmt::sprintf("UnicodeDoUpper: invalid character at \"%s\"",
			// std::string(p,iLen).c_str()) );
		}
	}

	return iStart;
}

void
make_upper(char* p, size_t len)
{
	const auto start = p;
	const auto end = p + len;
	while (p < end) {
		// Fast path:
		if (likely(!(*p & 0x80))) {
			if (unlikely(*p >= 'a' && *p <= 'z'))
				*p += 'A' - 'a';
			++p;
			continue;
		}

		const int iRemaining = len - (p - start);
		p += unicode_do_casing(p, iRemaining, Rage::upperCase);
	}
}

std::string
make_upper(std::string const& source)
{
	std::vector<char> buffer{ source.begin(), source.end() };

	// Ensure a null terminating character is in place just in case.
	buffer.push_back(0);

	::make_upper(&buffer[0], source.size());

	return std::string{ buffer.begin(), buffer.end() - 1 };
}

inline void
make_lower(char* p, size_t len)
{
	const auto start = p;
	const auto end = p + len;
	while (p < end) {
		// Fast path:
		if (likely(!(*p & 0x80))) {
			if (unlikely(*p >= 'A' && *p <= 'Z'))
				*p -= 'A' - 'a';
			++p;
			continue;
		}

		const int iRemaining = len - (p - start);
		p += unicode_do_casing(p, iRemaining, Rage::lowerCase);
	}
}

std::string
make_lower(std::string const& source)
{
	std::vector<char> buffer{ source.begin(), source.end() };

	// Ensure a null terminating character is in place just in case.
	buffer.push_back(0);

	make_lower(&buffer[0], source.size());

	return std::string{ buffer.begin(), buffer.end() - 1 };
}

bool
HexToBinary(const std::string&, std::string&);
void
UnicodeUpperLower(wchar_t*, size_t, const unsigned char*);

void
fapproach(float& val, float other_val, float to_move)
{
	assert(to_move >= 0);
	if (val == other_val)
		return;
	const auto fDelta = other_val - val;
	const auto fSign = fDelta / fabsf(fDelta);
	auto fToMove = fSign * to_move;
	if (fabsf(fToMove) > fabsf(fDelta))
		fToMove = fDelta; // snap
	val += fToMove;
}

bool
IsHexVal(const std::string& s)
{
	if (s.empty())
		return false;

	for (auto i : s)
		if (!(i >= '0' && i <= '9') &&
			!(toupper(i) >= 'A' && toupper(i) <= 'F'))
			return false;

	return true;
}

std::string
BinaryToHex(const void* pData_, int iNumBytes)
{
	const auto* pData = static_cast<const unsigned char*>(pData_);
	std::string s;
	for (auto i = 0; i < iNumBytes; i++) {
		const unsigned val = pData[i];
		s += ssprintf("%02x", val);
	}
	return s;
}

std::string
BinaryToHex(const std::string& sString)
{
	return BinaryToHex(sString.data(), sString.size());
}

float
HHMMSSToSeconds(const std::string& sHHMMSS)
{
	std::vector<std::string> arrayBits;
	split(sHHMMSS, ":", arrayBits, false);

	while (arrayBits.size() < 3)
		arrayBits.insert(arrayBits.begin(), "0"); // pad missing bits

	float fSeconds = 0;
	fSeconds += StringToInt(arrayBits[0]) * 60 * 60;
	fSeconds += StringToInt(arrayBits[1]) * 60;
	fSeconds += StringToFloat(arrayBits[2]);

	return fSeconds;
}

std::string
SecondsToHHMMSS(float fSecs)
{
	const auto iMinsDisplay = static_cast<int>(fSecs) / 60;
	const auto iSecsDisplay = static_cast<int>(fSecs) - iMinsDisplay * 60;
	auto sReturn = ssprintf(
	  "%02d:%02d:%02d", iMinsDisplay / 60, iMinsDisplay % 60, iSecsDisplay);
	return sReturn;
}

std::string
SecondsToMMSSMsMs(float fSecs)
{
	const auto iMinsDisplay = static_cast<int>(fSecs) / 60;
	const auto iSecsDisplay = static_cast<int>(fSecs) - iMinsDisplay * 60;
	const auto iLeftoverDisplay =
	  static_cast<int>((fSecs - iMinsDisplay * 60 - iSecsDisplay) * 100);
	auto sReturn = ssprintf(
	  "%02d:%02d.%02d", iMinsDisplay, iSecsDisplay, min(99, iLeftoverDisplay));
	return sReturn;
}

std::string
SecondsToMSSMsMs(float fSecs)
{
	const auto iMinsDisplay = static_cast<int>(fSecs) / 60;
	const auto iSecsDisplay = static_cast<int>(fSecs) - iMinsDisplay * 60;
	const auto iLeftoverDisplay =
	  static_cast<int>((fSecs - iMinsDisplay * 60 - iSecsDisplay) * 100);
	auto sReturn = ssprintf(
	  "%01d:%02d.%02d", iMinsDisplay, iSecsDisplay, min(99, iLeftoverDisplay));
	return sReturn;
}

std::string
SecondsToMMSSMsMsMs(float fSecs)
{
	const auto iMinsDisplay = static_cast<int>(fSecs) / 60;
	const auto iSecsDisplay = static_cast<int>(fSecs) - iMinsDisplay * 60;
	const auto iLeftoverDisplay =
	  static_cast<int>((fSecs - iMinsDisplay * 60 - iSecsDisplay) * 1000);
	auto sReturn = ssprintf(
	  "%02d:%02d.%03d", iMinsDisplay, iSecsDisplay, min(999, iLeftoverDisplay));
	return sReturn;
}

std::string
SecondsToMSS(float fSecs)
{
	const auto iMinsDisplay = static_cast<int>(fSecs) / 60;
	const auto iSecsDisplay = static_cast<int>(fSecs) - iMinsDisplay * 60;
	auto sReturn = ssprintf("%01d:%02d", iMinsDisplay, iSecsDisplay);
	return sReturn;
}

std::string
SecondsToMMSS(float fSecs)
{
	const auto iMinsDisplay = static_cast<int>(fSecs) / 60;
	const auto iSecsDisplay = static_cast<int>(fSecs) - iMinsDisplay * 60;
	auto sReturn = ssprintf("%02d:%02d", iMinsDisplay, iSecsDisplay);
	return sReturn;
}

std::string
PrettyPercent(float fNumerator, float fDenominator)
{
	return ssprintf("%0.2f%%", fNumerator / fDenominator * 100);
}

std::string
Commify(const std::string& num, const std::string& sep, const std::string& dot)
{
	size_t num_start = 0;
	auto num_end = num.size();
	const auto dot_pos = num.find(dot);
	const auto dash_pos = num.find('-');
	if (dot_pos != std::string::npos) {
		num_end = dot_pos;
	}
	if (dash_pos != std::string::npos) {
		num_start = dash_pos + 1;
	}
	const auto num_size = num_end - num_start;
	const auto commies =
	  num_size / 3 - static_cast<unsigned long long>(num_size % 3 == 0u);
	if (commies < 1) {
		return num;
	}
	const auto commified_len = num.size() + commies * sep.size();
	std::string ret;
	ret.resize(commified_len);
	size_t dest = 0;
	auto next_comma =
	  num_size % 3 + 3 * static_cast<int>(num_size % 3 == 0u) + num_start;
	for (size_t c = 0; c < num.size(); ++c) {
		if (c == next_comma && c < num_end) {
			for (auto s : sep) {
				ret[dest] = s;
				++dest;
			}
			next_comma += 3;
		}
		ret[dest] = num[c];
		++dest;
	}
	return ret;
}

static LocalizedString NUM_PREFIX("RageUtil", "NumPrefix");
static LocalizedString NUM_ST("RageUtil", "NumSt");
static LocalizedString NUM_ND("RageUtil", "NumNd");
static LocalizedString NUM_RD("RageUtil", "NumRd");
static LocalizedString NUM_TH("RageUtil", "NumTh");
std::string
FormatNumberAndSuffix(int i)
{
	std::string sSuffix;
	switch (i % 10) {
		case 1:
			sSuffix = NUM_ST;
			break;
		case 2:
			sSuffix = NUM_ND;
			break;
		case 3:
			sSuffix = NUM_RD;
			break;
		default:
			sSuffix = NUM_TH;
			break;
	}

	// "11th", "113th", etc.
	if (i % 100 / 10 == 1)
		sSuffix = NUM_TH;

	return NUM_PREFIX.GetValue() + ssprintf("%i", i) + sSuffix;
}

struct tm
GetLocalTime()
{
	const auto t = time(nullptr);
	struct tm tm
	{
	};
	localtime_r(&t, &tm);
	return tm;
}

#define FMT_BLOCK_SIZE 2048 // # of bytes to increment per try

#ifdef _WIN32
int
FillCharBuffer(char** eBuf, const char* szFormat, va_list argList)
{
	char* pBuf = nullptr;
	auto iChars = 1;
	auto iUsed = 0;
	auto iTry = 0;

	do {
		// must free the buffer: _malloca allocates on the heap OR the stack
		// depending on the space needed
		_freea(pBuf);
		// Grow more than linearly (e.g. 512, 1536, 3072, etc)
		iChars += iTry * FMT_BLOCK_SIZE;
		__try {
			pBuf = static_cast<char*>(_malloca(sizeof(char) * iChars));
		} __except (GetExceptionCode() == STATUS_STACK_OVERFLOW) {
			if (_resetstkoflw())
				sm_crash("Unrecoverable Stack Overflow");
		}
		iUsed = _vsnprintf(pBuf, iChars - 1, szFormat, argList);
		++iTry;
	} while (iUsed < 0);

	*eBuf = pBuf;
	return iUsed;
}
#endif

std::string
vssprintf(const char* szFormat, va_list argList)
{
	std::string sStr;

#ifdef _WIN32
	char* pBuf = nullptr;
	const auto iUsed = FillCharBuffer(&pBuf, szFormat, argList);

	// assign whatever we managed to format
	sStr.assign(pBuf, iUsed);
	// free the buffer one last time
	_freea(pBuf);
#else
	static bool bExactSizeSupported;
	static bool bInitialized = false;
	if (!bInitialized) {
		/* Some systems return the actual size required when snprintf
		 * doesn't have enough space.  This lets us avoid wasting time
		 * iterating, and wasting memory. */
		char ignore;
		bExactSizeSupported = (snprintf(&ignore, 0, "Hello World") == 11);
		bInitialized = true;
	}

	if (bExactSizeSupported) {
		va_list tmp;
		va_copy(tmp, argList);
		char ignore;
		int iNeeded = vsnprintf(&ignore, 0, szFormat, tmp);
		va_end(tmp);

		auto* buf = new char[iNeeded + 1];
		std::fill(buf, buf + iNeeded + 1, '\0');
		vsnprintf(buf, iNeeded + 1, szFormat, argList);
		std::string ret(buf);
		delete[] buf;
		return ret;
	}

	int iChars = FMT_BLOCK_SIZE;
	int iTry = 1;
	for (;;) {
		// Grow more than linearly (e.g. 512, 1536, 3072, etc)
		auto* buf = new char[iChars];
		std::fill(buf, buf + iChars, '\0');
		int used = vsnprintf(buf, iChars - 1, szFormat, argList);
		if (used == -1) {
			iChars += (++iTry * FMT_BLOCK_SIZE);
		} else {
			/* OK */
			sStr.assign(buf, used);
		}

		delete[] buf;
		if (used != -1) {
			break;
		}
	}
#endif
	return sStr;
}

/* Windows uses %I64i to format a 64-bit int, instead of %lli. Convert "a b %lli
 * %-3llu c d" to "a b %I64 %-3I64u c d". This assumes a well-formed format
 * string; invalid format strings should not crash, but the results are
 * undefined. */
#ifdef _WIN32
std::string
ConvertI64FormatString(const std::string& sStr)
{
	std::string sRet;
	sRet.reserve(sStr.size() + 16);

	size_t iOffset = 0;
	while (iOffset < sStr.size()) {
		const auto iPercent = sStr.find('%', iOffset);
		if (iPercent != std::string::npos) {
			sRet.append(sStr, iOffset, iPercent - iOffset);
			iOffset = iPercent;
		}

		auto iEnd = sStr.find_first_of("diouxXeEfFgGaAcsCSpnm%", iOffset + 1);
		if (iEnd != std::string::npos && iEnd - iPercent >= 3 && iPercent > 2 &&
			sStr[iEnd - 2] == 'l' && sStr[iEnd - 1] == 'l') {
			sRet.append(sStr, iPercent, iEnd - iPercent - 2); // %
			sRet.append("I64");								  // %I64
			sRet.append(sStr, iEnd, 1);						  // %I64i
			iOffset = iEnd + 1;
		} else {
			if (iEnd == std::string::npos)
				iEnd = sStr.size() - 1;
			sRet.append(sStr, iOffset, iEnd - iOffset + 1);
			iOffset = iEnd + 1;
		}
	}
	return sRet;
}
#else
std::string
ConvertI64FormatString(const std::string& sStr)
{
	return sStr;
}
#endif

/* ISO-639-1 codes: http://www.loc.gov/standards/iso639-2/php/code_list.php
 * native forms: http://people.w3.org/rishida/names/languages.html
 * We don't use 3-letter codes, so we don't bother supporting them. */
static const LanguageInfo g_langs[] = {
	{ "aa", "Afar" },		 { "ab", "Abkhazian" },
	{ "af", "Afrikaans" },	 { "am", "Amharic" },
	{ "ar", "Arabic" },		 { "as", "Assamese" },
	{ "ay", "Aymara" },		 { "az", "Azerbaijani" },
	{ "ba", "Bashkir" },	 { "be", "Byelorussian" },
	{ "bg", "Bulgarian" },	 { "bh", "Bihari" },
	{ "bi", "Bislama" },	 { "bn", "Bengali" },
	{ "bo", "Tibetan" },	 { "br", "Breton" },
	{ "ca", "Catalan" },	 { "co", "Corsican" },
	{ "cs", "Czech" },		 { "cy", "Welsh" },
	{ "da", "Danish" },		 { "de", "German" },
	{ "dz", "Bhutani" },	 { "el", "Greek" },
	{ "en", "English" },	 { "eo", "Esperanto" },
	{ "es", "Spanish" },	 { "et", "Estonian" },
	{ "eu", "Basque" },		 { "fa", "Persian" },
	{ "fi", "Finnish" },	 { "fj", "Fiji" },
	{ "fo", "Faeroese" },	 { "fr", "French" },
	{ "fy", "Frisian" },	 { "ga", "Irish" },
	{ "gd", "Gaelic" },		 { "gl", "Galician" },
	{ "gn", "Guarani" },	 { "gu", "Gujarati" },
	{ "ha", "Hausa" },		 { "he", "Hebrew" },
	{ "hi", "Hindi" },		 { "hr", "Croatian" },
	{ "hu", "Hungarian" },	 { "hy", "Armenian" },
	{ "ia", "Interlingua" }, { "id", "Indonesian" },
	{ "ie", "Interlingue" }, { "ik", "Inupiak" },
	{ "in", "Indonesian" }, // compatibility
	{ "is", "Icelandic" },	 { "it", "Italian" },
	{ "iw", "Hebrew" },							  // compatibility
	{ "ja", "Japanese" },	 { "ji", "Yiddish" }, // compatibility
	{ "jw", "Javanese" },	 { "ka", "Georgian" },
	{ "kk", "Kazakh" },		 { "kl", "Greenlandic" },
	{ "km", "Cambodian" },	 { "kn", "Kannada" },
	{ "ko", "Korean" },		 { "ks", "Kashmiri" },
	{ "ku", "Kurdish" },	 { "ky", "Kirghiz" },
	{ "la", "Latin" },		 { "ln", "Lingala" },
	{ "lo", "Laothian" },	 { "lt", "Lithuanian" },
	{ "lv", "Latvian" },	 { "mg", "Malagasy" },
	{ "mi", "Maori" },		 { "mk", "Macedonian" },
	{ "ml", "Malayalam" },	 { "mn", "Mongolian" },
	{ "mo", "Moldavian" },	 { "mr", "Marathi" },
	{ "ms", "Malay" },		 { "mt", "Maltese" },
	{ "my", "Burmese" },	 { "na", "Nauru" },
	{ "ne", "Nepali" },		 { "nl", "Dutch" },
	{ "no", "Norwegian" },	 { "oc", "Occitan" },
	{ "om", "Oromo" },		 { "or", "Oriya" },
	{ "pa", "Punjabi" },	 { "pl", "Polish" },
	{ "ps", "Pashto" },		 { "pt", "Portuguese" },
	{ "qu", "Quechua" },	 { "rm", "Rhaeto-Romance" },
	{ "rn", "Kirundi" },	 { "ro", "Romanian" },
	{ "ru", "Russian" },	 { "rw", "Kinyarwanda" },
	{ "sa", "Sanskrit" },	 { "sd", "Sindhi" },
	{ "sg", "Sangro" },		 { "sh", "Serbo-Croatian" },
	{ "si", "Singhalese" },	 { "sk", "Slovak" },
	{ "sl", "Slovenian" },	 { "sm", "Samoan" },
	{ "sn", "Shona" },		 { "so", "Somali" },
	{ "sq", "Albanian" },	 { "sr", "Serbian" },
	{ "ss", "Siswati" },	 { "st", "Sesotho" },
	{ "su", "Sudanese" },	 { "sv", "Swedish" },
	{ "sw", "Swahili" },	 { "ta", "Tamil" },
	{ "te", "Tegulu" },		 { "tg", "Tajik" },
	{ "th", "Thai" },		 { "ti", "Tigrinya" },
	{ "tk", "Turkmen" },	 { "tl", "Tagalog" },
	{ "tn", "Setswana" },	 { "to", "Tonga" },
	{ "tr", "Turkish" },	 { "ts", "Tsonga" },
	{ "tt", "Tatar" },		 { "tw", "Twi" },
	{ "uk", "Ukrainian" },	 { "ur", "Urdu" },
	{ "uz", "Uzbek" },		 { "vi", "Vietnamese" },
	{ "vo", "Volapuk" },	 { "wo", "Wolof" },
	{ "xh", "Xhosa" },		 { "yi", "Yiddish" },
	{ "yo", "Yoruba" },		 { "zh", "Chinese" },
	{ "zu", "Zulu" },
};

void
GetLanguageInfos(std::vector<const LanguageInfo*>& vAddTo)
{
	for (const auto& g_lang : g_langs)
		vAddTo.push_back(&g_lang);
}

const LanguageInfo*
GetLanguageInfo(const std::string& sIsoCode)
{
	for (const auto& g_lang : g_langs) {
		if (EqualsNoCase(sIsoCode, g_lang.szIsoCode))
			return &g_lang;
	}

	return nullptr;
}

std::string
join(const std::string& sDeliminator, const std::vector<std::string>& sSource)
{
	if (sSource.empty())
		return std::string();

	std::string sTmp;
	size_t final_size = 0;
	const auto delim_size = sDeliminator.size();
	for (size_t n = 0; n < sSource.size() - 1; ++n) {
		final_size += sSource[n].size() + delim_size;
	}
	final_size += sSource.back().size();
	sTmp.reserve(final_size);

	for (unsigned iNum = 0; iNum < sSource.size() - 1; iNum++) {
		sTmp += sSource[iNum];
		sTmp += sDeliminator;
	}
	sTmp += sSource.back();
	return sTmp;
}

std::string
join(const std::string& sDelimitor,
	 std::vector<std::string>::const_iterator begin,
	 std::vector<std::string>::const_iterator end)
{
	if (begin == end)
		return std::string();

	std::string sRet;
	size_t final_size = 0;
	const auto delim_size = sDelimitor.size();
	for (auto curr = begin; curr != end; ++curr) {
		final_size += curr->size();
		if (curr != end) {
			final_size += delim_size;
		}
	}
	sRet.reserve(final_size);

	while (begin != end) {
		sRet += *begin;
		++begin;
		if (begin != end)
			sRet += sDelimitor;
	}

	return sRet;
}

std::string
luajoin(const std::string& sDeliminator, const std::vector<std::string>& sSource)
{
	if (sSource.empty())
		return std::string();

	std::string sTmp;
	size_t final_size = 0;
	const auto delim_size = sDeliminator.size();
	for (size_t n = 0; n < sSource.size() - 1; ++n) {
		final_size += sSource[n].size() + delim_size;
	}
	final_size += sSource.back().size();
	sTmp.reserve(final_size);

	for (unsigned iNum = 0; iNum < sSource.size() - 1; iNum++) {
		sTmp += sSource[iNum];
		sTmp += sDeliminator;
	}
	sTmp += sSource.back();
	return sTmp;
}

std::string
luajoin(const std::string& sDelimitor,
		std::vector<std::string>::const_iterator begin,
		std::vector<std::string>::const_iterator end)
{
	if (begin == end)
		return std::string();

	std::string sRet;
	size_t final_size = 0;
	const auto delim_size = sDelimitor.size();
	for (auto curr = begin; curr != end; ++curr) {
		final_size += curr->size();
		if (curr != end) {
			final_size += delim_size;
		}
	}
	sRet.reserve(final_size);

	while (begin != end) {
		sRet += *begin;
		++begin;
		if (begin != end)
			sRet += sDelimitor;
	}

	return sRet;
}

std::string
SmEscape(const std::string& sUnescaped)
{
	return SmEscape(sUnescaped.c_str(), sUnescaped.size());
}

std::string
SmEscape(const char* cUnescaped, int len)
{
	std::string answer;
	for (auto i = 0; i < len; ++i) {
		// Other characters we could theoretically escape:
		// NotesWriterSM.cpp used to claim ',' should be escaped, but there was
		// no explanation why
		// '#' is both a control character and a valid part of a parameter.  The
		// only way for there to be
		//   any confusion is in a misformatted .sm file, though, so it is
		//   unnecessary to escape it.
		if (cUnescaped[i] == '/' && i + 1 < len && cUnescaped[i + 1] == '/') {
			answer += "\\/\\/";
			++i; // increment here so we skip both //s
			continue;
		}
		if (cUnescaped[i] == '\\' || cUnescaped[i] == ':' ||
			cUnescaped[i] == ';')
			answer += "\\";
		answer += cUnescaped[i];
	}
	return answer;
}

std::string
DwiEscape(const std::string& sUnescaped)
{
	return DwiEscape(sUnescaped.c_str(), sUnescaped.size());
}

std::string
DwiEscape(const char* cUnescaped, int len)
{
	std::string answer;
	for (auto i = 0; i < len; ++i) {
		switch (cUnescaped[i]) {
			// TODO: Which of these characters actually affect DWI?
			case '\\':
			case ':':
			case ';':
				answer += '|';
				break;
			case '[':
				answer += '(';
				break;
			case ']':
				answer += ')';
				break;
			default:
				answer += cUnescaped[i];
		}
	}
	return answer;
}

template<class S>
static int
DelimitorLength(const S& Delimitor)
{
	return Delimitor.size();
}

static int
DelimitorLength(char Delimitor)
{
	return 1;
}

static int
DelimitorLength(wchar_t Delimitor)
{
	return 1;
}

template<class S, class C>
void
do_split(const S& Source,
		 const C Delimitor,
		 std::vector<S>& AddIt,
		 const bool bIgnoreEmpty)
{
	/* Short-circuit if the source is empty; we want to return an empty vector
	 * if the string is empty, even if bIgnoreEmpty is true. */
	if (Source.empty())
		return;

	size_t startpos = 0;

	do {
		size_t pos;
		pos = Source.find(Delimitor, startpos);
		if (pos == Source.npos)
			pos = Source.size();

		if (pos - startpos > 0 || !bIgnoreEmpty) {
			/* Optimization: if we're copying the whole string, avoid substr;
			 * this allows this copy to be refcounted, which is much faster. */
			if (startpos == 0 && pos - startpos == Source.size())
				AddIt.push_back(Source);
			else {
				const S AddString = Source.substr(startpos, pos - startpos);
				AddIt.push_back(AddString);
			}
		}

		startpos = pos + DelimitorLength(Delimitor);
	} while (startpos <= Source.size());
}

void
split(const std::string& sSource,
	  const std::string& sDelimitor,
	  std::vector<std::string>& asAddIt,
	  const bool bIgnoreEmpty)
{
	if (sDelimitor.size() == 1)
		do_split(sSource, sDelimitor[0], asAddIt, bIgnoreEmpty);
	else
		do_split(sSource, sDelimitor, asAddIt, bIgnoreEmpty);
}

void
split(const wstring& sSource,
	  const wstring& sDelimitor,
	  std::vector<wstring>& asAddIt,
	  const bool bIgnoreEmpty)
{
	if (sDelimitor.size() == 1)
		do_split(sSource, sDelimitor[0], asAddIt, bIgnoreEmpty);
	else
		do_split(sSource, sDelimitor, asAddIt, bIgnoreEmpty);
}

template<class S>
void
do_split(const S& Source,
		 const S& Delimitor,
		 int& begin,
		 int& size,
		 int len,
		 const bool bIgnoreEmpty)
{
	if (size != -1) {
		// Start points to the beginning of the last delimiter. Move it up.
		begin += size + Delimitor.size();
		begin = min(begin, len);
	}

	size = 0;

	if (bIgnoreEmpty) {
		// Skip delims.
		while (begin + Delimitor.size() < Source.size() &&
			   !Source.compare(begin, Delimitor.size(), Delimitor))
			++begin;
	}

	/* Where's the string function to find within a substring?
	 * C++ strings apparently are missing that ... */
	size_t pos;
	if (Delimitor.size() == 1)
		pos = Source.find(Delimitor[0], begin);
	else
		pos = Source.find(Delimitor, begin);
	if (pos == Source.npos || static_cast<int>(pos) > len)
		pos = len;
	size = pos - begin;
}

void
split(const std::string& Source,
	  const std::string& Delimitor,
	  int& begin,
	  int& size,
	  int len,
	  const bool bIgnoreEmpty)
{
	do_split(Source, Delimitor, begin, size, len, bIgnoreEmpty);
}

void
split(const wstring& Source,
	  const wstring& Delimitor,
	  int& begin,
	  int& size,
	  int len,
	  const bool bIgnoreEmpty)
{
	do_split(Source, Delimitor, begin, size, len, bIgnoreEmpty);
}

void
split(const std::string& Source,
	  const std::string& Delimitor,
	  int& begin,
	  int& size,
	  const bool bIgnoreEmpty)
{
	do_split(Source, Delimitor, begin, size, Source.size(), bIgnoreEmpty);
}

void
split(const wstring& Source,
	  const wstring& Delimitor,
	  int& begin,
	  int& size,
	  const bool bIgnoreEmpty)
{
	do_split(Source, Delimitor, begin, size, Source.size(), bIgnoreEmpty);
}

/*
 * foo\fum\          -> "foo\fum\", "", ""
 * c:\foo\bar.txt    -> "c:\foo\", "bar", ".txt"
 * \\foo\fum         -> "\\foo\", "fum", ""
 */
void
splitpath(const std::string& sPath,
		  std::string& sDir,
		  std::string& sFilename,
		  std::string& sExt)
{
	sDir = sFilename = sExt = "";

	std::vector<std::string> asMatches;

	/*
	 * One level of escapes for the regex, one for C. Ew.
	 * This is really:
	 * ^(.*[\\/])?(.*)$
	 */
	static Regex sep("^(.*[\\\\/])?(.*)$");
	const auto bCheck = sep.Compare(sPath, asMatches);
	ASSERT(bCheck);

	sDir = asMatches[0];
	const auto sBase = asMatches[1];

	/* ^(.*)(\.[^\.]+)$ */
	static Regex SplitExt("^(.*)(\\.[^\\.]+)$");
	if (SplitExt.Compare(sBase, asMatches)) {
		sFilename = asMatches[0];
		sExt = asMatches[1];
	} else {
		sFilename = sBase;
	}
}

/* "foo.bar", "baz" -> "foo.baz"
 * "foo", "baz" -> "foo.baz"
 * "foo.bar", "" -> "foo" */
std::string
SetExtension(const std::string& sPath, const std::string& sExt)
{
	std::string sDir, sFileName, sOldExt;
	splitpath(sPath, sDir, sFileName, sOldExt);
	return sDir + sFileName + (!sExt.empty() ? "." : "") + sExt;
}

std::string
GetExtension(const std::string& sPath)
{
	const auto pos = sPath.rfind('.');
	if (pos == std::string::npos)
		return std::string();

	const auto slash = sPath.find('/', pos);
	if (slash != std::string::npos)
		return std::string(); /* rare: path/dir.ext/fn */

	return sPath.substr(pos + 1, sPath.size() - pos + 1);
}

std::string
GetFileNameWithoutExtension(const std::string& sPath)
{
	std::string sThrowAway, sFName;
	splitpath(sPath, sThrowAway, sFName, sThrowAway);
	return sFName;
}

void
MakeValidFilename(std::string& sName)
{
	auto wsName = StringToWString(sName);
	const wstring wsInvalid = L"/\\:*?\"<>|";
	for (auto& i : wsName) {
		const auto w = i;
		if (w >= 32 && w < 126 &&
			wsInvalid.find_first_of(w) == std::wstring::npos)
			continue;

		if (w == L'"') {
			i = L'\'';
			continue;
		}

		/* We could replace with closest matches in ASCII: convert the character
		 * to UTF-8 NFD (decomposed) (maybe NFKD?), and see if the first
		 * character is ASCII. This is useless for non-Western languages,
		 * since we'll replace the whole filename. */
		i = '_';
	}

	sName = WStringToString(wsName);
}

bool
FindFirstFilenameContaining(const std::vector<std::string>& filenames,
							std::string& out,
							const std::vector<std::string>& starts_with,
							const std::vector<std::string>& contains,
							const std::vector<std::string>& ends_with)
{
	for (const auto& filename : filenames) {
		auto lower = make_lower(GetFileNameWithoutExtension(filename));

		for (const auto& s : starts_with) {
			if (!lower.compare(0, s.size(), s)) {
				out = filename;
				return true;
			}
		}
		const auto lower_size = lower.size();
		for (const auto& s : ends_with) {
			if (lower_size >= s.size()) {
				const auto end_pos = lower_size - s.size();
				if (!lower.compare(end_pos, std::string::npos, s)) {
					out = filename;
					return true;
				}
			}
		}
		for (const auto& contain : contains) {
			if (lower.find(contain) != std::string::npos) {
				out = filename;
				return true;
			}
		}
	}
	return false;
}

int g_argc = 0;
char** g_argv = nullptr;

void
SetCommandlineArguments(int argc, char** argv)
{
	g_argc = argc;
	g_argv = argv;
}

void
GetCommandLineArguments(int& argc, char**& argv)
{
	argc = g_argc;
	argv = g_argv;
}

/* Search for the commandline argument given; eg. "test" searches for the
 * option "--test".  All commandline arguments are getopt_long style: --foo;
 * short arguments (-x) are not supported.  (These are not intended for
 * common, general use, so having short options isn't currently needed.)
 * If argument is non-NULL, accept an argument. */
bool
GetCommandlineArgument(const std::string& option,
					   std::string* argument,
					   int iIndex)
{
	const auto optstr = "--" + option;

	for (auto arg = 1; arg < g_argc; ++arg) {
		const std::string CurArgument = g_argv[arg];

		const auto i = CurArgument.find('=');
		auto CurOption = CurArgument.substr(0, i);
		if (!EqualsNoCase(CurOption, optstr))
			continue; // no match

		// Found it.
		if (iIndex != 0) {
			--iIndex;
			continue;
		}

		if (argument != nullptr) {
			if (i != std::string::npos)
				*argument = CurArgument.substr(i + 1);
			else
				*argument = "";
		}

		return true;
	}

	return false;
}

std::string
GetCwd()
{
	char buf[PATH_MAX];
	const auto ret = getcwd(buf, PATH_MAX) != nullptr;
	ASSERT(ret);
	return buf;
}

/*
 * Calculate a standard CRC32.  iCRC should be initialized to 0.
 * References:
 *   http://www.theorem.com/java/CRC32.java,
 *   http://www.faqs.org/rfcs/rfc1952.html
 */
void
CRC32(unsigned int& iCRC, const void* pVoidBuffer, size_t iSize)
{
	static unsigned tab[256];
	static auto initted = false;
	if (!initted) {
		initted = true;
		const auto POLY = 0xEDB88320;

		for (auto i = 0; i < 256; ++i) {
			tab[i] = i;
			for (auto j = 0; j < 8; ++j) {
				if ((tab[i] & 1) != 0u)
					tab[i] = tab[i] >> 1 ^ POLY;
				else
					tab[i] >>= 1;
			}
		}
	}

	iCRC ^= 0xFFFFFFFF;

	const auto* pBuffer = static_cast<const char*>(pVoidBuffer);
	for (unsigned i = 0; i < iSize; ++i)
		iCRC = iCRC >> 8 ^ tab[(iCRC ^ pBuffer[i]) & 0xFF];

	iCRC ^= 0xFFFFFFFF;
}

unsigned int
GetHashForString(const std::string& s)
{
	unsigned crc = 0;
	CRC32(crc, s.data(), s.size());
	return crc;
}

/* Return true if "dir" is empty or does not exist. */
bool
DirectoryIsEmpty(const std::string& sDir)
{
	if (sDir.empty())
		return true;
	if (!DoesFileExist(sDir))
		return true;

	std::vector<std::string> asFileNames;
	GetDirListing(sDir, asFileNames);
	return asFileNames.empty();
}

bool
CompareStringsAsc(const std::string& a, const std::string& b)
{
	return CompareNoCase(a, b) > 0;
}

bool
CompareStringsDesc(const std::string& a, const std::string& b)
{
	return CompareNoCase(b, a) > 0;
}

void
SortStringArray(std::vector<std::string>& arrayStrings, const bool bSortAscending)
{
	sort(arrayStrings.begin(),
		 arrayStrings.end(),
		 bSortAscending ? CompareStringsAsc : CompareStringsDesc);
}

float
calc_mean(const float* pStart, const float* pEnd)
{
	return std::accumulate(pStart, pEnd, 0.f) / std::distance(pStart, pEnd);
}

float
calc_stddev(const float* pStart, const float* pEnd, bool bSample)
{
	/* Calculate the mean. */
	const auto fMean = calc_mean(pStart, pEnd);

	/* Calculate stddev. */
	auto fDev = 0.0f;
	for (const auto* i = pStart; i != pEnd; ++i)
		fDev += (*i - fMean) * (*i - fMean);
	fDev /= std::distance(pStart, pEnd) - (bSample ? 1 : 0);
	fDev = sqrtf(fDev);

	return fDev;
}

void
TrimLeft(std::string& sStr, const char* s)
{
	auto n = 0;
	while (n < static_cast<int>(sStr.size()) && strchr(s, sStr[n]))
		n++;

	sStr.erase(sStr.begin(), sStr.begin() + n);
}

void
TrimRight(std::string& sStr, const char* s)
{
	int n = sStr.size();
	while (n > 0 && strchr(s, sStr[n - 1]))
		n--;

	/* Delete from n to the end. If n == sStr.size(), nothing is deleted;
	 * if n == 0, the whole string is erased. */
	sStr.erase(sStr.begin() + n, sStr.end());
}

void
Trim(std::string& sStr, const char* s)
{
	std::string::size_type b = 0, e = sStr.size();
	while (b < e && strchr(s, sStr[b]))
		++b;
	while (b < e && strchr(s, sStr[e - 1]))
		--e;
	sStr.assign(sStr.substr(b, e - b));
}

void
StripCrnl(std::string& s)
{
	while (!s.empty() && (s[s.size() - 1] == '\r' || s[s.size() - 1] == '\n'))
		s.erase(s.size() - 1);
}

bool
BeginsWith(const std::string& sTestThis, const std::string& sBeginning)
{
	assert(!sBeginning.empty());
	return sTestThis.compare(0, sBeginning.length(), sBeginning) == 0;
}

bool
EndsWith(const std::string& sTestThis, const std::string& sEnding)
{
	assert(!sEnding.empty());
	if (sTestThis.size() < sEnding.size())
		return false;
	return sTestThis.compare(sTestThis.length() - sEnding.length(),
							 sEnding.length(),
							 sEnding) == 0;
}

std::string
URLEncode(const std::string& sStr)
{
	std::string sOutput;
	for (auto t : sStr) {
		if (t >= '!' && t <= 'z')
			sOutput += t;
		else
			sOutput += "%" + ssprintf("%02X", t);
	}
	return sOutput;
}

// path is a .redir pathname. Read it and return the real one.
std::string
DerefRedir(const std::string& _path)
{
	auto sPath = _path;

	for (auto i = 0; i < 100; i++) {
		if (GetExtension(sPath) != "redir")
			return sPath;

		std::string sNewFileName;
		GetFileContents(sPath, sNewFileName, true);

		// Empty is invalid.
		if (sNewFileName.empty())
			return std::string();

		auto sPath2 = Dirname(sPath) + sNewFileName;

		CollapsePath(sPath2);

		sPath2 += "*";

		std::vector<std::string> matches;
		GetDirListing(sPath2, matches, false, true);

		if (matches.empty())
			RageException::Throw("The redirect \"%s\" references a file \"%s\" "
								 "which doesn't exist.",
								 sPath.c_str(),
								 sPath2.c_str());
		else if (matches.size() > 1)
			RageException::Throw("The redirect \"%s\" references a file \"%s\" "
								 "with multiple matches.",
								 sPath.c_str(),
								 sPath2.c_str());

		sPath = matches[0];
	}

	RageException::Throw("Circular redirect \"%s\".", sPath.c_str());
}

bool
GetFileContents(const std::string& sPath, std::string& sOut, bool bOneLine)
{
	// Don't warn if the file doesn't exist, but do warn if it exists and fails
	// to open.
	if (!IsAFile(sPath))
		return false;

	RageFile file;
	if (!file.Open(sPath)) {
		Locator::getLogger()->warn("GetFileContents({}): {}", sPath.c_str(), file.GetError().c_str());
		return false;
	}

	// todo: figure out how to make this UTF-8 safe. -aj
	std::string sData;
	int iGot;
	if (bOneLine)
		iGot = file.GetLine(sData);
	else
		iGot = file.Read(sData, file.GetFileSize());

	if (iGot == -1) {
		Locator::getLogger()->warn("GetFileContents({}): {}", sPath.c_str(), file.GetError().c_str());
		return false;
	}

	if (bOneLine)
		StripCrnl(sData);

	sOut = sData;
	return true;
}

bool
GetFileContents(const std::string& sFile, std::vector<std::string>& asOut)
{
	RageFile file;
	if (!file.Open(sFile)) {
		Locator::getLogger()->warn("GetFileContents({}): {}", sFile.c_str(), file.GetError().c_str());
		return false;
	}

	std::string sLine;
	while (file.GetLine(sLine))
		asOut.push_back(sLine);
	return true;
}

#include "pcre.h"

void
Regex::Compile()
{
	const char* error;
	int offset;
	m_pReg =
	  pcre_compile(m_sPattern.c_str(), PCRE_CASELESS, &error, &offset, nullptr);

	if (m_pReg == nullptr)
		RageException::Throw(
		  "Invalid regex: \"%s\" (%s).", m_sPattern.c_str(), error);

	const auto iRet = pcre_fullinfo(static_cast<pcre*>(m_pReg),
									nullptr,
									PCRE_INFO_CAPTURECOUNT,
									&m_iBackrefs);
	ASSERT(iRet >= 0);

	++m_iBackrefs;
	ASSERT(m_iBackrefs < 128);
}

void
Regex::Set(const std::string& sStr)
{
	Release();
	m_sPattern = sStr;
	Compile();
}

void
Regex::Release()
{
	pcre_free(m_pReg);
	m_pReg = nullptr;
	m_sPattern = std::string();
}

Regex::Regex(const std::string& sStr)
  : m_pReg(nullptr)
  , m_iBackrefs(0)
  , m_sPattern(std::string())
{
	Set(sStr);
}

Regex::Regex(const Regex& rhs)
  : m_pReg(nullptr)
  , m_iBackrefs(0)
  , m_sPattern(std::string())
{
	Set(rhs.m_sPattern);
}

Regex&
Regex::operator=(const Regex& rhs)
{
	if (this != &rhs)
		Set(rhs.m_sPattern);
	return *this;
}

Regex&
Regex::operator=(Regex&& rhs) noexcept
{
	std::swap(m_iBackrefs, rhs.m_iBackrefs);
	std::swap(m_pReg, rhs.m_pReg);
	std::swap(m_sPattern, rhs.m_sPattern);
	return *this;
}

Regex::~Regex()
{
	Release();
}

bool
Regex::Compare(const std::string& sStr)
{
	int iMat[128 * 3];
	const auto iRet = pcre_exec(static_cast<pcre*>(m_pReg),
								nullptr,
								sStr.data(),
								sStr.size(),
								0,
								0,
								iMat,
								128 * 3);

	if (iRet < -1)
		RageException::Throw("Unexpected return from pcre_exec('%s'): %i.",
							 m_sPattern.c_str(),
							 iRet);

	return iRet >= 0;
}

bool
Regex::Compare(const std::string& sStr, std::vector<std::string>& asMatches)
{
	asMatches.clear();

	int iMat[128 * 3];
	const auto iRet = pcre_exec(static_cast<pcre*>(m_pReg),
								nullptr,
								sStr.data(),
								sStr.size(),
								0,
								0,
								iMat,
								128 * 3);

	if (iRet < -1)
		RageException::Throw("Unexpected return from pcre_exec('%s'): %i.",
							 m_sPattern.c_str(),
							 iRet);

	if (iRet == -1)
		return false;

	for (unsigned i = 1; i < m_iBackrefs; ++i) {
		const auto iStart = iMat[i * 2], end = iMat[i * 2 + 1];
		if (iStart == -1)
			asMatches.push_back(std::string()); /* no match */
		else
			asMatches.push_back(sStr.substr(iStart, end - iStart));
	}

	return true;
}

// Arguments and behavior are the same are similar to
// http://us3.php.net/manual/en/function.preg-replace.php
bool
Regex::Replace(const std::string& sReplacement,
			   const std::string& sSubject,
			   std::string& sOut)
{
	std::vector<std::string> asMatches;
	if (!Compare(sSubject, asMatches))
		return false;

	sOut = sReplacement;

	// TODO: optimize me by iterating only once over the string
	for (unsigned i = 0; i < asMatches.size(); i++) {
		auto sFrom = ssprintf("\\${%d}", i);
		auto sTo = asMatches[i];
		s_replace(sOut, sFrom, sTo);
	}

	return true;
}

/* Given a UTF-8 byte, return the length of the codepoint (if a start code)
 * or 0 if it's a continuation byte. */
int
utf8_get_char_len(char p)
{
	if (!(p & 0x80))
		return 1; /* 0xxxxxxx - 1 */
	if (!(p & 0x40))
		return 1; /* 10xxxxxx - continuation */
	if (!(p & 0x20))
		return 2; /* 110xxxxx */
	if (!(p & 0x10))
		return 3; /* 1110xxxx */
	if (!(p & 0x08))
		return 4; /* 11110xxx */
	if (!(p & 0x04))
		return 5; /* 111110xx */
	if (!(p & 0x02))
		return 6; /* 1111110x */
	return 1;	  /* 1111111x */
}

static inline bool
is_utf8_continuation_byte(char c)
{
	return (c & 0xC0) == 0x80;
}

/* Decode one codepoint at start; advance start and place the result in ch.
 * If the encoded string is invalid, false is returned. */
bool
utf8_to_wchar_ec(const std::string& s, unsigned& start, wchar_t& ch)
{
	if (start >= s.size())
		return false;

	if (is_utf8_continuation_byte(s[start]) || /* misplaced continuation byte */
		(s[start] & 0xFE) == 0xFE)			   /* 0xFE, 0xFF */
	{
		start += 1;
		return false;
	}

	const auto len = utf8_get_char_len(s[start]);

	const int first_byte_mask[] = { -1, 0x7F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };

	ch = static_cast<wchar_t>(s[start] & first_byte_mask[len]);

	for (auto i = 1; i < len; ++i) {
		if (start + i >= s.size()) {
			/* We expected a continuation byte, but didn't get one. Return
			 * error, and point start at the unexpected byte; it's probably a
			 * new sequence. */
			start += i;
			return false;
		}

		const auto byte = s[start + i];
		if (!is_utf8_continuation_byte(byte)) {
			/* We expected a continuation byte, but didn't get one. Return
			 * error, and point start at the unexpected byte; it's probably a
			 * new sequence. */
			start += i;
			return false;
		}
		ch = ch << 6 | (byte & 0x3F);
	}

	auto bValid = true;
	{
		const auto c1 = static_cast<unsigned>(s[start]) & 0xFF;
		const auto c2 = static_cast<unsigned>(s[start + 1]) & 0xFF;
		const int c = (c1 << 8) + c2;
		if ((c & 0xFE00) == 0xC000 || (c & 0xFFE0) == 0xE080 ||
			(c & 0xFFF0) == 0xF080 || (c & 0xFFF8) == 0xF880 ||
			(c & 0xFFFC) == 0xFC80) {
			bValid = false;
		}
	}

	if (ch == 0xFFFE || ch == 0xFFFF)
		bValid = false;

	start += len;
	return bValid;
}

/* Like utf8_to_wchar_ec, but only does enough error checking to prevent
 * crashing. */
bool
utf8_to_wchar(const char* s, size_t iLength, unsigned& start, wchar_t& ch)
{
	if (start >= iLength)
		return false;

	const auto len = utf8_get_char_len(s[start]);

	if (start + len > iLength) {
		// We don't have room for enough continuation bytes. Return error.
		start += len;
		ch = L'?';
		return false;
	}

	switch (len) {
		case 1:
			ch = s[start + 0] & 0x7F;
			break;
		case 2:
			ch = (s[start + 0] & 0x1F) << 6 | (s[start + 1] & 0x3F);
			break;
		case 3:
			ch = (s[start + 0] & 0x0F) << 12 | (s[start + 1] & 0x3F) << 6 |
				 (s[start + 2] & 0x3F);
			break;
		case 4:
			ch = (s[start + 0] & 0x07) << 18 | (s[start + 1] & 0x3F) << 12 |
				 (s[start + 2] & 0x3F) << 6 | (s[start + 3] & 0x3F);
			break;
		case 5:
			ch = (s[start + 0] & 0x03) << 24 | (s[start + 1] & 0x3F) << 18 |
				 (s[start + 2] & 0x3F) << 12 | (s[start + 3] & 0x3F) << 6 |
				 (s[start + 4] & 0x3F);
			break;

		case 6:
			ch = (s[start + 0] & 0x01) << 30 | (s[start + 1] & 0x3F) << 24 |
				 (s[start + 2] & 0x3F) << 18 | (s[start + 3] & 0x3F) << 12 |
				 (s[start + 4] & 0x3F) << 6 | (s[start + 5] & 0x3F);
			break;
	}

	start += len;
	return true;
}

// UTF-8 encode ch and append to out.
void
wchar_to_utf8(wchar_t ch, std::string& out)
{
	if (ch < 0x80) {
		out.append(1, static_cast<char>(ch));
		return;
	}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#endif
	auto cbytes = 0;
	if (ch < 0x800)
		cbytes = 1;
	else if (ch < 0x10000)
		cbytes = 2;
	else if (ch < 0x200000)
		cbytes = 3;
	else if (ch < 0x4000000)
		cbytes = 4;
	else
		cbytes = 5;

	{
		const auto shift = cbytes * 6;
		const int init_masks[] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
		out.append(1, static_cast<char>(init_masks[cbytes - 1] | ch >> shift));
	}

	for (auto i = 0; i < cbytes; ++i) {
		const auto shift = (cbytes - i - 1) * 6;
		out.append(1, static_cast<char>(0x80 | (ch >> shift & 0x3F)));
	}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}

wchar_t
utf8_get_char(const std::string& s)
{
	unsigned start = 0;
	wchar_t ret;
	if (!utf8_to_wchar_ec(s, start, ret))
		return INVALID_CHAR;
	return ret;
}

bool
utf8_is_valid(const std::string& s)
{
	for (unsigned start = 0; start < s.size();) {
		wchar_t ch;
		if (!utf8_to_wchar_ec(s, start, ch))
			return false;
	}
	return true;
}

/* Windows tends to drop garbage BOM characters at the start of UTF-8 text
 * files. Remove them. */
void
utf8_remove_bom(std::string& sLine)
{
	if (!sLine.compare(0, 3, "\xef\xbb\xbf"))
		sLine.erase(0, 3);
}

void
MakeUpper(char* p, size_t iLen)
{
	for (size_t i = 0; *p != '\0' && i < iLen; i++, p++) {
		*p = toupper(*p);
	}
}

void
MakeLower(char* p, size_t iLen)
{
	for (size_t i = 0; *p != '\0' && i < iLen; i++, p++) {
		*p = tolower(*p);
	}
}

void
MakeUpper(wchar_t* p, size_t iLen)
{
	for (size_t i = 0; *p != L'\0' && i < iLen; i++, p++) {
		*p = towupper(*p);
	}
}

void
MakeLower(wchar_t* p, size_t iLen)
{
	for (size_t i = 0; *p != L'\0' && i < iLen; i++, p++) {
		*p = towlower(*p);
	}
}

void
MakeLower(std::string& data)
{
	std::transform(std::begin(data),
				   std::end(data),
				   std::begin(data),
				   [](unsigned char c) { return std::tolower(c); });
}

int
StringToInt(const std::string& sString)
{
	int ret;
	std::istringstream(sString) >> ret;
	return ret;
}

std::string
IntToString(const int& iNum)
{
	std::stringstream ss;
	ss << iNum;
	return ss.str();
}

float
StringToFloat(const std::string& sString)
{
	auto ret = strtof(sString.c_str(), nullptr);

	if (!std::isfinite(ret))
		ret = 0.0f;
	return ret;
}

bool
StringToFloat(const std::string& sString, float& fOut)
{
	char* endPtr;

	fOut = strtof(sString.c_str(), &endPtr);
	return !sString.empty() && *endPtr == '\0' && std::isfinite(fOut);
}

std::string
FloatToString(const float& num)
{
	std::stringstream ss;
	ss << num;
	return ss.str();
}

const wchar_t INVALID_CHAR = 0xFFFD; /* U+FFFD REPLACEMENT CHARACTER */

wstring
StringToWString(const std::string& s)
{
	wstring ret;
	ret.reserve(s.size());
	for (unsigned start = 0; start < s.size();) {
		const auto c = s[start];
		if (!(c & 0x80)) {
			// ASCII fast path
			ret += c;
			++start;
			continue;
		}

		auto ch = L'\0';
		if (!utf8_to_wchar(s.data(), s.size(), start, ch))
			ch = INVALID_CHAR;
		ret += ch;
	}

	return ret;
}

std::string
WStringToString(const wstring& sStr)
{
	std::string sRet;

	for (auto i : sStr)
		wchar_to_utf8(i, sRet);

	return sRet;
}

std::string
WcharToUTF8(wchar_t c)
{
	std::string ret;
	wchar_to_utf8(c, ret);
	return ret;
}

// &a; -> a
void
ReplaceEntityText(std::string& sText,
				  const std::map<std::string, std::string>& m)
{
	std::string sRet;

	size_t iOffset = 0;
	while (iOffset != sText.size()) {
		const auto iStart = sText.find('&', iOffset);
		if (iStart == std::string::npos) {
			// Optimization: if we didn't replace anything at all, do nothing.
			if (iOffset == 0)
				return;

			// Append the rest of the string.
			sRet.append(sText, iOffset, std::string::npos);
			break;
		}

		// Append the text between iOffset and iStart.
		sRet.append(sText, iOffset, iStart - iOffset);
		iOffset += iStart - iOffset;

		// Optimization: stop early on "&", so "&&&&&&&&&&&" isn't n^2.
		const auto iEnd = sText.find_first_of("&;", iStart + 1);
		if (iEnd == std::string::npos || sText[iEnd] == '&') {
			// & with no matching ;, or two & in a row. Append the & and
			// continue.
			sRet.append(sText, iStart, 1);
			++iOffset;
			continue;
		}

		auto sElement = sText.substr(iStart + 1, iEnd - iStart - 1);
		sElement = make_lower(sElement);

		auto it = m.find(sElement);
		if (it == m.end()) {
			sRet.append(sText, iStart, iEnd - iStart + 1);
			iOffset = iEnd + 1;
			continue;
		}

		const auto& sTo = it->second;
		sRet.append(sTo);
		iOffset = iEnd + 1;
	}

	sText = sRet;
}

// abcd -> &a; &b; &c; &d;
void
ReplaceEntityText(std::string& sText, const std::map<char, std::string>& m)
{
	std::string sFind;

	for (const auto& c : m)
		sFind.append(1, c.first);

	std::string sRet;

	size_t iOffset = 0;
	while (iOffset != sText.size()) {
		const auto iStart = sText.find_first_of(sFind, iOffset);
		if (iStart == std::string::npos) {
			// Optimization: if we didn't replace anything at all, do nothing.
			if (iOffset == 0)
				return;

			// Append the rest of the string.
			sRet.append(sText, iOffset, std::string::npos);
			break;
		}

		// Append the text between iOffset and iStart.
		sRet.append(sText, iOffset, iStart - iOffset);
		iOffset += iStart - iOffset;

		auto sElement = sText[iStart];

		auto it = m.find(sElement);
		ASSERT(it != m.end());

		const auto& sTo = it->second;
		sRet.append(1, '&');
		sRet.append(sTo);
		sRet.append(1, ';');
		++iOffset;
	}

	sText = sRet;
}

// Replace &#nnnn; (decimal) and &xnnnn; (hex) with corresponding UTF-8
// characters.
void
Replace_Unicode_Markers(std::string& sText)
{
	unsigned iStart = 0;
	while (iStart < sText.size()) {
		// Look for &#digits;
		auto bHex = false;
		auto iPos = sText.find("&#", iStart);
		if (iPos == std::string::npos) {
			bHex = true;
			iPos = sText.find("&x", iStart);
		}

		if (iPos == std::string::npos)
			break;
		iStart = iPos + 1;

		unsigned p = iPos;
		p += 2;

		// Found &# or &x. Is it followed by digits and a semicolon?
		if (p >= sText.size())
			continue;

		auto iNumDigits = 0;
		while (p < sText.size() && bHex ? isxdigit(sText[p])
										: isdigit(sText[p])) {
			p++;
			iNumDigits++;
		}

		if (!iNumDigits)
			continue; // must have at least one digit
		if (p >= sText.size() || sText[p] != ';')
			continue;
		p++;

		int iNum;
		if (bHex)
			sscanf(sText.c_str() + iPos, "&x%x;", &iNum);
		else
			sscanf(sText.c_str() + iPos, "&#%i;", &iNum);
		if (iNum > 0xFFFF)
			iNum = INVALID_CHAR;

		sText.replace(iPos, p - iPos, WcharToUTF8(static_cast<wchar_t>(iNum)));
	}
}

// Form a string to identify a wchar_t with ASCII.
std::string
WcharDisplayText(wchar_t c)
{
	std::string sChr;
	sChr = ssprintf("U+%4.4x", c);
	if (c < 128)
		sChr += ssprintf(" ('%c')", static_cast<char>(c));
	return sChr;
}

/* Return the last named component of dir:
 * a/b/c -> c
 * a/b/c/ -> c
 */
std::string
Basename(const std::string& sDir)
{
	const auto iEnd = sDir.find_last_not_of("/\\");
	if (iEnd == std::string::npos)
		return std::string();

	auto iStart = sDir.find_last_of("/\\", iEnd);
	if (iStart == std::string::npos)
		iStart = 0;
	else
		++iStart;

	return sDir.substr(iStart, iEnd - iStart + 1);
}

/* Return all but the last named component of dir:
 *
 * a/b/c -> a/b/
 * a/b/c/ -> a/b/
 * c/ -> ./
 * /foo -> /
 * / -> /
 */
std::string
Dirname(const std::string& dir)
{
	// Special case: "/" -> "/".
	if (dir.size() == 1 && dir[0] == '/')
		return "/";

	int pos = dir.size() - 1;
	// Skip trailing slashes.
	while (pos >= 0 && dir[pos] == '/')
		--pos;

	// Skip the last component.
	while (pos >= 0 && dir[pos] != '/')
		--pos;

	if (pos < 0)
		return "./";

	return dir.substr(0, pos + 1);
}

std::string
Capitalize(const std::string& s)
{
	if (s.empty())
		return std::string();

	auto* buf = const_cast<char*>(s.c_str());
	MakeUpper(buf, 1);
	return buf;
}

unsigned char g_UpperCase[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
	0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23,
	0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
	0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53,
	0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B,
	0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83,
	0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B,
	0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
	0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3,
	0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB,
	0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
	0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xC0, 0xC1, 0xC2, 0xC3,
	0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xF7, 0xD8, 0xD9, 0xDA, 0xDB,
	0xDC, 0xDD, 0xDE, 0xFF,
};

unsigned char g_LowerCase[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
	0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23,
	0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
	0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73,
	0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B,
	0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83,
	0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B,
	0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
	0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3,
	0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB,
	0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
	0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xC0, 0xC1, 0xC2, 0xC3,
	0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xF7, 0xD8, 0xD9, 0xDA, 0xDB,
	0xDC, 0xDD, 0xDE, 0xFF,
};

void
FixSlashesInPlace(std::string& sPath)
{
	for (auto& i : sPath)
		if (i == '\\')
			i = '/';
}

/* Keep trailing slashes, since that can be used to illustrate that a path
 * always represents a directory.
 *
 * foo/bar -> foo/bar
 * foo/bar/ -> foo/bar/
 * foo///bar/// -> foo/bar/
 * foo/bar/./baz -> foo/bar/baz
 * foo/bar/../baz -> foo/baz
 * ../foo -> ../foo
 * ../../foo -> ../../foo
 * ./foo -> foo (if bRemoveLeadingDot), ./foo (if !bRemoveLeadingDot)
 * ./ -> ./
 * ./// -> ./
 */

void
CollapsePath(std::string& sPath, bool bRemoveLeadingDot)
{
	std::string sOut;
	sOut.reserve(sPath.size());

	size_t iPos = 0;
	size_t iNext;
	for (; iPos < sPath.size(); iPos = iNext) {
		// Find the next slash.
		iNext = sPath.find('/', iPos);
		if (iNext == std::string::npos)
			iNext = sPath.size();
		else
			++iNext;

		/* Strip extra slashes, but don't remove slashes from the beginning of
		 * the string. */
		if (iNext - iPos == 1 && sPath[iPos] == '/') {
			if (!sOut.empty())
				continue;
		}

		// If this is a dot, skip it.
		if (iNext - iPos == 2 && sPath[iPos] == '.' && sPath[iPos + 1] == '/') {
			if (bRemoveLeadingDot || !sOut.empty())
				continue;
		}

		// If this is two dots,
		if (iNext - iPos == 3 && sPath[iPos] == '.' && sPath[iPos + 1] == '.' &&
			sPath[iPos + 2] == '/') {
			/* If this is the first path element (nothing to delete),
			 * or all we have is a slash, leave it. */
			if (sOut.empty() || (sOut.size() == 1 && sOut[0] == '/')) {
				sOut.append(sPath, iPos, iNext - iPos);
				continue;
			}

			// Search backwards for the previous path element.
			auto iPrev = sOut.rfind('/', sOut.size() - 2);
			if (iPrev == std::string::npos)
				iPrev = 0;
			else
				++iPrev;

			// If the previous element is also .., leave it.
			const auto bLastIsTwoDots = sOut.size() - iPrev == 3 &&
										sOut[iPrev] == '.' &&
										sOut[iPrev + 1] == '.';
			if (bLastIsTwoDots) {
				sOut.append(sPath, iPos, iNext - iPos);
				continue;
			}

			sOut.erase(iPrev);
			continue;
		}

		sOut.append(sPath, iPos, iNext - iPos);
	}

	sOut.swap(sPath);
}

namespace StringConversion {
template<>
bool
FromString<int>(const std::string& sValue, int& out)
{
	if (sscanf(sValue.c_str(), "%d", &out) == 1)
		return true;

	out = 0;
	return false;
}

template<>
bool
FromString<unsigned>(const std::string& sValue, unsigned& out)
{
	if (sscanf(sValue.c_str(), "%u", &out) == 1)
		return true;

	out = 0;
	return false;
}

template<>
bool
FromString<float>(const std::string& sValue, float& out)
{
	const auto* endptr = sValue.data() + sValue.size();
	out = strtof(sValue.c_str(), (char**)&endptr);
	if (endptr != sValue.data() && std::isfinite(out))
		return true;
	out = 0;
	return false;
}

template<>
bool
FromString<bool>(const std::string& sValue, bool& out)
{
	if (sValue.empty())
		return false;

	out = StringToInt(sValue) != 0;
	return true;
}

template<>
std::string
ToString<int>(const int& value)
{
	return ssprintf("%i", value);
}

template<>
std::string
ToString<unsigned>(const unsigned& value)
{
	return ssprintf("%u", value);
}

template<>
std::string
ToString<float>(const float& value)
{
	return ssprintf("%f", value);
}

template<>
std::string
ToString<bool>(const bool& value)
{
	return ssprintf("%i", value);
}
} // namespace StringConversion

bool
FileCopy(const std::string& sSrcFile, const std::string& sDstFile)
{
	if (!CompareNoCase(sSrcFile, sDstFile)) {
		Locator::getLogger()->warn("Tried to copy \"{}\" over itself", sSrcFile.c_str());
		return false;
	}

	RageFile in;
	if (!in.Open(sSrcFile, RageFile::READ))
		return false;

	RageFile out;
	if (!out.Open(sDstFile, RageFile::WRITE))
		return false;

	std::string sError;
	if (!FileCopy(in, out, sError)) {
		Locator::getLogger()->warn("FileCopy({},{}): {}", sSrcFile.c_str(), sDstFile.c_str(), sError.c_str());
		return false;
	}

	return true;
}

bool
FileCopy(RageFileBasic& in,
		 RageFileBasic& out,
		 std::string& sError,
		 bool* bReadError)
{
	for (;;) {
		std::string data;
		if (in.Read(data, 1024 * 32) == -1) {
			sError = ssprintf("read error: %s", in.GetError().c_str());
			if (bReadError != nullptr) {
				*bReadError = true;
			}
			return false;
		}
		if (data.empty()) {
			break;
		}
		const auto i = out.Write(data);
		if (i == -1) {
			sError = ssprintf("write error: %s", out.GetError().c_str());
			if (bReadError != nullptr) {
				*bReadError = false;
			}
			return false;
		}
	}

	if (out.Flush() == -1) {
		sError = ssprintf("write error: %s", out.GetError().c_str());
		if (bReadError != nullptr) {
			*bReadError = false;
		}
		return false;
	}

	return true;
}

LuaFunction(SecondsToMSSMsMs, SecondsToMSSMsMs(FArg(1)))
  LuaFunction(SecondsToHHMMSS, SecondsToHHMMSS(FArg(1)))
	LuaFunction(SecondsToMMSSMsMs, SecondsToMMSSMsMs(FArg(1)))
	  LuaFunction(SecondsToMMSSMsMsMs, SecondsToMMSSMsMsMs(FArg(1)))
		LuaFunction(SecondsToMSS, SecondsToMSS(FArg(1)))
		  LuaFunction(SecondsToMMSS, SecondsToMMSS(FArg(1)))
			LuaFunction(FormatNumberAndSuffix, FormatNumberAndSuffix(IArg(1)))
			  LuaFunction(Basename, Basename(SArg(1))) static std::string
  MakeLower(const std::string& s)
{
	return make_lower(s);
}
LuaFunction(Lowercase, MakeLower(SArg(1))) static std::string
  MakeUpper(const std::string& s)
{
	return make_upper(s);
}
LuaFunction(Uppercase, MakeUpper(SArg(1)))
  LuaFunction(mbstrlen, static_cast<int>(StringToWString(SArg(1)).length()))
	LuaFunction(URLEncode, URLEncode(SArg(1)));
LuaFunction(PrettyPercent, PrettyPercent(FArg(1), FArg(2)));
// LuaFunction( IsHexVal, IsHexVal( SArg(1) ) );
LuaFunction(lerp, lerp(FArg(1), FArg(2), FArg(3)));

int
LuaFunc_commify(lua_State* L);
int
LuaFunc_commify(lua_State* L)
{
	const std::string num = SArg(1);
	std::string sep = ",";
	std::string dot = ".";
	if (!lua_isnoneornil(L, 2)) {
		sep = lua_tostring(L, 2);
	}
	if (!lua_isnoneornil(L, 3)) {
		dot = lua_tostring(L, 3);
	}
	const auto ret = Commify(num, sep, dot);
	LuaHelpers::Push(L, ret);
	return 1;
}
LUAFUNC_REGISTER_COMMON(commify);

void
luafunc_approach_internal(lua_State* L,
						  int valind,
						  int goalind,
						  int speedind,
						  float mult);
void
luafunc_approach_internal(lua_State* L,
						  int valind,
						  int goalind,
						  int speedind,
						  const float mult,
						  int process_index)
{
#define TONUMBER_NICE(dest, num_name, index)                                   \
	if (!lua_isnumber(L, index)) {                                             \
		luaL_error(L,                                                          \
				   "approach: " #num_name " for approach %d is not a number.", \
				   process_index);                                             \
	}                                                                          \
	(dest) = static_cast<float>(lua_tonumber(L, index));
	float val = 0;
	float goal = 0;
	float speed = 0;
	TONUMBER_NICE(val, current, valind);
	TONUMBER_NICE(goal, goal, goalind);
	TONUMBER_NICE(speed, speed, speedind);
#undef TONUMBER_NICE
	if (speed < 0) {
		luaL_error(L, "approach: speed %d is negative.", process_index);
	}
	fapproach(val, goal, speed * mult);
	lua_pushnumber(L, val);
}

int
LuaFunc_approach(lua_State* L);
int
LuaFunc_approach(lua_State* L)
{
	// Args:  current, goal, speed
	// Returns:  new_current
	luafunc_approach_internal(L, 1, 2, 3, 1.0f, 1);
	return 1;
}
LUAFUNC_REGISTER_COMMON(approach);

int
LuaFunc_multiapproach(lua_State* L);
int
LuaFunc_multiapproach(lua_State* L)
{
	// Args:  {currents}, {goals}, {speeds}, speed_multiplier
	// speed_multiplier is optional, and is intended to be the delta time for
	// the frame, so that this can be used every frame and have the current
	// approach the goal at a framerate independent speed.
	// Returns:  {currents}
	// Modifies the values in {currents} in place.
	if (lua_gettop(L) < 3) {
		luaL_error(L,
				   "multiapproach:  A table of current values, a table of goal "
				   "values, and a table of speeds must be passed.");
	}
	const auto currents_len = lua_objlen(L, 1);
	const auto goals_len = lua_objlen(L, 2);
	const auto speeds_len = lua_objlen(L, 3);
	auto mult = 1.0f;
	if (lua_isnumber(L, 4) != 0) {
		mult = static_cast<float>(lua_tonumber(L, 4));
	}
	if (currents_len != goals_len || currents_len != speeds_len) {
		luaL_error(L,
				   "multiapproach:  There must be the same number of current "
				   "values, goal values, and speeds.");
	}
	if (!lua_istable(L, 1) || !lua_istable(L, 2) || !lua_istable(L, 3)) {
		luaL_error(
		  L, "multiapproach:  current, goal, and speed must all be tables.");
	}
	for (size_t i = 1; i <= currents_len; ++i) {
		lua_rawgeti(L, 1, i);
		lua_rawgeti(L, 2, i);
		lua_rawgeti(L, 3, i);
		luafunc_approach_internal(L, -3, -2, -1, mult, i);
		lua_rawseti(L, 1, i);
		lua_pop(L, 3);
	}
	lua_pushvalue(L, 1);
	return 1;
}
LUAFUNC_REGISTER_COMMON(multiapproach);

int
LuaFunc_get_music_file_length(lua_State* L);
int
LuaFunc_get_music_file_length(lua_State* L)
{
	// Args:  file_path
	// Returns:  The length of the music in seconds.
	const std::string path = SArg(1);
	std::string error;
	RageSoundReader* sample = RageSoundReader_FileReader::OpenFile(path, error);
	if (sample == nullptr) {
		luaL_error(L, "The music file '%s' does not exist.", path.c_str());
		lua_pushnil(L);
	} else
		lua_pushnumber(L, sample->GetLength() / 1000.0f);
	return 1;
}
LUAFUNC_REGISTER_COMMON(get_music_file_length);
