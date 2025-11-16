// Copyright (c) 2012 Artyom Beilis (Tonkikh)
// Copyright (c) 2020-2021 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#define NOWIDE_SOURCE
#include <nowide/iostream.hpp>

#ifndef NOWIDE_WINDOWS

namespace nowide {
    // LCOV_EXCL_START
    /// Avoid empty compilation unit warnings
    /// \internal
    NOWIDE_DECL void dummy_exported_function()
    {}
    // LCOV_EXCL_STOP
} // namespace nowide

#else

#include "console_buffer.hpp"
#include <cassert>
#include <iostream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace nowide {
    namespace detail {

        namespace {
            bool is_atty_handle(HANDLE h)
            {
                DWORD dummy;
                return h && GetConsoleMode(h, &dummy) != FALSE;
            }
        } // namespace

        class console_output_buffer final : public console_output_buffer_base
        {
            HANDLE handle_;

        public:
            explicit console_output_buffer(HANDLE handle) : handle_(handle)
            {}

        protected:
            bool
            do_write(const wchar_t* buffer, std::size_t num_chars_to_write, std::size_t& num_chars_written) override
            {
                DWORD size = 0;
                const bool result =
                  WriteConsoleW(handle_, buffer, static_cast<DWORD>(num_chars_to_write), &size, 0) != 0;
                num_chars_written = size;
                return result;
            }
        };

        class console_input_buffer final : public console_input_buffer_base
        {
            HANDLE handle_;

        public:
            explicit console_input_buffer(HANDLE handle) : handle_(handle)
            {}

        protected:
            bool do_read(wchar_t* buffer, std::size_t num_chars_to_read, std::size_t& num_chars_read) override
            {
                DWORD size = 0;
                const auto to_read_size = static_cast<DWORD>(num_chars_to_read);
                const bool result = ReadConsoleW(handle_, buffer, to_read_size, &size, 0) != 0;
                num_chars_read = size;
                return result;
            }
        };

        winconsole_ostream::winconsole_ostream(const target_stream target,
                                               const bool isBuffered,
                                               winconsole_ostream* tieStream) :
            std::ostream(nullptr)
        {
            const HANDLE h = GetStdHandle(target == target_stream::output ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);

            if(is_atty_handle(h))
            {
                d.reset(new console_output_buffer(h));
                rdbuf(d.get());
            } else
            {
                switch(target)
                {
                case target_stream::error: rdbuf(std::cerr.rdbuf()); break;
                case target_stream::log: rdbuf(std::clog.rdbuf()); break;
                case target_stream::output: rdbuf(std::cout.rdbuf()); break;
                }
            }
            assert(rdbuf());

            if(tieStream)
                tie(tieStream);
            if(!isBuffered)
                setf(ios_base::unitbuf);
        }
        winconsole_ostream::~winconsole_ostream()
        {
            try
            {
                flush();
            } catch(...)
            {} // LCOV_EXCL_LINE
        }

        winconsole_istream::winconsole_istream(winconsole_ostream* tieStream) : std::istream(0)
        {
            HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
            if(is_atty_handle(h))
            {
                d.reset(new console_input_buffer(h));
                std::istream::rdbuf(d.get());
            } else
            {
                std::istream::rdbuf(std::cin.rdbuf());
                assert(rdbuf());
            }
            if(tieStream)
                tie(tieStream);
        }

        winconsole_istream::~winconsole_istream()
        {}

    } // namespace detail

// Make sure those are initialized as early as possible
#ifdef NOWIDE_MSVC
#pragma warning(disable : 4073)
#pragma init_seg(lib)
#endif
#ifdef NOWIDE_HAS_INIT_PRIORITY
#define NOWIDE_INIT_PRIORITY __attribute__((init_priority(101)))
#else
#define NOWIDE_INIT_PRIORITY
#endif
    using target_stream = detail::winconsole_ostream::target_stream;
    detail::winconsole_ostream cout NOWIDE_INIT_PRIORITY(target_stream::output, true, nullptr);
    detail::winconsole_istream cin NOWIDE_INIT_PRIORITY(&cout);
    detail::winconsole_ostream cerr NOWIDE_INIT_PRIORITY(target_stream::error, false, &cout);
    detail::winconsole_ostream clog NOWIDE_INIT_PRIORITY(target_stream::log, true, nullptr);
} // namespace nowide

#endif
