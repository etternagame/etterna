//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//  Copyright (c) 2020 Alexander Grund
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef NOWIDE_CONFIG_HPP_INCLUDED
#define NOWIDE_CONFIG_HPP_INCLUDED

#include <nowide/replacement.hpp>

#if(defined(__WIN32) || defined(_WIN32) || defined(WIN32)) && !defined(__CYGWIN__)
#define NOWIDE_WINDOWS
#endif

#ifdef _MSC_VER
#define NOWIDE_MSVC _MSC_VER
#endif

#if defined(__MINGW64__)
#define NOWIDE_FTELL64 ftello64
#define NOWIDE_FSEEK64 fseeko64
#elif defined(__APPLE__)
#define NOWIDE_FTELL64 ftello
#define NOWIDE_FSEEK64 fseeko
#elif defined(_MSC_VER)
#define NOWIDE_FTELL64 _ftelli64
#define NOWIDE_FSEEK64 _fseeki64
#else
#define NOWIDE_FTELL64 ftell
#define NOWIDE_FSEEK64 fseek
#endif

#ifdef __GNUC__
#define NOWIDE_SYMBOL_VISIBLE __attribute__((__visibility__("default")))
#endif

#ifndef NOWIDE_SYMBOL_VISIBLE
#define NOWIDE_SYMBOL_VISIBLE
#endif

#ifdef NOWIDE_WINDOWS
#define NOWIDE_SYMBOL_EXPORT __declspec(dllexport)
#define NOWIDE_SYMBOL_IMPORT __declspec(dllimport)
#else
#define NOWIDE_SYMBOL_EXPORT NOWIDE_SYMBOL_VISIBLE
#define NOWIDE_SYMBOL_IMPORT
#endif

#if defined(NOWIDE_DYN_LINK)
#ifdef NOWIDE_SOURCE
#define NOWIDE_DECL NOWIDE_SYMBOL_EXPORT
#else
#define NOWIDE_DECL NOWIDE_SYMBOL_IMPORT
#endif // NOWIDE_SOURCE
#else
#define NOWIDE_DECL
#endif // NOWIDE_DYN_LINK

#ifndef NOWIDE_DECL
#define NOWIDE_DECL
#endif

#if defined(NOWIDE_WINDOWS)
#ifdef NOWIDE_USE_FILEBUF_REPLACEMENT
#undef NOWIDE_USE_FILEBUF_REPLACEMENT
#endif
#define NOWIDE_USE_FILEBUF_REPLACEMENT 1
#elif !defined(NOWIDE_USE_FILEBUF_REPLACEMENT)
#define NOWIDE_USE_FILEBUF_REPLACEMENT 0
#endif

#if defined(__GNUC__) && __GNUC__ >= 7
#define NOWIDE_FALLTHROUGH __attribute__((fallthrough))
#else
#define NOWIDE_FALLTHROUGH
#endif

#if !defined(NOWIDE_LIKELY)
#define NOWIDE_LIKELY(x) x
#endif
#if !defined(NOWIDE_UNLIKELY)
#define NOWIDE_UNLIKELY(x) x
#endif

#endif
