//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <nowide/cstdlib.hpp>

#include <nowide/args.hpp>
#include <nowide/detail/convert.hpp>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "test.hpp"

bool is_ascii(const std::string& s)
{
    for(std::string::const_iterator it = s.begin(); it != s.end(); ++it)
    {
        if(static_cast<unsigned char>(*it) > 0x7F)
            return false;
    }
    return true;
}

std::string replace_non_ascii(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    namespace utf = nowide::detail::utf;
    typedef utf::utf_traits<char> utf8;
    std::string result;
    result.reserve(s.size());
    while(it != s.end())
    {
        utf::code_point c = utf8::decode(it, s.end());
        TEST(c != utf::illegal && c != utf::incomplete);
        if(c > 0x7F)
            c = '?'; // WinAPI seems to do this
        result.push_back(static_cast<char>(c));
    }
    return result;
}

void compare_string_arrays(char** main_val, char** utf8_val, bool sort)
{
    std::vector<std::string> vec_main, vec_utf8;
    for(; *main_val; ++main_val)
        vec_main.push_back(std::string(*main_val));
    for(; *utf8_val; ++utf8_val)
        vec_utf8.push_back(std::string(*utf8_val));
    // Same number of strings
    TEST_EQ(vec_main.size(), vec_utf8.size());
    if(sort)
    {
        // Order doesn't matter
        std::sort(vec_main.begin(), vec_main.end());
        std::sort(vec_utf8.begin(), vec_utf8.end());
    }
    for(size_t i = 0; i < vec_main.size(); ++i)
    {
        // Skip strings with non-ascii chars
        if(is_ascii(vec_main[i]) && vec_main[i] != vec_utf8[i])
            TEST_EQ(vec_main[i], replace_non_ascii(vec_utf8[i]));
    }
}

void compare_getenv(char** env)
{
    // For all all variables in env check against getenv
    for(char** e = env; *e != 0; e++)
    {
        const char* key_begin = *e;
        const char* key_end = strchr(key_begin, '=');
        TEST(key_end);
        std::string key = std::string(key_begin, key_end);
        const char* std_value = std::getenv(key.c_str());
        const char* bnw_value = nowide::getenv(key.c_str());
        // If std_value is set, bnw value must be too and be equal, else bnw value must be unset too
        if(std_value)
        {
            TEST(bnw_value);
            // Compare only if ascii
            if(is_ascii(std_value) && std::string(std_value) != std::string(bnw_value))
                TEST_EQ(std_value, replace_non_ascii(bnw_value));
        } else
            TEST(!bnw_value);
    }
}

const std::string example = "\xd7\xa9-\xd0\xbc-\xce\xbd";

void run_child(int argc, char** argv, char** env)
{
    // Test arguments
    TEST(argc == 2);
    TEST_EQ(argv[1], example);
    TEST(argv[2] == 0);

    // Test getenv
    TEST(nowide::getenv("NOWIDE_TEST"));
    TEST_EQ(nowide::getenv("NOWIDE_TEST"), example);
    TEST(nowide::getenv("NOWIDE_TEST_NONE") == 0);
    // Empty variables are unreliable on windows, hence skip. E.g. using "set FOO=" unsets FOO
#ifndef NOWIDE_WINDOWS
    TEST(nowide::getenv("NOWIDE_EMPTY"));
    TEST_EQ(nowide::getenv("NOWIDE_EMPTY"), std::string());
#endif // !_WIN32

    // This must be contained in env
    std::string sample = "NOWIDE_TEST=" + example;
    bool found = false;
    for(char** e = env; *e != 0; e++)
    {
        if(*e == sample)
            found = true;
    }
    TEST(found);

    std::cout << "Subprocess ok" << std::endl;
}

void run_parent(const char* exe_path)
{
#if NOWIDE_TEST_USE_NARROW
    TEST(nowide::setenv("NOWIDE_TEST", example.c_str(), 1) == 0);
    TEST(nowide::setenv("NOWIDE_TEST_NONE", example.c_str(), 1) == 0);
    TEST(nowide::unsetenv("NOWIDE_TEST_NONE") == 0);
    TEST(nowide::setenv("NOWIDE_EMPTY", "", 1) == 0);
    TEST(nowide::getenv("NOWIDE_EMPTY"));
    std::string command = "\"";
    command += exe_path;
    command += "\" ";
    command += example;
    TEST(nowide::system(command.c_str()) == 0);
    std::cout << "Parent ok" << std::endl;
#else
    std::wstring envVar = L"NOWIDE_TEST=" + nowide::widen(example);
    TEST(_wputenv(envVar.c_str()) == 0);
    std::wstring wcommand = nowide::widen(exe_path) + L" " + nowide::widen(example);
    TEST(_wsystem(wcommand.c_str()) == 0);
    std::cout << "Wide Parent ok" << std::endl;
#endif
}

void test_main(int argc, char** argv, char** env)
{
    const int old_argc = argc;
    char** old_argv = argv;
    char** old_env = env;
    {
        nowide::args _(argc, argv, env);
        TEST(argc == old_argc);
        std::cout << "Checking arguments" << std::endl;
        compare_string_arrays(old_argv, argv, false);
        std::cout << "Checking env" << std::endl;
        compare_string_arrays(old_env, env, true);
        compare_getenv(env);
    }
    // When `args` is destructed the old values must be restored
    TEST(argc == old_argc);
    TEST(argv == old_argv);
    TEST(env == old_env);

    nowide::args a(argc, argv, env);
    if(argc == 1)
        run_parent(argv[0]);
    else
        run_child(argc, argv, env);
}
