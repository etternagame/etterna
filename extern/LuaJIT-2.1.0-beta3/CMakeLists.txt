# Copyright (C) 2007-2013 LuaDist.
# Copyright (c) 2017 George White.
# Created by Peter Drahoš
# Modified to support upstream by George White
#
# Permission is hereby granted, free of charge, to any person obtaining a copy 
# of this software and associated documentation files (the "Software"), to deal 
# in the Software without restriction, including without limitation the rights 
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
# copies of the Software, and to permit persons to whom the Software is 
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
# SOFTWARE.

project(LuaJIT)
set(LUAJIT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

if(NOT IS_DIRECTORY ${LUAJIT_SOURCE_DIR})
	message(FATAL_ERROR "LUAJIT_SOURCE_DIR doesn't point to a valid directory.")
endif()

## CONFIGURATION

# Default configuration (we assume POSIX by default)
set(LUA_PATH "LUA_PATH" CACHE STRING "Environment variable to use as package.path.")
set(LUA_CPATH "LUA_CPATH" CACHE STRING "Environment variable to use as package.cpath.")
set(LUA_INIT "LUA_INIT" CACHE STRING "Environment variable for initial script.")

option(LUA_ANSI "Use only ansi features." OFF)
option(LUA_USE_RELATIVE_LOADLIB "Use modified loadlib.c with support for relative paths on posix systems." ON)
set(LUA_IDSIZE 60 CACHE STRING "gives the maximum size for the description of the source.")
set(LUA_PROMPT "> " CACHE STRING "Is the default prompt used by stand-alone Lua.")
set(LUA_PROMPT2 ">> " CACHE STRING "Is the default continuation prompt used by stand-alone Lua.")
set(LUA_MAXINPUT 512 CACHE STRING "Is the maximum length for an input line in the stand-alone interpreter.")

# Version
set(MAJVER 2)
set(MINVER 0)
set(RELVER 5)
set(ABIVER 5.1)
set(NODOTABIVER 51)

# Extra flags
option(LUAJIT_DISABLE_FFI "Permanently disable the FFI extension to reduce the size of the LuaJIT executable. But please consider that the FFI library is compiled-in, but NOT loaded by default. It only allocates any memory, if you actually make use of it." OFF)
option(LUAJIT_ENABLE_LUA52COMPAT "Features from Lua 5.2 that are unlikely to break existing code are enabled by default. Some other features that *might* break some existing code (e.g. __pairs or os.execute() return values) can be enabled here. Note: this does not provide full compatibility with Lua 5.2 at this time." ON)
option(LUAJIT_DISABLE_JIT "Disable the JIT compiler, i.e. turn LuaJIT into a pure interpreter." OFF)

option(LUAJIT_USE_SYSMALLOC "Use the system provided memory allocator (realloc) instead of the bundled memory allocator. This is slower, but sometimes helpful for debugging. It's helpful for Valgrind's memcheck tool, too. This option cannot be enabled on x64, since the built-in allocator is mandatory." OFF)
option(LUAJIT_USE_VALGRIND "This option is required to run LuaJIT under Valgrind. The Valgrind header files must be installed. You should enable debug information, too." OFF)
option(LUAJIT_USE_GDBJIT "This is the client for the GDB JIT API. GDB 7.0 or higher is required to make use of it. See lj_gdbjit.c for details. Enabling this causes a non-negligible overhead, even when not running under GDB." OFF)

option(LUA_USE_APICHECK "Turn on assertions for the Lua/C API to debug problems with lua_* calls. This is rather slow, use only while developing C libraries/embeddings." ON)
option(LUA_USE_ASSERT "Turn on assertions for the whole LuaJIT VM. This significantly slows down everything. Use only if you suspect a problem with LuaJIT itself." OFF)

option(LUAJIT_CPU_SSE2 "Disable SSE2." OFF)
option(LUAJIT_CPU_NOCMOV "Disable NOCMOV." OFF)

# Tunable variables
set(LUAI_MAXSTACK 65500 CACHE STRING "Max. # of stack slots for a thread (<64K).")
set(LUAI_MAXCSTACK 8000 CACHE STRING "Max. # of stack slots for a C func (<10K).")
set(LUAI_GCPAUSE 200 CACHE STRING "Pause GC until memory is at 200%.")
set(LUAI_GCMUL 200 CACHE STRING "Run GC at 200% of allocation speed.")
set(LUA_MAXCAPTURES 32 CACHE STRING "Max. pattern captures.")

## SETUP
set(LUA_DIRSEP "/")
set(LUA_MODULE_SUFFIX ${CMAKE_SHARED_MODULE_SUFFIX})
set(LUA_LDIR ${INSTALL_LMOD})
set(LUA_CDIR ${INSTALL_CMOD})

if(LUA_USE_RELATIVE_LOADLIB)
  # This will set up relative paths to lib
  string(REGEX REPLACE "[^!/]+" ".." LUA_DIR "!/${INSTALL_BIN}/")
else()
  # Direct path to installation
  set(LUA_DIR ${CMAKE_INSTALL_PREFIX} CACHE STRING "Destination from which modules will be resolved. See INSTALL_LMOD and INSTALL_CMOD.")
endif()

set(LUA_PATH_DEFAULT "./?.lua;${LUA_DIR}${LUA_LDIR}/?.lua;${LUA_DIR}${LUA_LDIR}/?/init.lua;./?/init.lua")
set(LUA_CPATH_DEFAULT "./?${LUA_MODULE_SUFFIX};${LUA_DIR}${LUA_CDIR}/?${LUA_MODULE_SUFFIX};${LUA_DIR}${LUA_CDIR}/loadall${LUA_MODULE_SUFFIX}")

if(WIN32 AND NOT CYGWIN)
  # Windows systems
  add_definitions(-DLUAJIT_OS=LUAJIT_OS_WINDOWS)
  option(LUA_BUILD_WLUA "Build wluajit interpreter for no-console applications." ON)
  set(LJVM_MODE peobj)
  # Paths (Double escapes needed)
  set(LUA_DIRSEP "\\\\")
  string(REPLACE "/" ${LUA_DIRSEP} LUA_DIR "${LUA_DIR}")
  string(REPLACE "/" ${LUA_DIRSEP} LUA_LDIR "${LUA_LDIR}")
  string(REPLACE "/" ${LUA_DIRSEP} LUA_CDIR "${LUA_CDIR}")
  string(REPLACE "/" ${LUA_DIRSEP} LUA_PATH_DEFAULT "${LUA_PATH_DEFAULT}")
  string(REPLACE "/" ${LUA_DIRSEP} LUA_CPATH_DEFAULT "${LUA_CPATH_DEFAULT}")

elseif(APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000 -image_base 7fff04c4a000")
  option(LUA_USE_POSIX "Use POSIX functionality." ON)
  option(LUA_USE_DLOPEN "Use dynamic linker to load modules." ON)
  set(LJVM_MODE machasm)
else()
  option(LUA_USE_POSIX "Use POSIX functionality." ON)
  option(LUA_USE_DLOPEN "Use dynamic linker to load modules." OFF)
  set(LJVM_MODE elfasm)
endif()

# Setup needed variables and libraries
if(LUA_USE_POSIX)
  # On POSIX Lua links to standard math library "m"
  list(APPEND LIBS m)
endif()

if(LUA_USE_DLOPEN)
  # Link to dynamic linker library "dl"
  list(APPEND LIBS dl)
endif()

## SOURCES

set(LJLIB_C
  ${LUAJIT_SOURCE_DIR}/src/lib_aux.c
  ${LUAJIT_SOURCE_DIR}/src/lib_init.c
  ${LUAJIT_SOURCE_DIR}/src/lib_base.c
  ${LUAJIT_SOURCE_DIR}/src/lib_math.c
  ${LUAJIT_SOURCE_DIR}/src/lib_bit.c
  ${LUAJIT_SOURCE_DIR}/src/lib_string.c
  ${LUAJIT_SOURCE_DIR}/src/lib_table.c
  ${LUAJIT_SOURCE_DIR}/src/lib_io.c
  ${LUAJIT_SOURCE_DIR}/src/lib_os.c
  ${LUAJIT_SOURCE_DIR}/src/lib_debug.c
  ${LUAJIT_SOURCE_DIR}/src/lib_jit.c
  ${LUAJIT_SOURCE_DIR}/src/lib_ffi.c
  ${LUAJIT_SOURCE_DIR}/src/lib_package.c
)

## GENERATE VM
# Build minilua
include_directories (${LUAJIT_SOURCE_DIR}/src)
add_executable(minilua ${LUAJIT_SOURCE_DIR}/src/host/minilua.c)
target_link_libraries(minilua ${LIBS})

# Dynasm
set(DASM ${LUAJIT_SOURCE_DIR}/dynasm/dynasm.lua)
set(DASM_T ${LUAJIT_SOURCE_DIR}/src/host/buildvm_arch.h)

# 2DO: Proper detection of flags
set(DASM_FLAGS -D FPU -D HFABI)
set(DASM_ARCH x86)

# All ARM hosts (iOS, Android, Raspberry Pi etc.)
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "(arm|ARM).*")
  set(DASM_ARCH arm)
  list(APPEND DASM_FLAGS "-D DUALNUM")
  add_definitions("-DLJ_ARCH_HASFPU=1")
endif()

# Windows is ... special
if(WIN32)
  list(APPEND DASM_FLAGS -D WIN)
endif()

# 32bit vs 64bit
if(CMAKE_SIZEOF_VOID_P EQUAL 8 OR "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")
  list(APPEND DASM_FLAGS -D P64)
endif()

if(NOT LUAJIT_DISABLE_JIT)
  list(APPEND DASM_FLAGS -D JIT)
endif()

if(NOT LUAJIT_DISABLE_FFI)
  list(APPEND DASM_FLAGS -D FFI)
endif()

if(NOT LUAJIT_CPU_SSE2)
  list(APPEND DASM_FLAGS -D SSE2)
endif()

string(REPLACE ";" " " DASM_FLAGS_STR "${DASM_FLAGS}")

message("DASM_FLAGS: ${DASM_FLAGS_STR}")
message("DASM_ARCH: ${DASM_ARCH}")

set(DASM_DASC ${LUAJIT_SOURCE_DIR}/src/vm_${DASM_ARCH}.dasc)

# Generate buildvm arch header
add_custom_command(OUTPUT ${DASM_T}
  COMMAND minilua ${DASM} ${DASM_FLAGS} -o ${DASM_T} ${DASM_DASC}
  WORKING_DIRECTORY ${LUAJIT_SOURCE_DIR}/dynasm
  DEPENDS minilua
)

# buildvm
file(GLOB SRC_BUILDVM ${LUAJIT_SOURCE_DIR}/src/host/buildvm*.c)
add_executable(buildvm ${SRC_BUILDVM} ${DASM_T})

macro(add_buildvm_target _target _mode)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_target}
    COMMAND buildvm ARGS -m ${_mode} -o ${CMAKE_CURRENT_BINARY_DIR}/${_target} ${ARGN}
    WORKING_DIRECTORY ${LUAJIT_SOURCE_DIR}
    DEPENDS buildvm ${ARGN}
 )
