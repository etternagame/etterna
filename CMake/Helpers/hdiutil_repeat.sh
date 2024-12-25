#!/usr/bin/env bash

## Copied from https://github.com/HEXRD/hexrdgui/pull/1768/files
# 
# BSD 3-Clause License
# 
# Copyright (c) 2024, Kitware, Inc., Lawrence Livermore National Laboratory,
# and Air Force Research Laboratory.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, this
#     list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.




# Workaround XProtect race condition for "hdiutil create" for MacOS 13

 set -e

 if [ "$1" != "create" ]; then
   # If it isn't an `hdiutil create` command, just run and exit normally
   hdiutil "$@"
   exit 0
 fi

 # For an `hdiutil create` command, try repeatedly, up to 10 times
 # This prevents spurious errors caused by a race condition with XProtect
 # See https://github.com/actions/runner-images/issues/7522
 i=0
 until
 hdiutil "$@"
 do
 if [ $i -eq 10 ]; then exit 1; fi
 i=$((i+1))
 sleep 1
 done