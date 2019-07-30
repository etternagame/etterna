include(CheckIncludeFiles)
check_include_files(alloca.h HAVE_ALLOCA_H)
check_include_files(dirent.h HAVE_DIRENT_H)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(sys/utsname.h HAVE_SYS_UTSNAME_H)

include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)

# Mostly Windows functions.
check_function_exists(_mkdir HAVE__MKDIR)
check_cxx_symbol_exists(_snprintf cstdio HAVE__SNPRINTF)
check_cxx_symbol_exists(stricmp cstring HAVE_STRICMP)
check_cxx_symbol_exists(_stricmp cstring HAVE__STRICMP)

# Mostly non-Windows functions.
check_function_exists(fcntl HAVE_FCNTL)
check_function_exists(mkdir HAVE_MKDIR)
check_cxx_symbol_exists(snprintf cstdio HAVE_SNPRINTF)
check_cxx_symbol_exists(strcasecmp cstring HAVE_STRCASECMP)


# Mostly universal symbols.
check_symbol_exists(posix_fadvise fcntl.h HAVE_POSIX_FADVISE)
