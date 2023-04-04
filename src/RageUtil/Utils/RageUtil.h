/** @brief RageUtil - Miscellaneous helper macros and functions. */

#ifndef RAGE_UTIL_H
#define RAGE_UTIL_H

#include <map>
#include <sstream>
#include <cstring>
#include <vector>
#include <memory>
#include <cmath>

class RageFileDriver;

/** @brief Safely delete pointers. */
#define SAFE_DELETE(p)                                                         \
	do {                                                                       \
		delete (p);                                                            \
		(p) = nullptr;                                                         \
	} while (false)
/** @brief Safely delete array pointers. */
#define SAFE_DELETE_ARRAY(p)                                                   \
	do {                                                                       \
		delete[](p);                                                           \
		(p) = nullptr;                                                         \
	} while (false)

/** @brief Zero out the memory. */
#define ZERO(x) memset(&(x), 0, sizeof(x))

/** @brief Get the length of the array. */
#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))

/**
 * @brief Scales x so that l1 corresponds to l2 and h1 corresponds to h2.
 *
 * This does not modify x, so it MUST assign the result to something!
 * Do the multiply before the divide to that integer scales have more precision.
 *
 * One such example: SCALE(x, 0, 1, L, H); interpolate between L and H.
 */
#define SCALE(x, l1, h1, l2, h2)                                               \
	(((x) - (l1)) * ((h2) - (l2)) / ((h1) - (l1)) + (l2))

template<typename T, typename U>
auto
lerp(T x, U l, U h) -> U
{
	return static_cast<U>(x * (h - l) + l);
}

template<typename T, typename U, typename V>
auto
CLAMP(T& x, U l, V h) -> bool
{
	if (x > static_cast<T>(h)) {
		x = static_cast<T>(h);
		return true;
	}
	if (x < static_cast<T>(l)) {
		x = static_cast<T>(l);
		return true;
	}
	return false;
}

template<class T>
auto
ENUM_CLAMP(T& x, T l, T h) -> bool
{
	if (x > h) {
		x = h;
		return true;
	} else if (x < l) {
		x = l;
		return true;
	}
	return false;
}

inline auto
wife2(float maxms, float ts) -> float
{
	maxms = maxms * 1000.F;
	float avedeviation = 95.F * ts;
	float y = 1 - static_cast<float>(
					pow(2, -1 * maxms * maxms / (avedeviation * avedeviation)));
	y = static_cast<float>(pow(y, 2));
	return (2 - -8) * (1 - y) + -8;
}

static const float wife3_mine_hit_weight = -7.F;
static const float wife3_hold_drop_weight = -4.5F;
static const float wife3_miss_weight = -5.5F;

// erf approximation A&S formula 7.1.26
inline auto
werwerwerwerf(float x) -> float
{
	static const float a1 = 0.254829592F;
	static const float a2 = -0.284496736F;
	static const float a3 = 1.421413741F;
	static const float a4 = -1.453152027F;
	static const float a5 = 1.061405429F;
	static const float p = 0.3275911F;

	int sign = 1;
	if (x < 0.F) {
		sign = -1;
	}
	x = std::abs(x);

	auto t = 1.F / (1.F + p * x);
	auto y =
	  1.F - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * exp(-x * x);

	return sign * y;
}

inline auto
wife3(float maxms, float ts) -> float
{
	// so judge scaling isn't so extreme
	static const float j_pow = 0.75F;
	// min/max points
	static const float max_points = 2.F;
	// offset at which points starts decreasing(ms)
	float ridic = 5.F * ts;

	// technically the max boo is always 180ms above j4 however this is
	// immaterial to the end purpose of the scoring curve - assignment of point
	// values
	float max_boo_weight = 180.F * ts;

	// need positive values for this
	maxms = std::abs(maxms * 1000.F);

	// case optimizations
	if (maxms <= ridic) {
		return max_points;
	}

	// piecewise inflection
	float zero = 65.F * pow(ts, j_pow);
	float dev = 22.7F * pow(ts, j_pow);

	if (maxms <= zero) {
		return max_points * werwerwerwerf((zero - maxms) / dev);
	}
	if (maxms <= max_boo_weight) {
		return (maxms - zero) * wife3_miss_weight / (max_boo_weight - zero);
	}
	return wife3_miss_weight;
}

