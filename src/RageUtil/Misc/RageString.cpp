#include "Etterna/Globals/global.h"
#include "RageString.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <cstring>

void
make_upper(char* p, size_t len);
void
make_lower(char* p, size_t len);

std::string
Rage::join(std::string const& delimiter, std::vector<std::string> const& source)
{
	if (source.empty()) {
		return "";
	}
	return Rage::join(delimiter, source.begin(), source.end());
}

std::string
Rage::join(std::string const& delimiter,
		   std::vector<std::string>::const_iterator start,
		   std::vector<std::string>::const_iterator finish)
{
	if (start == finish) {
		return "";
	}
	std::stringstream builder;

	auto append = [&builder, &delimiter](std::string const& target) {
		builder << target;
		builder << delimiter;
	};
	auto inclusive = finish - 1;
	std::for_each(start, inclusive, append);

	builder << *inclusive;
	return builder.str();
}

template<class S>
static int
DelimitorLength(S const& delimitor)
{
	return delimitor.size();
}

static int
DelimitorLength(char delimitor)
{
	return 1;
}

static int
DelimitorLength(wchar_t delimitor)
{
	return 1;
}

template<class S, class C>
std::vector<S>
do_split(S const& source,
		 C const delimitor,
		 Rage::EmptyEntries const shouldIgnore)
{
	std::vector<S> result;
	/* Short-circuit if the source is empty; we want to return an empty vector
	 * if the string is empty, even if bIgnoreEmpty is true. */
	if (source.empty()) {
		return result;
	}
	size_t startpos = 0;

	do {
		size_t pos{ source.find(delimitor, startpos) };
		if (pos == source.npos) {
			pos = source.size();
		}
		if (pos - startpos > 0 || shouldIgnore == Rage::EmptyEntries::include) {
			/* Optimization: if we're copying the whole string, avoid substr;
			 * this allows this copy to be refcounted, which is much faster. */
			if (startpos == 0 && pos - startpos == source.size()) {
				result.push_back(source);
			} else {
				S const target{ source.substr(startpos, pos - startpos) };
				result.push_back(target);
			}
		}

		startpos = pos + DelimitorLength(delimitor);
	} while (startpos <= source.size());

	return result;
}

std::vector<std::string>
Rage::split(std::string const& source, std::string const& delimiter)
{
	return Rage::split(source, delimiter, Rage::EmptyEntries::skip);
}
std::vector<std::string>
Rage::split(std::string const& source,
			std::string const& delimiter,
			Rage::EmptyEntries shouldIgnoreEmptyEntries)
{
	if (delimiter.size() == 1) {
		// TODO: Look into an optimized character string tokenizer. Perhaps
		// std::getline?
		return do_split(source, delimiter[0], shouldIgnoreEmptyEntries);
	}
	return do_split(source, delimiter, shouldIgnoreEmptyEntries);
}

std::vector<std::wstring>
Rage::split(std::wstring const& source, std::wstring const& delimiter)
{
	return Rage::split(source, delimiter, Rage::EmptyEntries::include);
}
std::vector<std::wstring>
Rage::split(std::wstring const& source,
			std::wstring const& delimiter,
			Rage::EmptyEntries shouldIgnoreEmptyEntries)
{
	if (delimiter.size() == 1) {
		return do_split(source, delimiter[0], shouldIgnoreEmptyEntries);
	}
	return do_split(source, delimiter, shouldIgnoreEmptyEntries);
}

template<class S>
void
do_split(S const& source,
		 S const& delimitor,
		 int& start,
		 int& size,
		 int len,
		 Rage::EmptyEntries const shouldIgnore)
{
	using std::min;
	if (size != -1) {
		// Start points to the beginning of the last delimiter. Move it up.
		start += size + delimitor.size();
		start = min(start, len);
	}

	size = 0;

	if (shouldIgnore == Rage::EmptyEntries::skip) {
		while (start + delimitor.size() < source.size() &&
			   !source.compare(start, delimitor.size(), delimitor)) {
			++start;
		}
	}

	/* Where's the string function to find within a substring?
	 * C++ strings apparently are missing that ... */
	size_t pos;
	if (delimitor.size() == 1) {
		pos = source.find(delimitor[0], start);
	} else {
		pos = source.find(delimitor, start);
	}
	if (pos == source.npos || static_cast<int>(pos) > len) {
		pos = len;
	}
	size = pos - start;
}

