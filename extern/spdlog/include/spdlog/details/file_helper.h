// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include "spdlog/common.h"
#include <tuple>

namespace spdlog {
namespace details {

// Helper class for file sinks.
// When failing to open a file, retry several times(5) with a delay interval(10 ms).
// Throw spdlog_ex exception on errors.

class file_helper
{
public:
    explicit file_helper() = default;

    file_helper(const file_helper &) = delete;
    file_helper &operator=(const file_helper &) = delete;
    ~file_helper();

    void open(const filename_t &fname, bool truncate = false);
    void reopen(bool truncate);
    void flush();
    void close();
    void write(const fmt::memory_buffer &buf);
    size_t size() const;
    const filename_t &filename() const;
    static bool file_exists(const filename_t &fname);

    //
    // return file path and its extension:
    //
    // "mylog.txt" => ("mylog", ".txt")
    // "mylog" => ("mylog", "")
    // "mylog." => ("mylog.", "")
    // "/dir1/dir2/mylog.txt" => ("/dir1/dir2/mylog", ".txt")
    //
    // the starting dot in filenames is ignored (hidden files):
    //
    // ".mylog" => (".mylog". "")
    // "my_folder/.mylog" => ("my_folder/.mylog", "")
    // "my_folder/.mylog.txt" => ("my_folder/.mylog", ".txt")
    static std::tuple<filename_t, filename_t> split_by_extension(const filename_t &fname);

private:
    const int open_tries = 5;
    const int open_interval = 10;
    std::FILE *fd_{nullptr};
    filename_t _filename;
};
} // namespace details
} // namespace spdlog

#ifdef SPDLOG_HEADER_ONLY
#include "file_helper-inl.h"
#endif