inline void
wrap(int& x, int n)
{
	if (x < 0) {
		x += ((-x / n) + 1) * n;
	}
	x %= n;
}
inline void
wrap(unsigned& x, unsigned n)
{
	x %= n;
}
inline void
wrap(float& x, float n)
{
	if (x < 0) {
		x += truncf(((-x / n) + 1)) * n;
	}
	x = fmodf(x, n);
}

inline auto
fracf(float f) -> float
{
	return f - truncf(f);
}

template<class T>
void
CircularShift(std::vector<T>& v, int dist)
{
	for (int i = std::abs(dist); i > 0; i--) {
		if (v.size() == 0)
			break;
		if (dist > 0) {
			T t = v[0];
			v.erase(v.begin());
			v.push_back(t);
		} else {
			T t = v.back();
			v.erase(v.end() - 1);
			v.insert(v.begin(), t);
		}
	}
}

inline auto
sstolower(char ch) -> char
{
	return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch + 'a' - 'A') : ch;
}

template<typename CT>
inline auto
ssicmp(const CT* pA1, const CT* pA2) -> int
{
	CT f;
	CT l;

	do {
		f = sstolower(*(pA1++));
		l = sstolower(*(pA2++));
	} while ((f) && (f == l));

	return static_cast<int>(f - l);
}

inline auto
CompareNoCase(const std::string& a, const std::string& b) -> int
{
	return ssicmp(a.c_str(), b.c_str());
}

inline auto
EqualsNoCase(const std::string& a, const std::string& b) -> bool
{
	return CompareNoCase(a, b) == 0;
}

void
s_replace(std::string& target, std::string const& from, std::string const& to);

inline void
ensure_slash_at_end(std::string& s)
{
	if (s.back() != '/') {
		s += "/";
	}
}

/** @brief Determine if the source string begins with the specified content. */
auto
starts_with(std::string const& source, std::string const& target) -> bool;

/** @brief Determine if the source string ends with the specified content. */
auto
ends_with(std::string const& source, std::string const& target) -> bool;

/** @brief Convert the string into its uppercase variant. */
auto
make_upper(std::string const& source) -> std::string;

/** @brief Convert the string into its lowercase variant. */
auto
make_lower(std::string const& source) -> std::string;

template<typename Type, typename Ret>
static auto
CreateClass() -> Ret*
{
	return new Type;
}

/* Helper for ConvertValue(). */
template<typename TO, typename FROM>
struct ConvertValueHelper
{
	explicit ConvertValueHelper(FROM* pVal)
	  : m_pFromValue(pVal)
	{
		m_ToValue = static_cast<TO>(*m_pFromValue);
	}

	~ConvertValueHelper() { *m_pFromValue = static_cast<FROM>(m_ToValue); }

	auto operator*() -> TO& { return m_ToValue; }
	operator TO*() { return &m_ToValue; }

  private:
	FROM* m_pFromValue;
	TO m_ToValue;
};

/* Safely temporarily convert between types.  For example,
 *
 * float f = 10.5;
 * *ConvertValue<int>(&f) = 12;
 */
template<typename TO, typename FROM>
auto
ConvertValue(FROM* pValue) -> ConvertValueHelper<TO, FROM>
{
	return ConvertValueHelper<TO, FROM>(pValue);
}

/* Safely add an integer to an enum.
 *
 * This is illegal:
 *
 *  ((int&)val) += iAmt;
 *
 * It breaks aliasing rules; the compiler is allowed to assume that "val"
 * doesn't change (unless it's declared volatile), and in some cases, you'll end
 * up getting old values for "val" following the add.  (What's probably really
 * happening is that the memory location is being added to, but the value is
 * stored in a register, and breaking aliasing rules means the compiler doesn't
 * know that the register value is invalid.)
 */
template<typename T>
static void
enum_add(T& val, int iAmt)
{
	val = static_cast<T>(val + iAmt);
}

template<typename T>
static auto
enum_add2(T val, int iAmt) -> T
{
	return static_cast<T>(val + iAmt);
}

template<typename T>
static auto
enum_cycle(T val, int iMax, int iAmt = 1) -> T
{
	int iVal = val + iAmt;
	iVal %= iMax;
	return static_cast<T>(iVal);
}

/* We only have unsigned swaps; byte swapping a signed value doesn't make sense.
 *
 * Platform-specific, optimized versions are defined in arch_setup, with the
 * names ArchSwap32, ArchSwap24, and ArchSwap16; we define them to their real
 * names here, to force inclusion of this file when swaps are in use (to prevent
 * different dependencies on different systems).
 */
#ifdef HAVE_BYTE_SWAPS
#define Swap32 ArchSwap32
#define Swap24 ArchSwap24
#define Swap16 ArchSwap16
#else
inline auto
Swap32(uint32_t n) -> uint32_t
{
	return (n >> 24) | ((n >> 8) & 0x0000FF00) | ((n << 8) & 0x00FF0000) |
		   (n << 24);
}

inline auto
Swap24(uint32_t n) -> uint32_t
{
	return Swap32(n) >> 8; // xx223344 -> 443322xx -> 00443322
}

inline auto
Swap16(uint16_t n) -> uint16_t
{
	return (n >> 8) | (n << 8);
}
#endif

inline auto
Swap32LE(uint32_t n) -> uint32_t
{
	return n;
}
inline auto
Swap24LE(uint32_t n) -> uint32_t
{
	return n;
}
inline auto
Swap16LE(uint16_t n) -> uint16_t
{
	return n;
}
inline auto
Swap32BE(uint32_t n) -> uint32_t
{
	return Swap32(n);
}
inline auto
Swap24BE(uint32_t n) -> uint32_t
{
	return Swap24(n);
}
inline auto
Swap16BE(uint16_t n) -> uint16_t
{
	return Swap16(n);
}

/* return f rounded to the nearest multiple of fRoundInterval */
inline auto
Quantize(const float f, const float fRoundInterval) -> float
{
	return static_cast<int>((f + fRoundInterval / 2) / fRoundInterval) *
		   fRoundInterval;
}

inline auto
Quantize(const int i, const int iRoundInterval) -> int
{
	return static_cast<int>((i + iRoundInterval / 2) / iRoundInterval) *
		   iRoundInterval;
}

/* return f truncated to the nearest multiple of fTruncInterval */
inline auto
ftruncf(const float f, const float fTruncInterval) -> float
{
	return static_cast<int>((f) / fTruncInterval) * fTruncInterval;
}

/* Return i rounded up to the nearest multiple of iInterval. */
inline auto
QuantizeUp(int i, int iInterval) -> int
{
	return static_cast<int>((i + iInterval - 1) / iInterval) * iInterval;
}

inline auto
QuantizeUp(float i, float iInterval) -> float
{
	return ceilf(i / iInterval) * iInterval;
}

/* Return i rounded down to the nearest multiple of iInterval. */
inline auto
QuantizeDown(int i, int iInterval) -> int
{
	return static_cast<int>((i - iInterval + 1) / iInterval) * iInterval;
}

inline auto
QuantizeDown(float i, float iInterval) -> float
{
	return floorf(i / iInterval) * iInterval;
}

// Move val toward other_val by to_move.
void
fapproach(float& val, float other_val, float to_move);

/* Return a positive x mod y. */
inline auto
fmodfp(float x, float y) -> float
{
	x = fmodf(x, y); /* x is [-y,y] */
	x += y;			 /* x is [0,y*2] */
	x = fmodf(x, y); /* x is [0,y] */
	return x;
}

inline auto
power_of_two(int input) -> int
{
	auto exp = 31;
	auto i = input;
	if (i >> 16 != 0) {
		i >>= 16;
	} else {
		exp -= 16;
	}
	if (i >> 8 != 0) {
		i >>= 8;
	} else {
		exp -= 8;
	}
	if (i >> 4 != 0) {
		i >>= 4;
	} else {
		exp -= 4;
	}
	if (i >> 2 != 0) {
		i >>= 2;
	} else {
		exp -= 2;
	}
	if (i >> 1 == 0) {
		exp -= 1;
	}
	const auto value = 1 << exp;
	return input == value ? value : value << 1;
}
inline auto
IsAnInt(const std::string& s) -> bool
{
	if (s.empty()) {
		return false;
	}

	for (auto i : s) {
		if (i < '0' || i > '9') {
			return false;
		}
	}

	return true;
}
auto
IsHexVal(const std::string& s) -> bool;
auto
BinaryToHex(const void* pData_, int iNumBytes) -> std::string;
auto
BinaryToHex(const std::string& sString) -> std::string;
auto
HHMMSSToSeconds(const std::string& sHMS) -> float;
auto
SecondsToHHMMSS(float fSecs) -> std::string;
auto
SecondsToMSSMsMs(float fSecs) -> std::string;
auto
SecondsToMMSSMsMs(float fSecs) -> std::string;
auto
SecondsToMMSSMsMsMs(float fSecs) -> std::string;
auto
SecondsToMSS(float fSecs) -> std::string;
auto
SecondsToMMSS(float fSecs) -> std::string;
auto
PrettyPercent(float fNumerator, float fDenominator) -> std::string;
inline auto
PrettyPercent(int fNumerator, int fDenominator) -> std::string
{
	return PrettyPercent(float(fNumerator), float(fDenominator));
}
auto
Commify(int iNum) -> std::string;
auto
Commify(const std::string& num,
		const std::string& sep = ",",
		const std::string& dot = ".") -> std::string;
auto
FormatNumberAndSuffix(int i) -> std::string;

auto
GetLocalTime() -> struct tm;

// Supress warnings about format strings not being string literals
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#elif defined(_MSC_VER)
// TODO: Suppress warnings for Windows
#endif
template<typename... Args>
auto
ssprintf(const char* format, Args... args) -> std::string
{
	// Extra space for '\0'
	size_t size = snprintf(nullptr, 0, format, args...) + 1;
	std::unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, format, args...);
	// Don't want the '\0' inside
	return std::string(buf.get(), buf.get() + size - 1);
}
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC pop
#elif defined(_MSC_VER)
// TODO: Suppress warnings for Windows
#endif

template<typename... Args>
auto
ssprintf(const std::string& format, Args... args) -> std::string
{
	return ssprintf(format.c_str(), args...);
}

auto
vssprintf(const char* fmt, va_list argList) -> std::string;
auto
ConvertI64FormatString(const std::string& sStr) -> std::string;

/*
 * Splits a Path into 4 parts (Directory, Drive, Filename, Extention).
 * Supports UNC path names. If Path is a directory (eg.
 * c:\games\stepmania"), append a slash so the last element will end up in
 * Dir, not FName: "c:\games\stepmania\".
 * */
void
splitpath(const std::string& Path,
		  std::string& Dir,
		  std::string& Filename,
		  std::string& Ext);

auto
SetExtension(const std::string& path, const std::string& ext) -> std::string;
auto
GetExtension(const std::string& sPath) -> std::string;
auto
GetFileNameWithoutExtension(const std::string& sPath) -> std::string;
void
MakeValidFilename(std::string& sName);

auto
FindFirstFilenameContaining(const std::vector<std::string>& filenames,
							std::string& out,
							const std::vector<std::string>& starts_with,
							const std::vector<std::string>& contains,
							const std::vector<std::string>& ends_with) -> bool;

extern const wchar_t INVALID_CHAR;

auto
utf8_get_char_len(char p) -> int;
auto
utf8_to_wchar(const char* s, size_t iLength, unsigned& start, wchar_t& ch)
  -> bool;
auto
utf8_to_wchar_ec(const std::string& s, unsigned& start, wchar_t& ch) -> bool;
void
wchar_to_utf8(wchar_t ch, std::string& out);
auto
utf8_get_char(const std::string& s) -> wchar_t;
auto
utf8_is_valid(const std::string& s) -> bool;
void
utf8_remove_bom(std::string& s);
void
MakeUpper(char* p, size_t iLen);
void
MakeLower(char* p, size_t iLen);
void
MakeLower(std::string& data);
void
MakeUpper(wchar_t* p, size_t iLen);
void
MakeLower(wchar_t* p, size_t iLen);
/**
 * @brief Have a standard way of converting Strings to integers.
 * @param sString the string to convert.
 * @return the integer we are after. */
