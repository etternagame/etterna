//
// Copyright (c) 2012 Artyom Beilis (Tonkikh)
// Copyright (c) 2020-2022 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#define NOWIDE_SOURCE

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#elif defined(__MINGW32__) && defined(__STRICT_ANSI__)
// Need the _w* functions which are extensions on MinGW but not on MinGW-w64
#include <_mingw.h>
#ifndef __MINGW64_VERSION_MAJOR
#undef __STRICT_ANSI__
#endif
#elif defined(__CYGWIN__) && !defined(_GNU_SOURCE)
// The setenv family of functions is an extension on Cygwin
#define _GNU_SOURCE 1
#endif

#include <nowide/cstdlib.hpp>

#ifndef NOWIDE_WINDOWS
namespace nowide {
    int setenv(const char* key, const char* value, int overwrite)
    {
        return ::setenv(key, value, overwrite);
    }

    int unsetenv(const char* key)
    {
        return ::unsetenv(key);
    }

    int putenv(char* string)
    {
        return ::putenv(string);
    }
} // namespace nowide
#else
#include <nowide/stackstring.hpp>
#include <vector>
#include <windows.h>

namespace {
// thread_local was broken on MinGW for all 32bit compiler releases prior to 11.x, see
// https://sourceforge.net/p/mingw-w64/bugs/527/
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83562
// Using a non-trivial destructor causes program termination on thread exit.
#if defined(__MINGW32__) && !defined(__MINGW64__) && !defined(__clang__) && (__GNUC__ < 11)
class stackstring_for_thread
{
    union
    {
        nowide::stackstring s_;
    };

public:
    stackstring_for_thread() : s_(){};
    // Empty destructor so the union member (using a non-trivial destructor) does not get destroyed.
    // This will leak memory if any is allocated by the stackstring for each terminated thread
    // but as most values fit into the stack buffer this is rare and still better than a crash.
    ~stackstring_for_thread(){};
    void convert(const wchar_t* begin, const wchar_t* end)
    {
        s_.convert(begin, end);
    }

    char* get()
    {
        return s_.get();
    }
};
#else
using stackstring_for_thread = nowide::stackstring;
#endif

} // namespace

namespace nowide {
    char* getenv(const char* key)
    {
        thread_local stackstring_for_thread value;

        const wshort_stackstring name(key);

        static const size_t buf_size = 64;
        wchar_t buf[buf_size];
        std::vector<wchar_t> tmp;
        wchar_t* ptr = buf;
        size_t n = GetEnvironmentVariableW(name.get(), buf, buf_size);
        if(n == 0 && GetLastError() == 203) // ERROR_ENVVAR_NOT_FOUND
            return 0;
        if(n >= buf_size)
        {
            tmp.resize(n + 1, L'\0');
            n = GetEnvironmentVariableW(name.get(), &tmp[0], static_cast<unsigned>(tmp.size() - 1));
            // The size may have changed
            if(n >= tmp.size() - 1)
                return 0;
            ptr = &tmp[0];
        }
        value.convert(ptr, ptr + n);
        return value.get();
    }

    int setenv(const char* key, const char* value, int overwrite)
    {
        const wshort_stackstring name(key);
        if(!overwrite)
        {
            wchar_t unused[2];
            if(GetEnvironmentVariableW(name.get(), unused, 2) != 0 || GetLastError() != 203) // ERROR_ENVVAR_NOT_FOUND
                return 0;
        }
        const wstackstring wval(value);
        if(SetEnvironmentVariableW(name.get(), wval.get()))
            return 0;
        return -1;
    }

    int unsetenv(const char* key)
    {
        const wshort_stackstring name(key);
        if(SetEnvironmentVariableW(name.get(), 0))
            return 0;
        return -1;
    }

    int putenv(char* string)
    {
        const char* key = string;
        const char* key_end = string;
        while(*key_end != '=' && *key_end != '\0')
            key_end++;
        if(*key_end == '\0')
            return -1;
        const wshort_stackstring wkey(key, key_end);
        const wstackstring wvalue(key_end + 1);

        if(SetEnvironmentVariableW(wkey.get(), wvalue.get()))
            return 0;
        return -1;
    }

    int system(const char* cmd)
    {
        if(!cmd)
            return _wsystem(0);
        const wstackstring wcmd(cmd);
        return _wsystem(wcmd.get());
    }
} // namespace nowide
#endif