endmacro(add_buildvm_target)

if(WIN32 AND NOT CYGWIN)
  add_buildvm_target(lj_vm.obj peobj)
  set (LJ_VM_SRC ${CMAKE_CURRENT_BINARY_DIR}/lj_vm.obj)
else()
  add_buildvm_target(lj_vm.s ${LJVM_MODE})
  set (LJ_VM_SRC ${CMAKE_CURRENT_BINARY_DIR}/lj_vm.s)
  if (APPLE)
    set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/lj_vm.s PROPERTY LANGUAGE ASM)
  else() 
    set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/lj_vm.s PROPERTY LANGUAGE C)
  endif()
endif()

add_buildvm_target(lj_ffdef.h	 ffdef   ${LJLIB_C})
add_buildvm_target(lj_bcdef.h    bcdef	 ${LJLIB_C})
add_buildvm_target(lj_folddef.h  folddef ${LUAJIT_SOURCE_DIR}/src/lj_opt_fold.c)
add_buildvm_target(lj_recdef.h	 recdef  ${LJLIB_C})
add_buildvm_target(lj_libdef.h	 libdef  ${LJLIB_C})

SET(DEPS
  ${LJ_VM_SRC}
  ${CMAKE_CURRENT_BINARY_DIR}/lj_ffdef.h
  ${CMAKE_CURRENT_BINARY_DIR}/lj_bcdef.h
  ${CMAKE_CURRENT_BINARY_DIR}/lj_libdef.h
  ${CMAKE_CURRENT_BINARY_DIR}/lj_recdef.h
  ${CMAKE_CURRENT_BINARY_DIR}/lj_folddef.h
)