void
Rage::split_in_place(std::string const& source,
					 std::string const& delimitor,
					 int& start,
					 int& size)
{
	do_split(
	  source, delimitor, start, size, source.size(), Rage::EmptyEntries::skip);
}

void
Rage::split_in_place(std::string const& source,
					 std::string const& delimitor,
					 int& start,
					 int& size,
					 Rage::EmptyEntries shouldIgnore)
{
	do_split(source, delimitor, start, size, source.size(), shouldIgnore);
}

void
Rage::split_in_place(std::wstring const& source,
					 std::wstring const& delimitor,
					 int& start,
					 int& size)
{
	do_split(
	  source, delimitor, start, size, source.size(), Rage::EmptyEntries::skip);
}

void
Rage::split_in_place(std::wstring const& source,
					 std::wstring const& delimitor,
					 int& start,
					 int& size,
					 Rage::EmptyEntries shouldIgnore)
{
	do_split(source, delimitor, start, size, source.size(), shouldIgnore);
}

void
Rage::split_in_place(std::string const& source,
					 std::string const& delimitor,
					 int& start,
					 int& size,
					 int len)
{
	do_split(source, delimitor, start, size, len, Rage::EmptyEntries::skip);
}

void
Rage::split_in_place(std::string const& source,
					 std::string const& delimitor,
					 int& start,
					 int& size,
					 int len,
					 Rage::EmptyEntries shouldIgnore)
{
	do_split(source, delimitor, start, size, len, shouldIgnore);
}

void
Rage::split_in_place(std::wstring const& source,
					 std::wstring const& delimitor,
					 int& start,
					 int& size,
					 int len)
{
	do_split(source, delimitor, start, size, len, Rage::EmptyEntries::skip);
}

void
Rage::split_in_place(std::wstring const& source,
					 std::wstring const& delimitor,
					 int& start,
					 int& size,
					 int len,
					 Rage::EmptyEntries shouldIgnore)
{
	do_split(source, delimitor, start, size, len, shouldIgnore);
}

std::string
Rage::trim_left(std::string const& source)
{
	return Rage::trim_left(source, "\r\n\t ");
}

std::string
Rage::trim_left(std::string const& source, std::string const& delimiters)
{
	size_t n = 0;
	auto end = source.size();
	auto const* d_str = delimiters.c_str();
	while (n < end && std::strchr(d_str, source[n])) {
		++n;
	}
	return source.substr(n);
}

std::string
Rage::trim_right(std::string const& source)
{
	return Rage::trim_right(source, "\r\n\t ");
}

std::string
Rage::trim_right(std::string const& source, std::string const& delimiters)
{
	int n = source.size();
	auto const* d_str = delimiters.c_str();
	while (n > 0 && std::strchr(d_str, source[n - 1])) {
		n--;
	}

	return source.substr(0, n);
}

std::string
Rage::trim(std::string const& source)
{
	return Rage::trim(source, "\r\n\t ");
}

std::string
Rage::trim(std::string const& source, std::string const& delimiters)
{
	std::string::size_type start = 0;
	auto lastPos = source.size();
	auto const* d_str = delimiters.c_str();
	while (start < lastPos && std::strchr(d_str, source[start])) {
		++start;
	}
	while (start < lastPos && std::strchr(d_str, source[lastPos - 1])) {
		--lastPos;
	}
	return source.substr(start, lastPos - start);
}

std::string
Rage::base_name(std::string const& dir)
{
	auto iEnd = dir.find_last_not_of("/\\");
	if (iEnd == dir.npos) {
		return "";
	}
	auto iStart = dir.find_last_of("/\\", iEnd);
	if (iStart == dir.npos) {
		iStart = 0;
	} else {
		++iStart;
	}
	return dir.substr(iStart, iEnd - iStart + 1);
}

std::string
Rage::dir_name(std::string const& dir)
{
	// Special case: "/" -> "/".
	if (dir.size() == 1 && dir[0] == '/') {
		return "/";
	}
	int pos = dir.size() - 1;
	// Skip trailing slashes.
	while (pos >= 0 && dir[pos] == '/') {
		--pos;
	}
	// Skip the last component.
	while (pos >= 0 && dir[pos] != '/') {
		--pos;
	}
	if (pos < 0) {
		return "./";
	}
	return dir.substr(0, pos + 1);
}

/* Branch optimizations: */
#if defined(__GNUC__) || defined(__clang__)
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif
