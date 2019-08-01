/** @brief RageUtil - Miscellaneous helper macros and functions. */

#ifndef RAGE_UTIL_H
#define RAGE_UTIL_H

#include <map>
#include <random>
#include <sstream>
#include <vector>
class RageFileDriver;

/** @brief Safely delete pointers. */
#define SAFE_DELETE(p)                                                         \
	do {                                                                       \
		delete (p);                                                            \
		(p) = NULL;                                                            \
	} while (false)
/** @brief Safely delete array pointers. */
#define SAFE_DELETE_ARRAY(p)                                                   \
	do {                                                                       \
		delete[](p);                                                           \
		(p) = NULL;                                                            \
	} while (false)

/** @brief Zero out the memory. */
#define ZERO(x) memset(&(x), 0, sizeof(x))
/** @brief Copy from a to b. */
#define COPY(a, b)                                                             \
	do {                                                                       \
		ASSERT(sizeof(a) == sizeof(b));                                        \
		memcpy(&(a), &(b), sizeof(a));                                         \
	} while (false)
/** @brief Get the length of the array. */
#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))

/* Common harmless mismatches.  All min(T,T) and max(T,T) cases are handled
 * by the generic template we get from <algorithm>. */
inline float
min(float a, int b)
{
	return a < b ? a : b;
}
inline float
min(int a, float b)
{
	return a < b ? a : b;
}
inline float
max(float a, int b)
{
	return a > b ? a : b;
}
inline float
max(int a, float b)
{
	return a > b ? a : b;
}
inline unsigned long
min(unsigned int a, unsigned long b)
{
	return a < b ? a : b;
}
inline unsigned long
min(unsigned long a, unsigned int b)
{
	return a < b ? a : b;
}
inline unsigned long
max(unsigned int a, unsigned long b)
{
	return a > b ? a : b;
}
inline unsigned long
max(unsigned long a, unsigned int b)
{
	return a > b ? a : b;
}

/** @brief If outside the range from low to high, bring it within range. */
#define clamp(val, low, high) (max((low), min((val), (high))))

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
U
lerp(T x, U l, U h)
{
	return static_cast<U>(x * (h - l) + l);
}

template<typename T, typename U, typename V>
bool
CLAMP(T& x, U l, V h)
{
	if (x > static_cast<T>(h)) {
		x = static_cast<T>(h);
		return true;
	} else if (x < static_cast<T>(l)) {
		x = static_cast<T>(l);
		return true;
	}
	return false;
}

template<class T>
bool
ENUM_CLAMP(T& x, T l, T h)
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

inline float
wife2(float maxms, float ts)
{
	maxms = maxms * 1000.f;
	float avedeviation = 95.f * ts;
	float y = 1 - static_cast<float>(
					pow(2, -1 * maxms * maxms / (avedeviation * avedeviation)));
	y = pow(y, 2);
	return (2 - -8) * (1 - y) + -8;
}

inline void
wrap(int& x, int n)
{
	if (x < 0)
		x += ((-x / n) + 1) * n;
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
	if (x < 0)
		x += truncf(((-x / n) + 1)) * n;
	x = fmodf(x, n);
}

inline float
fracf(float f)
{
	return f - truncf(f);
}