auto
StringToInt(const std::string& sString) -> int;
/**
 * @brief Have a standard way of converting integers to Strings.
 * @param iNum the integer to convert.
 * @return the string we are after. */
auto
IntToString(const int& iNum) -> std::string;
auto
StringToFloat(const std::string& sString) -> float;
auto
FloatToString(const float& num) -> std::string;
auto
StringToFloat(const std::string& sString, float& fOut) -> bool;
// Better than IntToString because you can check for success.
template<class T>
auto
operator>>(const std::string& lhs, T& rhs) -> bool
{
	return !!(std::istringstream(lhs) >> rhs);
}

auto
WStringToString(const std::wstring& sString) -> std::string;
auto
WcharToUTF8(wchar_t c) -> std::string;
auto
StringToWString(const std::string& sString) -> std::wstring;

struct LanguageInfo
{
	const char* szIsoCode;
	const char* szEnglishName;
};
void
GetLanguageInfos(std::vector<const LanguageInfo*>& vAddTo);
auto
GetLanguageInfo(const std::string& sIsoCode) -> const LanguageInfo*;

// Splits a std::string into an std::vector<std::string> according the
// Delimitor.
void
split(const std::string& sSource,
	  const std::string& sDelimitor,
	  std::vector<std::string>& asAddIt,
	  bool bIgnoreEmpty = true);
void
split(const std::wstring& sSource,
	  const std::wstring& sDelimitor,
	  std::vector<std::wstring>& asAddIt,
	  bool bIgnoreEmpty = true);

/* In-place split. */
void
split(const std::string& sSource,
	  const std::string& sDelimitor,
	  int& iBegin,
	  int& iSize,
	  bool bIgnoreEmpty = true);
void
split(const std::wstring& sSource,
	  const std::wstring& sDelimitor,
	  int& iBegin,
	  int& iSize,
	  bool bIgnoreEmpty = true);

/* In-place split of partial string. */
void
split(const std::string& sSource,
	  const std::string& sDelimitor,
	  int& iBegin,
	  int& iSize,
	  int iLen,
	  bool bIgnoreEmpty); /* no default to avoid ambiguity */
void
split(const std::wstring& sSource,
	  const std::wstring& sDelimitor,
	  int& iBegin,
	  int& iSize,
	  int iLen,
	  bool bIgnoreEmpty);

// Joins a std::vector<std::string> to create a std::string according the
// Deliminator.
auto
join(const std::string& sDelimitor, const std::vector<std::string>& sSource)
  -> std::string;
auto
join(const std::string& sDelimitor,
	 std::vector<std::string>::const_iterator begin,
	 std::vector<std::string>::const_iterator end) -> std::string;

auto
luajoin(const std::string& sDelimitor, const std::vector<std::string>& sSource)
  -> std::string;
auto
luajoin(const std::string& sDelimitor,
		std::vector<std::string>::const_iterator begin,
		std::vector<std::string>::const_iterator end) -> std::string;

// These methods escapes a string for saving in a .sm or .crs file
auto
SmEscape(const std::string& sUnescaped) -> std::string;
auto
SmEscape(const char* cUnescaped, int len) -> std::string;

// These methods "escape" a string for .dwi by turning = into -, ] into I,
// etc. That is "lossy".
auto
DwiEscape(const std::string& sUnescaped) -> std::string;
auto
DwiEscape(const char* cUnescaped, int len) -> std::string;

auto
GetCwd() -> std::string;

void
SetCommandlineArguments(int argc, char** argv);
void
GetCommandLineArguments(int& argc, char**& argv);
auto
GetCommandlineArgument(const std::string& option,
					   std::string* argument = nullptr,
					   int iIndex = 0) -> bool;
extern int g_argc;
extern char** g_argv;

void
CRC32(unsigned int& iCRC, const void* pBuffer, size_t iSize);
auto
GetHashForString(const std::string& s) -> unsigned int;
auto
GetHashForFile(const std::string& sPath) -> unsigned int;
auto
GetHashForDirectory(const std::string& sDir)
  -> unsigned int; // a hash value that remains the
				   // same as long as nothing in the
				   // directory has changed
