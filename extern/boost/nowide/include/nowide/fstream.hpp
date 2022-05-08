//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef NOWIDE_FSTREAM_HPP_INCLUDED
#define NOWIDE_FSTREAM_HPP_INCLUDED

#include <nowide/config.hpp>
#include <nowide/filebuf.hpp>
#include <istream>
#include <ostream>

namespace nowide {
    /// \cond INTERNAL
    namespace detail {
        // clang-format off
        struct StreamTypeIn
        {
            static std::ios_base::openmode mode() { return std::ios_base::in; }
            static std::ios_base::openmode mode_modifier() { return mode(); }
            template<typename CharType, typename Traits>
            struct stream_base{
                typedef std::basic_istream<CharType, Traits> type;
            };
        };
        struct StreamTypeOut
        {
            static std::ios_base::openmode mode() { return std::ios_base::out; }
            static std::ios_base::openmode mode_modifier() { return mode(); }
            template<typename CharType, typename Traits>
            struct stream_base{
                typedef std::basic_ostream<CharType, Traits> type;
            };
        };
        struct StreamTypeInOut
        {
            static std::ios_base::openmode mode() { return std::ios_base::in | std::ios_base::out; }
            static std::ios_base::openmode mode_modifier() { return std::ios_base::openmode(); }
            template<typename CharType, typename Traits>
            struct stream_base{
                typedef std::basic_iostream<CharType, Traits> type;
            };
        };
        // clang-format on

        /// Base class for all basic_*fstream classes
        /// Contains basic_filebuf instance so its pointer can be used to construct basic_*stream
        /// Provides common functions to reduce boilerplate code including inheriting from
        /// the correct std::basic_[io]stream class and initializing it
        /// \tparam T_StreamType One of StreamType* above.
        ///         Class used instead of value, because openmode::operator| may not be constexpr
        template<typename CharType, typename Traits, typename T_StreamType>
        class fstream_impl;

        template<typename Path, typename Result>
        struct enable_if_path;
    } // namespace detail
    /// \endcond

    ///
    /// \brief Same as std::basic_ifstream<char> but accepts UTF-8 strings under Windows
    ///
    template<typename CharType, typename Traits = std::char_traits<CharType> >
    class basic_ifstream : public detail::fstream_impl<CharType, Traits, detail::StreamTypeIn>
    {
        typedef detail::fstream_impl<CharType, Traits, detail::StreamTypeIn> fstream_impl;

    public:
        basic_ifstream()
        {}

        explicit basic_ifstream(const char* file_name, std::ios_base::openmode mode = std::ios_base::in)
        {
            open(file_name, mode);
        }
#if NOWIDE_USE_WCHAR_OVERLOADS
        explicit basic_ifstream(const wchar_t* file_name, std::ios_base::openmode mode = std::ios_base::in)
        {
            open(file_name, mode);
        }
#endif

        explicit basic_ifstream(const std::string& file_name, std::ios_base::openmode mode = std::ios_base::in)
        {
            open(file_name, mode);
        }

        template<typename Path>
        explicit basic_ifstream(
          const Path& file_name,
          typename detail::enable_if_path<Path, std::ios_base::openmode>::type mode = std::ios_base::in)
        {
            open(file_name, mode);
        }
        using fstream_impl::open;
        using fstream_impl::is_open;
        using fstream_impl::close;
        using fstream_impl::rdbuf;
        using fstream_impl::swap;
        basic_ifstream(const basic_ifstream&) = delete;
        basic_ifstream& operator=(const basic_ifstream&) = delete;
        basic_ifstream(basic_ifstream&& other) noexcept : fstream_impl(std::move(other))
        {}
        basic_ifstream& operator=(basic_ifstream&& rhs) noexcept
        {
            fstream_impl::operator=(std::move(rhs));
            return *this;
        }
    };

    ///
    /// \brief Same as std::basic_ofstream<char> but accepts UTF-8 strings under Windows
    ///

    template<typename CharType, typename Traits = std::char_traits<CharType> >
    class basic_ofstream : public detail::fstream_impl<CharType, Traits, detail::StreamTypeOut>
    {
        typedef detail::fstream_impl<CharType, Traits, detail::StreamTypeOut> fstream_impl;

