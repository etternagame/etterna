//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//  Copyright (c) 2019 Alexander Grund
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <nowide/cstdio.hpp>

#include <nowide/convert.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "test.hpp"

bool file_exists(const std::string& filename)
{
#ifdef NOWIDE_WINDOWS
    FILE* f = nowide::detail::wfopen(nowide::widen(filename).c_str(), L"r");
#else
    FILE* f = std::fopen(filename.c_str(), "r");
#endif
    bool result = false;
    if(f)
    {
        std::fclose(f);
        result = true;
    }
    return result;
}

void create_test_file(const std::string& filename)
{
#ifdef NOWIDE_WINDOWS
    FILE* f = nowide::detail::wfopen(nowide::widen(filename).c_str(), L"w");
#else
    FILE* f = std::fopen(filename.c_str(), "w");
#endif
    TEST(f);
    TEST(std::fputs("test\n", f) >= 0);
    std::fclose(f);
}

#if NOWIDE_MSVC
#include <crtdbg.h> // For _CrtSetReportMode
void noop_invalid_param_handler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned, uintptr_t)
{}
#endif

void test_main(int, char** argv, char**)
{
    const std::string prefix = argv[0];
    const std::string filename = prefix + "\xd7\xa9-\xd0\xbc-\xce\xbd.txt";
#if NOWIDE_MSVC
    // Prevent abort on freopen(NULL, ...)
    _set_invalid_parameter_handler(noop_invalid_param_handler);
#endif

    std::cout << " -- fopen - existing file" << std::endl;
    {
        create_test_file(filename);
        FILE* f = nowide::fopen(filename.c_str(), "r");
        TEST(f);
        char buf[16];
        TEST(std::fgets(buf, 16, f) != 0);
        TEST(strcmp(buf, "test\n") == 0);
        std::fclose(f);
    }
    std::cout << " -- remove" << std::endl;
    {
        create_test_file(filename);
        TEST(file_exists(filename));
        TEST(nowide::remove(filename.c_str()) == 0);
        TEST(!file_exists(filename));
    }
    std::cout << " -- fopen non-existing file" << std::endl;
    {
        nowide::remove(filename.c_str());
        TEST(!file_exists(filename));
        TEST(nowide::fopen(filename.c_str(), "r") == NULL);
        TEST(!file_exists(filename));
    }
    std::cout << " -- freopen" << std::endl;
    {
        create_test_file(filename);
        FILE* f = nowide::fopen(filename.c_str(), "r+");
        TEST(f);
        std::cout << " -- Can read & write" << std::endl;
        {
            char buf[32];
            TEST(std::fgets(buf, 32, f) != 0);
            TEST(strcmp(buf, "test\n") == 0);
            TEST(std::fseek(f, 0, SEEK_END) == 0);
            TEST(std::fputs("foobar\n", f) >= 0);
        }
        // Reopen in read mode
        // Note that changing the mode is not possibly on all implementations
        // E.g. MSVC disallows NULL completely as the file parameter
        FILE* f2 = nowide::freopen(NULL, "r", f);
        if(!f2)
            f2 = nowide::freopen(filename.c_str(), "r", f);
        std::cout << " -- no write possible" << std::endl;
        {
            TEST(f2 == f);
            TEST(std::fputs("not-written\n", f) < 0);
            TEST(std::fseek(f, 0, SEEK_SET) == 0);
            char buf[32];
            TEST(std::fgets(buf, 32, f) != 0);
            TEST(strcmp(buf, "test\n") == 0);
            TEST(std::fgets(buf, 32, f) != 0);
            TEST(strcmp(buf, "foobar\n") == 0);
        }
        std::cout << " -- Reopen different file" << std::endl;
        const std::string filename2 = filename + ".1.txt";
        TEST(nowide::freopen(filename2.c_str(), "w", f) == f);
        {
            char buf[32];
            TEST(std::fputs("baz\n", f) >= 0);
            std::fclose(f);
            f = nowide::fopen(filename2.c_str(), "r");
            TEST(f);
            TEST(std::fgets(buf, 32, f) != 0);
            TEST(strcmp(buf, "baz\n") == 0);
        }
        std::fclose(f);
        nowide::remove(filename2.c_str());
    }
    std::cout << " -- rename" << std::endl;
    {
        create_test_file(filename);
        const std::string filename2 = filename + ".1.txt";
        nowide::remove(filename2.c_str());
        TEST(file_exists(filename));
        TEST(!file_exists(filename2));
        TEST(nowide::rename(filename.c_str(), filename2.c_str()) == 0);
        TEST(!file_exists(filename));
        TEST(file_exists(filename2));
        TEST(nowide::remove(filename.c_str()) < 0);
        TEST(nowide::remove(filename2.c_str()) == 0);
    }
}