auto
DirectoryIsEmpty(const std::string& sPath) -> bool;

auto
CompareStringsAsc(const std::string& sStr1, const std::string& sStr2) -> bool;
void
SortStringArray(std::vector<std::string>& asAddTo, bool bSortAscending = true);

/* Find the mean and standard deviation of all numbers in [start,end). */
auto
calc_mean(const float* pStart, const float* pEnd) -> float;
/* When bSample is true, it calculates the square root of an unbiased
 * estimator for the population variance. Note that this is not an unbiased
 * estimator for the population standard deviation but it is close and an
 * unbiased estimator is complicated (apparently). When the entire
 * population is known, bSample should be false to calculate the exact
 * standard deviation. */
auto
calc_stddev(const float* pStart, const float* pEnd, bool bSample = false)
  -> float;

/* Useful for objects with no operator-, eg. std::map::iterator (more
 * convenient than advance). */
template<class T>
inline auto
Increment(T a) -> T
{
	++a;
	return a;
}
template<class T>
inline auto
Decrement(T a) -> T
{
	--a;
	return a;
}

void
TrimLeft(std::string& sStr, const char* szTrim = "\r\n\t ");
void
TrimRight(std::string& sStr, const char* szTrim = "\r\n\t ");
void
Trim(std::string& sStr, const char* szTrim = "\r\n\t ");
void
StripCrnl(std::string& sStr);
auto
BeginsWith(const std::string& sTestThis, const std::string& sBeginning) -> bool;
auto
EndsWith(const std::string& sTestThis, const std::string& sEnding) -> bool;
auto
URLEncode(const std::string& sStr) -> std::string;

auto
DerefRedir(const std::string& sPath) -> std::string;
auto
GetFileContents(const std::string& sPath,
				std::string& sOut,
				bool bOneLine = false) -> bool;
auto
GetFileContents(const std::string& sFile, std::vector<std::string>& asOut)
  -> bool;

class Regex
{
  public:
	Regex(const std::string& sPat = "");
	Regex(const Regex& rhs);
	auto operator=(const Regex& rhs) -> Regex&;
	auto operator=(Regex&& rhs) noexcept -> Regex&;
	~Regex();
	[[nodiscard]] auto IsSet() const -> bool { return !m_sPattern.empty(); }
	void Set(const std::string& str);
	auto Compare(const std::string& sStr) -> bool;
	auto Compare(const std::string& sStr, std::vector<std::string>& asMatches)
	  -> bool;
	auto Replace(const std::string& sReplacement,
				 const std::string& sSubject,
				 std::string& sOut) -> bool;

  private:
	void Compile();
	void Release();

	void* m_pReg;
	unsigned m_iBackrefs;
	std::string m_sPattern;
};

void
ReplaceEntityText(std::string& sText,
				  const std::map<std::string, std::string>& m);
void
ReplaceEntityText(std::string& sText, const std::map<char, std::string>& m);
void
Replace_Unicode_Markers(std::string& Text);
auto
WcharDisplayText(wchar_t c) -> std::string;

auto
Basename(const std::string& dir) -> std::string;
auto
Dirname(const std::string& dir) -> std::string;
auto
Capitalize(const std::string& s) -> std::string;

#if defined(HAVE_UNISTD_H)
#include <unistd.h> /* correct place with correct definitions */
#endif

extern unsigned char g_UpperCase[256];
extern unsigned char g_LowerCase[256];

/* ASCII-only case insensitivity. */
struct char_traits_char_nocase : public std::char_traits<char>
{
	static auto eq(char c1, char c2) -> bool
	{
		return g_UpperCase[static_cast<unsigned char>(c1)] ==
			   g_UpperCase[static_cast<unsigned char>(c2)];
	}

	static auto ne(char c1, char c2) -> bool
	{
		return g_UpperCase[static_cast<unsigned char>(c1)] !=
			   g_UpperCase[static_cast<unsigned char>(c2)];
	}