## LuaJIT Library

set(LJCORE_C
  ${LUAJIT_SOURCE_DIR}/src/lj_alloc.c
  ${LUAJIT_SOURCE_DIR}/src/lj_alloc.h
  ${LUAJIT_SOURCE_DIR}/src/lj_api.c
  ${LUAJIT_SOURCE_DIR}/src/lj_arch.h
  ${LUAJIT_SOURCE_DIR}/src/lj_asm.c
  ${LUAJIT_SOURCE_DIR}/src/lj_asm.h
  ${LUAJIT_SOURCE_DIR}/src/lj_bc.c
  ${LUAJIT_SOURCE_DIR}/src/lj_bc.h
  ${LUAJIT_SOURCE_DIR}/src/lj_bcdump.h
  ${LUAJIT_SOURCE_DIR}/src/lj_bcread.c
  ${LUAJIT_SOURCE_DIR}/src/lj_bcwrite.c
  ${LUAJIT_SOURCE_DIR}/src/lj_buf.c
  ${LUAJIT_SOURCE_DIR}/src/lj_buf.h
  ${LUAJIT_SOURCE_DIR}/src/lj_carith.c
  ${LUAJIT_SOURCE_DIR}/src/lj_carith.h
  ${LUAJIT_SOURCE_DIR}/src/lj_ccall.c
  ${LUAJIT_SOURCE_DIR}/src/lj_ccall.h
  ${LUAJIT_SOURCE_DIR}/src/lj_ccallback.c
  ${LUAJIT_SOURCE_DIR}/src/lj_ccallback.h
  ${LUAJIT_SOURCE_DIR}/src/lj_cconv.c
  ${LUAJIT_SOURCE_DIR}/src/lj_cconv.h
  ${LUAJIT_SOURCE_DIR}/src/lj_cdata.c
  ${LUAJIT_SOURCE_DIR}/src/lj_cdata.h
  ${LUAJIT_SOURCE_DIR}/src/lj_char.c
  ${LUAJIT_SOURCE_DIR}/src/lj_char.h
  ${LUAJIT_SOURCE_DIR}/src/lj_clib.c
  ${LUAJIT_SOURCE_DIR}/src/lj_clib.h
  ${LUAJIT_SOURCE_DIR}/src/lj_cparse.c
  ${LUAJIT_SOURCE_DIR}/src/lj_cparse.h
  ${LUAJIT_SOURCE_DIR}/src/lj_crecord.c
  ${LUAJIT_SOURCE_DIR}/src/lj_crecord.h
  ${LUAJIT_SOURCE_DIR}/src/lj_ctype.c
  ${LUAJIT_SOURCE_DIR}/src/lj_ctype.h
  ${LUAJIT_SOURCE_DIR}/src/lj_debug.c
  ${LUAJIT_SOURCE_DIR}/src/lj_debug.h
  ${LUAJIT_SOURCE_DIR}/src/lj_def.h
  ${LUAJIT_SOURCE_DIR}/src/lj_dispatch.c
  ${LUAJIT_SOURCE_DIR}/src/lj_dispatch.h
  ${LUAJIT_SOURCE_DIR}/src/lj_emit_arm.h
  ${LUAJIT_SOURCE_DIR}/src/lj_emit_mips.h
  ${LUAJIT_SOURCE_DIR}/src/lj_emit_ppc.h
  ${LUAJIT_SOURCE_DIR}/src/lj_emit_x86.h
  ${LUAJIT_SOURCE_DIR}/src/lj_err.c
  ${LUAJIT_SOURCE_DIR}/src/lj_err.h
  ${LUAJIT_SOURCE_DIR}/src/lj_errmsg.h
  ${LUAJIT_SOURCE_DIR}/src/lj_ff.h
  ${LUAJIT_SOURCE_DIR}/src/lj_ffrecord.c
  ${LUAJIT_SOURCE_DIR}/src/lj_ffrecord.h
  ${LUAJIT_SOURCE_DIR}/src/lj_frame.h
  ${LUAJIT_SOURCE_DIR}/src/lj_func.c
  ${LUAJIT_SOURCE_DIR}/src/lj_func.h
  ${LUAJIT_SOURCE_DIR}/src/lj_gc.c
  ${LUAJIT_SOURCE_DIR}/src/lj_gc.h
  ${LUAJIT_SOURCE_DIR}/src/lj_gdbjit.c
  ${LUAJIT_SOURCE_DIR}/src/lj_gdbjit.h
  ${LUAJIT_SOURCE_DIR}/src/lj_ir.c
  ${LUAJIT_SOURCE_DIR}/src/lj_ir.h
  ${LUAJIT_SOURCE_DIR}/src/lj_ircall.h
  ${LUAJIT_SOURCE_DIR}/src/lj_iropt.h
  ${LUAJIT_SOURCE_DIR}/src/lj_jit.h
  ${LUAJIT_SOURCE_DIR}/src/lj_lex.c
  ${LUAJIT_SOURCE_DIR}/src/lj_lex.h
  ${LUAJIT_SOURCE_DIR}/src/lj_lib.c
  ${LUAJIT_SOURCE_DIR}/src/lj_lib.h
  ${LUAJIT_SOURCE_DIR}/src/lj_load.c
  ${LUAJIT_SOURCE_DIR}/src/lj_mcode.c
  ${LUAJIT_SOURCE_DIR}/src/lj_mcode.h
  ${LUAJIT_SOURCE_DIR}/src/lj_meta.c
  ${LUAJIT_SOURCE_DIR}/src/lj_meta.h
  ${LUAJIT_SOURCE_DIR}/src/lj_obj.c
  ${LUAJIT_SOURCE_DIR}/src/lj_obj.h
  ${LUAJIT_SOURCE_DIR}/src/lj_opt_dce.c
  ${LUAJIT_SOURCE_DIR}/src/lj_opt_fold.c
  ${LUAJIT_SOURCE_DIR}/src/lj_opt_loop.c
  ${LUAJIT_SOURCE_DIR}/src/lj_opt_mem.c
  ${LUAJIT_SOURCE_DIR}/src/lj_opt_narrow.c
  ${LUAJIT_SOURCE_DIR}/src/lj_opt_sink.c
  ${LUAJIT_SOURCE_DIR}/src/lj_opt_split.c
  ${LUAJIT_SOURCE_DIR}/src/lj_parse.c
  ${LUAJIT_SOURCE_DIR}/src/lj_parse.h
  ${LUAJIT_SOURCE_DIR}/src/lj_profile.c
  ${LUAJIT_SOURCE_DIR}/src/lj_profile.h
  ${LUAJIT_SOURCE_DIR}/src/lj_record.c
  ${LUAJIT_SOURCE_DIR}/src/lj_record.h
  ${LUAJIT_SOURCE_DIR}/src/lj_snap.c
  ${LUAJIT_SOURCE_DIR}/src/lj_snap.h
  ${LUAJIT_SOURCE_DIR}/src/lj_state.c
  ${LUAJIT_SOURCE_DIR}/src/lj_state.h
  ${LUAJIT_SOURCE_DIR}/src/lj_str.c
  ${LUAJIT_SOURCE_DIR}/src/lj_str.h
  ${LUAJIT_SOURCE_DIR}/src/lj_strfmt.c
  ${LUAJIT_SOURCE_DIR}/src/lj_strfmt.h
  ${LUAJIT_SOURCE_DIR}/src/lj_strfmt_num.c
  ${LUAJIT_SOURCE_DIR}/src/lj_strscan.c
  ${LUAJIT_SOURCE_DIR}/src/lj_strscan.h
  ${LUAJIT_SOURCE_DIR}/src/lj_tab.c
  ${LUAJIT_SOURCE_DIR}/src/lj_tab.h
  ${LUAJIT_SOURCE_DIR}/src/lj_target.h
  ${LUAJIT_SOURCE_DIR}/src/lj_trace.c
  ${LUAJIT_SOURCE_DIR}/src/lj_trace.h
  ${LUAJIT_SOURCE_DIR}/src/lj_traceerr.h
  ${LUAJIT_SOURCE_DIR}/src/lj_udata.c
  ${LUAJIT_SOURCE_DIR}/src/lj_udata.h
  ${LUAJIT_SOURCE_DIR}/src/lj_vm.h
  ${LUAJIT_SOURCE_DIR}/src/lj_vmevent.c
  ${LUAJIT_SOURCE_DIR}/src/lj_vmevent.h
  ${LUAJIT_SOURCE_DIR}/src/lj_vmmath.c
  ${LJLIB_C}
)

add_library(lua STATIC ${LJCORE_C} ${DEPS})
target_include_directories(lua
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR} "${LUAJIT_SOURCE_DIR}/dynasm" "${LUAJIT_SOURCE_DIR}/src/host"
	PUBLIC "${LUAJIT_SOURCE_DIR}/src")
target_link_libraries(lua ${LIBS})
set_target_properties(lua PROPERTIES LINKER_LANGUAGE C)

#add_executable(luajit ${LUAJIT_SOURCE_DIR}/src/luajit.c)
add_library(luajit STATIC ${LUAJIT_SOURCE_DIR}/src/luajit.c)
set_target_properties(luajit PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(luajit lua)