template<class T>
void
CircularShift(vector<T>& v, int dist)
{
	for (int i = abs(dist); i > 0; i--) {
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

template<typename Type, typename Ret>
static Ret*
CreateClass()
{
	return new Type;
}

/*
 * Helper function to remove all objects from an STL container for which the
 * Predicate pred is true. If you want to remove all objects for which the
 * predicate returns false, wrap the predicate with not1().
 */
template<typename Container, typename Predicate>
void
RemoveIf(Container& c, Predicate p)
{
	c.erase(remove_if(c.begin(), c.end(), p), c.end());
}
template<typename Container, typename Value>
void
RemoveIfEqual(Container& c, const Value& v)
{
	c.erase(remove(c.begin(), c.end(), v), c.end());
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

	TO& operator*() { return m_ToValue; }
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
ConvertValueHelper<TO, FROM>
ConvertValue(FROM* pValue)
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
static T
enum_add2(T val, int iAmt)
{
	return static_cast<T>(val + iAmt);
}

template<typename T>
static T
enum_cycle(T val, int iMax, int iAmt = 1)
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
inline uint32_t
Swap32(uint32_t n)
{
	return (n >> 24) | ((n >> 8) & 0x0000FF00) | ((n << 8) & 0x00FF0000) |
		   (n << 24);
}

inline uint32_t
Swap24(uint32_t n)
{
	return Swap32(n) >> 8; // xx223344 -> 443322xx -> 00443322
}

inline uint16_t
Swap16(uint16_t n)
{
	return (n >> 8) | (n << 8);
}
#endif

inline uint32_t
Swap32LE(uint32_t n)
{
	return n;
}
inline uint32_t
Swap24LE(uint32_t n)
{
	return n;
}
inline uint16_t
Swap16LE(uint16_t n)
{
	return n;
}
inline uint32_t
Swap32BE(uint32_t n)
{
	return Swap32(n);
}
inline uint32_t
Swap24BE(uint32_t n)
{
	return Swap24(n);
}
inline uint16_t
Swap16BE(uint16_t n)
{
	return Swap16(n);
}

typedef std::mt19937 RandomGen;

extern RandomGen g_RandomNumberGenerator;

void
seed_lua_prng();

inline int
random_up_to(RandomGen& rng, int limit)
{
	RandomGen::result_type res = rng();
	// Cutting off the incomplete [0,n) chunk at the max value makes the result
	// more evenly distributed. -Kyz
	RandomGen::result_type up_to_max =
	  RandomGen::max() - (RandomGen::max() % limit);
	while (res > up_to_max) {
		res = rng();
	}

	return int(res % limit);
}

inline int
random_up_to(int limit)
{
	return random_up_to(g_RandomNumberGenerator, limit);
}

/**
 * @brief Generate a random float between 0 inclusive and 1 exclusive.
 * @return the random float.
 */
inline float
RandomFloat()
{
	return float(g_RandomNumberGenerator() / 4294967296.0);
}

/**
 * @brief Return a float between the low and high values.
 * @param fLow the low value, inclusive.
 * @param fHigh the high value, inclusive.
 * @return the random float.
 */
inline float
RandomFloat(float fLow, float fHigh)
{
	return SCALE(RandomFloat(), 0.0f, 1.0f, fLow, fHigh);
}

// Returns an integer between nLow and nHigh inclusive
inline int
RandomInt(int low, int high)
{
	return random_up_to(g_RandomNumberGenerator, high - low + 1) + low;
}

// Returns an integer between 0 and n-1 inclusive (replacement for rand() % n).
inline int
RandomInt(int n)
{
	return random_up_to(g_RandomNumberGenerator, n);
}

// Simple function for generating random numbers
inline float
randomf(const float low = -1.0f, const float high = 1.0f)
{
	return RandomFloat(low, high);
}

/* return f rounded to the nearest multiple of fRoundInterval */
inline float
Quantize(const float f, const float fRoundInterval)
{
	return static_cast<int>((f + fRoundInterval / 2) / fRoundInterval) *
		   fRoundInterval;
}

inline int
Quantize(const int i, const int iRoundInterval)
{
	return static_cast<int>((i + iRoundInterval / 2) / iRoundInterval) *
		   iRoundInterval;
}

/* return f truncated to the nearest multiple of fTruncInterval */
inline float
ftruncf(const float f, const float fTruncInterval)
{
	return static_cast<int>((f) / fTruncInterval) * fTruncInterval;
}

/* Return i rounded up to the nearest multiple of iInterval. */
inline int
QuantizeUp(int i, int iInterval)
{
	return static_cast<int>((i + iInterval - 1) / iInterval) * iInterval;
}

inline float
QuantizeUp(float i, float iInterval)
{
	return ceilf(i / iInterval) * iInterval;
}

/* Return i rounded down to the nearest multiple of iInterval. */
inline int
QuantizeDown(int i, int iInterval)
{
	return static_cast<int>((i - iInterval + 1) / iInterval) * iInterval;
}

inline float
QuantizeDown(float i, float iInterval)
{
	return floorf(i / iInterval) * iInterval;
}

// Move val toward other_val by to_move.
void
fapproach(float& val, float other_val, float to_move);

/* Return a positive x mod y. */
float
fmodfp(float x, float y);

int
power_of_two(int input);
bool
IsAnInt(const RString& s);
bool
IsHexVal(const RString& s);
RString
BinaryToHex(const void* pData_, int iNumBytes);
RString
BinaryToHex(const RString& sString);
bool
HexToBinary(const RString& s, unsigned char* stringOut);
bool
HexToBinary(const RString& s, RString* sOut);
float
HHMMSSToSeconds(const RString& sHMS);
RString
SecondsToHHMMSS(float fSecs);
RString
SecondsToMSSMsMs(float fSecs);
RString
SecondsToMMSSMsMs(float fSecs);
RString
SecondsToMMSSMsMsMs(float fSecs);
RString
SecondsToMSS(float fSecs);
RString
SecondsToMMSS(float fSecs);
RString
PrettyPercent(float fNumerator, float fDenominator);
inline RString
PrettyPercent(int fNumerator, int fDenominator)
{
	return PrettyPercent(float(fNumerator), float(fDenominator));
}
RString
Commify(int iNum);
RString
Commify(const RString& num, const RString& sep = ",", const RString& dot = ".");
RString
FormatNumberAndSuffix(int i);

struct tm
GetLocalTime();

template<typename... Args>
RString
ssprintf(const char* format, Args... args)
{
	return ssprintf(std::string(format), args...);
}

template<typename... Args>
RString
ssprintf(const std::string& format, Args... args)
{
	// lifted without shame from stack overflow to replace 2002 code with 2011+
	// code
	size_t size =
	  snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
	std::unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, format.c_str(), args...);
	return RString(
	  std::string(buf.get(),
				  buf.get() + size - 1)); // We don't want the '\0' inside
}
RString
vssprintf(const char* fmt, va_list argList);
RString
ConvertI64FormatString(const RString& sStr);

/*
 * Splits a Path into 4 parts (Directory, Drive, Filename, Extention).  Supports
 * UNC path names. If Path is a directory (eg. c:\games\stepmania"), append a
 * slash so the last element will end up in Dir, not FName:
 * "c:\games\stepmania\".
 * */
void
splitpath(const RString& Path, RString& Dir, RString& Filename, RString& Ext);

RString
SetExtension(const RString& path, const RString& ext);
RString
GetExtension(const RString& sPath);
RString
GetFileNameWithoutExtension(const RString& sPath);
void
MakeValidFilename(RString& sName);

bool
FindFirstFilenameContaining(const vector<RString>& filenames,
							RString& out,
							const vector<RString>& starts_with,
							const vector<RString>& contains,
							const vector<RString>& ends_with);

extern const wchar_t INVALID_CHAR;

int
utf8_get_char_len(char p);
bool
utf8_to_wchar(const char* s, size_t iLength, unsigned& start, wchar_t& ch);
bool
utf8_to_wchar_ec(const RString& s, unsigned& start, wchar_t& ch);
void
wchar_to_utf8(wchar_t ch, RString& out);
wchar_t
utf8_get_char(const RString& s);
bool
utf8_is_valid(const RString& s);
void
utf8_remove_bom(RString& s);
void
MakeUpper(char* p, size_t iLen);
void
MakeLower(char* p, size_t iLen);
void
MakeUpper(wchar_t* p, size_t iLen);
void
MakeLower(wchar_t* p, size_t iLen);
/**
 * @brief Have a standard way of converting Strings to integers.
 * @param sString the string to convert.
 * @return the integer we are after. */
int
StringToInt(const RString& sString);
/**
 * @brief Have a standard way of converting integers to Strings.
 * @param iNum the integer to convert.
 * @return the string we are after. */
RString
IntToString(const int& iNum);
float
StringToFloat(const RString& sString);
RString
FloatToString(const float& num);
bool
StringToFloat(const RString& sString, float& fOut);
// Better than IntToString because you can check for success.
template<class T>
bool
operator>>(const RString& lhs, T& rhs)
{
	return !!(istringstream(lhs) >> rhs);
}

RString
WStringToRString(const wstring& sString);
RString
WcharToUTF8(wchar_t c);
wstring
RStringToWstring(const RString& sString);

struct LanguageInfo
{
	const char* szIsoCode;
	const char* szEnglishName;
};
void
GetLanguageInfos(vector<const LanguageInfo*>& vAddTo);
const LanguageInfo*
GetLanguageInfo(const RString& sIsoCode);
RString
GetLanguageNameFromISO639Code(const RString& sName);

// Splits a RString into an vector<RString> according the Delimitor.
void
split(const RString& sSource,
	  const RString& sDelimitor,
	  vector<RString>& asAddIt,
	  const bool bIgnoreEmpty = true);
void
split(const wstring& sSource,
	  const wstring& sDelimitor,
	  vector<wstring>& asAddIt,
	  const bool bIgnoreEmpty = true);

/* In-place split. */
void
split(const RString& sSource,
	  const RString& sDelimitor,
	  int& iBegin,
	  int& iSize,
	  bool bIgnoreEmpty = true);
void
split(const wstring& sSource,
	  const wstring& sDelimitor,
	  int& iBegin,
	  int& iSize,
	  bool bIgnoreEmpty = true);

/* In-place split of partial string. */
void
split(const RString& sSource,
	  const RString& sDelimitor,
	  int& iBegin,
	  int& iSize,
	  int iLen,
	  bool bIgnoreEmpty); /* no default to avoid ambiguity */
void
split(const wstring& sSource,
	  const wstring& sDelimitor,
	  int& iBegin,
	  int& iSize,
	  int iLen,
	  bool bIgnoreEmpty);

// Joins a vector<RString> to create a RString according the Deliminator.
RString
join(const RString& sDelimitor, const vector<RString>& sSource);
RString
join(const RString& sDelimitor,
	 vector<RString>::const_iterator begin,
	 vector<RString>::const_iterator end);

std::string
luajoin(const std::string& sDelimitor, const vector<std::string>& sSource);
std::string
luajoin(const std::string& sDelimitor,
		vector<std::string>::const_iterator begin,
		vector<std::string>::const_iterator end);

// These methods escapes a string for saving in a .sm or .crs file
RString
SmEscape(const RString& sUnescaped);
RString
SmEscape(const char* cUnescaped, int len);

// These methods "escape" a string for .dwi by turning = into -, ] into I, etc.
// That is "lossy".
RString
DwiEscape(const RString& sUnescaped);
RString
DwiEscape(const char* cUnescaped, int len);

RString
GetCwd();

void
SetCommandlineArguments(int argc, char** argv);
void
GetCommandLineArguments(int& argc, char**& argv);
bool
GetCommandlineArgument(const RString& option,
					   RString* argument = nullptr,
					   int iIndex = 0);
extern int g_argc;
extern char** g_argv;

void
CRC32(unsigned int& iCRC, const void* pBuffer, size_t iSize);
unsigned int
GetHashForString(const RString& s);
unsigned int
GetHashForFile(const RString& sPath);
unsigned int
GetHashForDirectory(const RString& sDir); // a hash value that remains the same
										  // as long as nothing in the directory
										  // has changed
bool
DirectoryIsEmpty(const RString& sPath);

bool
CompareRStringsAsc(const RString& sStr1, const RString& sStr2);
bool
CompareRStringsDesc(const RString& sStr1, const RString& sStr2);
void
SortRStringArray(vector<RString>& asAddTo, const bool bSortAscending = true);

/* Find the mean and standard deviation of all numbers in [start,end). */
float
calc_mean(const float* pStart, const float* pEnd);
/* When bSample is true, it calculates the square root of an unbiased estimator
 * for the population variance. Note that this is not an unbiased estimator for
 * the population standard deviation but it is close and an unbiased estimator
 * is complicated (apparently). When the entire population is known, bSample
 * should be false to calculate the exact standard deviation. */
float
calc_stddev(const float* pStart, const float* pEnd, bool bSample = false);

/*
 * Find the slope, intercept, and error of a linear least squares regression
 * of the points given.  Error is returned as the sqrt of the average squared
 * Y distance from the chosen line.
 * Returns true on success, false on failure.
 */
bool
CalcLeastSquares(const vector<pair<float, float>>& vCoordinates,
				 float& fSlope,
				 float& fIntercept,
				 float& fError);

/*
 * This method throws away any points that are more than fCutoff away from
 * the line defined by fSlope and fIntercept.
 */
void
FilterHighErrorPoints(vector<pair<float, float>>& vCoordinates,
					  float fSlope,
					  float fIntercept,
					  float fCutoff);

template<class T1, class T2>
int
FindIndex(T1 begin, T1 end, const T2* p)
{
	T1 iter = find(begin, end, p);
	if (iter == end)
		return -1;
	return iter - begin;
}

/* Useful for objects with no operator-, eg. map::iterator (more convenient than
 * advance). */
template<class T>
T
Increment(T a)
{
	++a;
	return a;
}
template<class T>
T
Decrement(T a)
{
	--a;
	return a;
}

void
TrimLeft(RString& sStr, const char* szTrim = "\r\n\t ");
void
TrimRight(RString& sStr, const char* szTrim = "\r\n\t ");
void
Trim(RString& sStr, const char* szTrim = "\r\n\t ");
void
StripCrnl(RString& sStr);
bool
BeginsWith(const RString& sTestThis, const RString& sBeginning);
bool
EndsWith(const RString& sTestThis, const RString& sEnding);
RString
URLEncode(const RString& sStr);

void
StripCvsAndSvn(
  vector<RString>& vs); // Removes various versioning system metafolders.
void
StripMacResourceForks(vector<RString>& vs); // Removes files starting with "._"

RString
DerefRedir(const RString& sPath);
bool
GetFileContents(const RString& sPath, RString& sOut, bool bOneLine = false);
bool
GetFileContents(const RString& sFile, vector<RString>& asOut);
bool
GetFileContents(const std::string& sPath,
				std::string& sOut,
				bool bOneLine = false);
bool
GetFileContents(const std::string& sFile, vector<std::string>& asOut);

class Regex
{
  public:
	Regex(const RString& sPat = "");
	Regex(const Regex& rhs);
	Regex& operator=(const Regex& rhs);
	~Regex();
	bool IsSet() const { return !m_sPattern.empty(); }
	void Set(const RString& str);
	bool Compare(const RString& sStr);
	bool Compare(const RString& sStr, vector<RString>& asMatches);
	bool Replace(const RString& sReplacement,
				 const RString& sSubject,
				 RString& sOut);

  private:
	void Compile();
	void Release();

	void* m_pReg;
	unsigned m_iBackrefs;
	RString m_sPattern;
};

void
ReplaceEntityText(RString& sText, const map<RString, RString>& m);
void
ReplaceEntityText(RString& sText, const map<char, RString>& m);
void
ReplaceEntityText(std::string& sText, const map<std::string, std::string>& m);
void
ReplaceEntityText(std::string& sText, const map<char, std::string>& m);
void
Replace_Unicode_Markers(RString& Text);
RString
WcharDisplayText(wchar_t c);

RString
Basename(const RString& dir);
RString
Dirname(const RString& dir);
RString
Capitalize(const RString& s);

#if defined(HAVE_UNISTD_H)
#include <unistd.h> /* correct place with correct definitions */
#endif

extern unsigned char g_UpperCase[256];
extern unsigned char g_LowerCase[256];

/* ASCII-only case insensitivity. */
struct char_traits_char_nocase : public char_traits<char>
{
	static bool eq(char c1, char c2)
	{
		return g_UpperCase[static_cast<unsigned char>(c1)] ==
			   g_UpperCase[static_cast<unsigned char>(c2)];
	}

	static bool ne(char c1, char c2)
	{
		return g_UpperCase[static_cast<unsigned char>(c1)] !=
			   g_UpperCase[static_cast<unsigned char>(c2)];
	}

	static bool lt(char c1, char c2)
	{
		return g_UpperCase[static_cast<unsigned char>(c1)] <
			   g_UpperCase[static_cast<unsigned char>(c2)];
	}

	static int compare(const char* s1, const char* s2, size_t n)
	{
		int ret = 0;
		while (n--) {
			ret = fasttoupper(*s1++) - fasttoupper(*s2++);
			if (ret != 0)
				break;
		}
		return ret;
	}

	static char fasttoupper(char a)
	{
		return g_UpperCase[static_cast<unsigned char>(a)];
	}

	static const char* find(const char* s, int n, char a)
	{
		a = fasttoupper(a);
		while (n-- > 0 && fasttoupper(*s) != a)
			++s;

		if (fasttoupper(*s) == a)
			return s;
		return nullptr;
	}
};
typedef basic_string<char, char_traits_char_nocase> istring;

/* Compatibility/convenience shortcuts. These are actually defined in
 * RageFileManager.h, but declared here since they're used in many places. */
void
GetDirListing(const RString& sPath,
			  vector<RString>& AddTo,
			  bool bOnlyDirs = false,
			  bool bReturnPathToo = false);
void
GetDirListingRecursive(const RString& sDir,
					   const RString& sMatch,
					   vector<RString>& AddTo); /* returns path too */
void
GetDirListingRecursive(RageFileDriver* prfd,
					   const RString& sDir,
					   const RString& sMatch,
					   vector<RString>& AddTo); /* returns path too */
bool
DeleteRecursive(
  const RString& sDir); /* delete the dir and all files/subdirs inside it */
bool
DeleteRecursive(
  RageFileDriver* prfd,
  const RString& sDir); /* delete the dir and all files/subdirs inside it */
bool
DoesFileExist(const RString& sPath);
bool
IsAFile(const RString& sPath);
bool
IsADirectory(const RString& sPath);
int
GetFileSizeInBytes(const RString& sFilePath);

// call FixSlashesInPlace on any path that came from the user
void
FixSlashesInPlace(RString& sPath);
void
CollapsePath(RString& sPath, bool bRemoveLeadingDot = false);

/** @brief Utilities for converting the RStrings. */
namespace StringConversion {
template<typename T>
bool
FromString(const RString& sValue, T& out);

template<typename T>
RString
ToString(const T& value);

template<>
inline bool
FromString<RString>(const RString& sValue, RString& out)
{
	out = sValue;
	return true;
}
template<>
inline RString
ToString<RString>(const RString& value)
{
	return value;
}
}

class RageFileBasic;
bool
FileCopy(const RString& sSrcFile, const RString& sDstFile);
bool
FileCopy(RageFileBasic& in,
		 RageFileBasic& out,
		 RString& sError,
		 bool* bReadError = nullptr);

template<class T>
void
GetAsNotInBs(const vector<T>& as, const vector<T>& bs, vector<T>& difference)
{
	vector<T> bsUnmatched = bs;
	// Cannot use FOREACH_CONST here because vector<T>::const_iterator is an
	// implicit type.
	for (typename vector<T>::const_iterator a = as.begin(); a != as.end();
		 ++a) {
		typename vector<T>::iterator iter =
		  find(bsUnmatched.begin(), bsUnmatched.end(), *a);
		if (iter != bsUnmatched.end())
			bsUnmatched.erase(iter);
		else
			difference.push_back(*a);
	}
}

template<class T>
void
GetConnectsDisconnects(const vector<T>& before,
					   const vector<T>& after,
					   vector<T>& disconnects,
					   vector<T>& connects)
{
	GetAsNotInBs(before, after, disconnects);
	GetAsNotInBs(after, before, connects);
}

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