	static auto lt(char c1, char c2) -> bool
	{
		return g_UpperCase[static_cast<unsigned char>(c1)] <
			   g_UpperCase[static_cast<unsigned char>(c2)];
	}

	static auto compare(const char* s1, const char* s2, size_t n) -> int
	{
		int ret = 0;
		while ((n--) != 0U) {
			ret = fasttoupper(*s1++) - fasttoupper(*s2++);
			if (ret != 0) {
				break;
			}
		}
		return ret;
	}

	static auto fasttoupper(char a) -> char
	{
		return g_UpperCase[static_cast<unsigned char>(a)];
	}

	static auto find(const char* s, int n, char a) -> const char*
	{
		a = fasttoupper(a);
		while (n-- > 0 && fasttoupper(*s) != a) {
			++s;
		}

		if (fasttoupper(*s) == a) {
			return s;
		}
		return nullptr;
	}
};
using istring = std::basic_string<char, char_traits_char_nocase>;

/* Compatibility/convenience shortcuts. These are actually defined in
 * RageFileManager.h, but declared here since they're used in many places.
 */
void
GetDirListing(const std::string& sPath,
			  std::vector<std::string>& addTo,
			  bool onlydir,
			  bool returnPathToo);

void
GetDirListingRecursive(const std::string& sDir,
					   const std::string& sMatch,
					   std::vector<std::string>& AddTo); /* returns path too */
void
GetDirListingRecursive(RageFileDriver* prfd,
					   const std::string& sDir,
					   const std::string& sMatch,
					   std::vector<std::string>& AddTo); /* returns path too */
auto
DoesFileExist(const std::string& sPath) -> bool;
auto
IsAFile(const std::string& sPath) -> bool;
auto
IsADirectory(const std::string& sPath) -> bool;
auto
GetFileSizeInBytes(const std::string& sFilePath) -> int;

/** @brief Get the first x characters of a string. Allow negative warping.
 *
 * This comes from http://stackoverflow.com/a/7597469/445373
 */
auto
head(std::string const& source, int32_t length) -> std::string;

/** @brief Get the last x characters of a string. Allow negative warping.
 *
 * This comes from http://stackoverflow.com/a/7597469/445373
 */
auto
tail(std::string const& source, int32_t length) -> std::string;

// call FixSlashesInPlace on any path that came from the user
void
FixSlashesInPlace(std::string& sPath);
void
CollapsePath(std::string& sPath, bool bRemoveLeadingDot = false);

/** @brief Utilities for converting the Strings. */
namespace StringConversion {
template<typename T>
auto
FromString(const std::string& sValue, T& out) -> bool;

template<typename T>
auto
ToString(const T& value) -> std::string;

template<>
inline auto
FromString<std::string>(const std::string& sValue, std::string& out) -> bool
{
	out = sValue;
	return true;
}
template<>
inline auto
ToString<std::string>(const std::string& value) -> std::string
{
	return value;
}
} // namespace StringConversion

class RageFileBasic;
auto
FileCopy(const std::string& sSrcFile, const std::string& sDstFile) -> bool;
auto
FileCopy(RageFileBasic& in,
		 RageFileBasic& out,
		 std::string& sError,
		 bool* bReadError = nullptr) -> bool;

template<class T>
void
GetAsNotInBs(const std::vector<T>& as,
			 const std::vector<T>& bs,
			 std::vector<T>& difference)
{
	std::vector<T> bsUnmatched = bs;
	// Cannot use FOREACH_CONST here because std::vector<T>::const_iterator
	// is an implicit type.
	for (typename std::vector<T>::const_iterator a = as.begin(); a != as.end();
		 ++a) {
		typename std::vector<T>::iterator iter =
		  find(bsUnmatched.begin(), bsUnmatched.end(), *a);
		if (iter != bsUnmatched.end())
			bsUnmatched.erase(iter);
		else
			difference.push_back(*a);
	}
}

template<class T>
void
GetConnectsDisconnects(const std::vector<T>& before,
					   const std::vector<T>& after,
					   std::vector<T>& disconnects,
					   std::vector<T>& connects)
{
	GetAsNotInBs(before, after, disconnects);
	GetAsNotInBs(after, before, connects);
}

#endif