    public:
        basic_ofstream()
        {}
        explicit basic_ofstream(const char* file_name, std::ios_base::openmode mode = std::ios_base::out)
        {
            open(file_name, mode);
        }
#if NOWIDE_USE_WCHAR_OVERLOADS
        explicit basic_ofstream(const wchar_t* file_name, std::ios_base::openmode mode = std::ios_base::out)
        {
            open(file_name, mode);
        }
#endif
        explicit basic_ofstream(const std::string& file_name, std::ios_base::openmode mode = std::ios_base::out)
        {
            open(file_name, mode);
        }
        template<typename Path>
        explicit basic_ofstream(
          const Path& file_name,
          typename detail::enable_if_path<Path, std::ios_base::openmode>::type mode = std::ios_base::out)
        {
            open(file_name, mode);
        }

        using fstream_impl::open;
        using fstream_impl::is_open;
        using fstream_impl::close;
        using fstream_impl::rdbuf;
        using fstream_impl::swap;
        basic_ofstream(const basic_ofstream&) = delete;
        basic_ofstream& operator=(const basic_ofstream&) = delete;
        basic_ofstream(basic_ofstream&& other) noexcept : fstream_impl(std::move(other))
        {}
        basic_ofstream& operator=(basic_ofstream&& rhs)
        {
            fstream_impl::operator=(std::move(rhs));
            return *this;
        }
    };

#ifdef NOWIDE_MSVC
#pragma warning(push)
#pragma warning(disable : 4250) // <class> : inherits <method> via dominance
#endif
    ///
    /// \brief Same as std::basic_fstream<char> but accepts UTF-8 strings under Windows
    ///
    template<typename CharType, typename Traits = std::char_traits<CharType> >
    class basic_fstream : public detail::fstream_impl<CharType, Traits, detail::StreamTypeInOut>
    {
        typedef detail::fstream_impl<CharType, Traits, detail::StreamTypeInOut> fstream_impl;

    public:
        basic_fstream()
        {}
        explicit basic_fstream(const char* file_name,
                               std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
        {
            open(file_name, mode);
        }
#if NOWIDE_USE_WCHAR_OVERLOADS
        explicit basic_fstream(const wchar_t* file_name,
                               std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
        {
            open(file_name, mode);
        }
#endif
        explicit basic_fstream(const std::string& file_name,
                               std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
        {
            open(file_name, mode);
        }
        template<typename Path>
        explicit basic_fstream(const Path& file_name,
                               typename detail::enable_if_path<Path, std::ios_base::openmode>::type mode =
                                 std::ios_base::in | std::ios_base::out)
        {
            open(file_name, mode);
        }

        using fstream_impl::open;
        using fstream_impl::is_open;
        using fstream_impl::close;
        using fstream_impl::rdbuf;
        using fstream_impl::swap;
        basic_fstream(const basic_fstream&) = delete;
        basic_fstream& operator=(const basic_fstream&) = delete;
        basic_fstream(basic_fstream&& other) noexcept : fstream_impl(std::move(other))
        {}
        basic_fstream& operator=(basic_fstream&& rhs)
        {
            fstream_impl::operator=(std::move(rhs));
            return *this;
        }
    };
    template<typename CharType, typename Traits>
    void swap(basic_filebuf<CharType, Traits>& lhs, basic_filebuf<CharType, Traits>& rhs)
    {
        lhs.swap(rhs);
    }
    template<typename CharType, typename Traits>
    void swap(basic_ifstream<CharType, Traits>& lhs, basic_ifstream<CharType, Traits>& rhs)
    {
        lhs.swap(rhs);
    }
    template<typename CharType, typename Traits>
    void swap(basic_ofstream<CharType, Traits>& lhs, basic_ofstream<CharType, Traits>& rhs)
    {
        lhs.swap(rhs);
    }
    template<typename CharType, typename Traits>
    void swap(basic_fstream<CharType, Traits>& lhs, basic_fstream<CharType, Traits>& rhs)
    {
        lhs.swap(rhs);
    }

    ///
    /// Same as std::filebuf but accepts UTF-8 strings under Windows
    ///
    typedef basic_filebuf<char> filebuf;
    ///
    /// Same as std::ifstream but accepts UTF-8 strings under Windows
    /// and *\::filesystem::path on all systems
    ///
    typedef basic_ifstream<char> ifstream;
    ///
    /// Same as std::ofstream but accepts UTF-8 strings under Windows
    /// and *\::filesystem::path on all systems
    ///
    typedef basic_ofstream<char> ofstream;
    ///
    /// Same as std::fstream but accepts UTF-8 strings under Windows
    /// and *\::filesystem::path on all systems
    ///
    typedef basic_fstream<char> fstream;

    // Implementation
    namespace detail {
        /// Holds an instance of T
        /// Required to make sure this is constructed first before passing it to sibling classes
        template<typename T>
        struct buf_holder
        {
            T buf_;
        };
        template<typename CharType, typename Traits, typename T_StreamType>
        class fstream_impl : private buf_holder<basic_filebuf<CharType, Traits> >, // must be first due to init order
                             public T_StreamType::template stream_base<CharType, Traits>::type
        {
            typedef basic_filebuf<CharType, Traits> internal_buffer_type;
            typedef buf_holder<internal_buffer_type> base_buf_holder;
            typedef typename T_StreamType::template stream_base<CharType, Traits>::type stream_base;

        public:
            using stream_base::setstate;
            using stream_base::clear;

        protected:
            using base_buf_holder::buf_;

            fstream_impl() : stream_base(&buf_)
            {}
            fstream_impl(const fstream_impl&) = delete;
            fstream_impl& operator=(const fstream_impl&) = delete;

            // coverity[exn_spec_violation]
            fstream_impl(fstream_impl&& other) noexcept : base_buf_holder(std::move(other)),
                                                          stream_base(std::move(other))
            {
                this->set_rdbuf(rdbuf());
            }
            fstream_impl& operator=(fstream_impl&& rhs) noexcept
            {
                base_buf_holder::operator=(std::move(rhs));
                stream_base::operator=(std::move(rhs));
                return *this;
            }
            void swap(fstream_impl& other)
            {
                stream_base::swap(other);
                rdbuf()->swap(*other.rdbuf());
            }

            void open(const std::string& file_name, std::ios_base::openmode mode = T_StreamType::mode())
            {
                open(file_name.c_str(), mode);
            }
            template<typename Path>
            typename detail::enable_if_path<Path, void>::type open(const Path& file_name,
                                                                   std::ios_base::openmode mode = T_StreamType::mode())
            {
                open(file_name.c_str(), mode);
            }
            void open(const char* file_name, std::ios_base::openmode mode = T_StreamType::mode())
            {
                if(!rdbuf()->open(file_name, mode | T_StreamType::mode_modifier()))
                    setstate(std::ios_base::failbit);
                else
                    clear();
            }
#if NOWIDE_USE_WCHAR_OVERLOADS
            void open(const wchar_t* file_name, std::ios_base::openmode mode = T_StreamType::mode())
            {
                if(!rdbuf()->open(file_name, mode | T_StreamType::mode_modifier()))
                    setstate(std::ios_base::failbit);
                else
                    clear();
            }
#endif
            bool is_open()
            {
                return rdbuf()->is_open();
            }
            bool is_open() const
            {
                return rdbuf()->is_open();
            }
            void close()
            {
                if(!rdbuf()->close())
                    setstate(std::ios_base::failbit);
            }

            internal_buffer_type* rdbuf() const
            {
                return const_cast<internal_buffer_type*>(&buf_);
            }
        };
#ifdef NOWIDE_MSVC
#pragma warning(pop)
#endif
        /// Trait to heuristically check for a *\::filesystem::path
        /// Done by checking for make_preferred and filename member functions with correct signature
        template<typename T>
        struct is_path
        {
            typedef char one;
            struct two
            {
                char dummy[2];
            };

            template<typename U, U& (U::*)(), U (U::*)() const>
            struct Check;
            template<typename U>
            static one test(Check<U, &U::make_preferred, &U::filename>*);
            template<typename U>
            static two test(...);

            enum
            {
                value = sizeof(test<T>(0)) == sizeof(one)
            };
        };
        template<bool B, typename T>
        struct enable_if
        {};
        template<typename T>
        struct enable_if<true, T>
        {
            typedef T type;
        };
        /// SFINAE trait which has a member "type = Result" if the Path is a *\::filesystem::path
        template<typename Path, typename Result>
        struct enable_if_path : enable_if<is_path<Path>::value, Result>
        {};
    } // namespace detail
} // namespace nowide

#endif
