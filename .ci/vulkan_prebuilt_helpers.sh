# MIT License

# Copyright (c) 2022 humbletim

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# helper functions for downloading/installing platform-specific Vulkan SDKs
# originally meant for use from GitHub Actions
#   see: https://github.com/humbletim/install-vulkan-sdk
# -- humbletim 2022.02

# example of running manually:
# $ . vulkan_prebuilt_helpers.
# $ VULKAN_SDK_VERSION=1.3.204.0 download_linux    # fetches vulkan_sdk.tar.gz
# $ VULKAN_SDK=$PWD/VULKAN_SDK install_linux       # installs

function _os_filename() {
  case $1 in
    mac) echo vulkan_sdk.zip ;;
    linux) echo vulkan_sdk.tar.gz ;;
    windows) echo vulkan_sdk.exe ;;
    *) echo "unknown $1" >&2 ; exit 9 ;;
  esac
}

function download_vulkan_installer() {
  local os=$1
  local filename=$(_os_filename $os)
  local url=https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VERSION/$os/$filename?Human=true
  echo "_download_os_installer $os $filename $url" >&2
  if [[ -f $filename ]] ; then
    echo "using cached: $filename" >&2
  else
    curl --fail-with-body -s -L -o ${filename}.tmp $url || { echo "curl failed with error code: $?" >&2 ; curl -s -L --head $url >&2 ; exit 32 ; }
    test -f ${filename}.tmp
    mv -v ${filename}.tmp ${filename} 
  fi
  ls -lh $filename >&2
}

function unpack_vulkan_installer() {
  local os=$1
  local filename=$(_os_filename $os)
  test -f $filename
  install_${os}
}

function install_linux() {
  test -d $VULKAN_SDK && test -f vulkan_sdk.tar.gz
  echo "extract just the SDK's prebuilt binaries ($VULKAN_SDK_VERSION/x86_64) from vulkan_sdk.tar.gz into $VULKAN_SDK" >&2
  tar -C "$VULKAN_SDK" --strip-components 2 -xf vulkan_sdk.tar.gz $VULKAN_SDK_VERSION/x86_64
}

function install_windows() {
  test -d $VULKAN_SDK && test -f vulkan_sdk.exe
  ./vulkan_sdk.exe in --al --am -c -t $VULKAN_SDK
}

function install_mac() {
  test -d $VULKAN_SDK && test -f vulkan_sdk.zip
  unzip vulkan_sdk.zip
  local InstallVulkan
  if [[ -d InstallVulkan-${VULKAN_SDK_VERSION}.app/Contents ]] ; then
    InstallVulkan=InstallVulkan-${VULKAN_SDK_VERSION}
  elif [[ -d vulkansdk-macOS-${VULKAN_SDK_VERSION}.app/Contents ]] ; then
    InstallVulkan=vulkansdk-macOS-${VULKAN_SDK_VERSION}
  elif [[ -d InstallVulkan.app/Contents ]] ; then
    InstallVulkan=InstallVulkan
  else
    echo "expecting ..vulkan.app/Contents folder (perhaps lunarg changed the archive layout again?): vulkan_sdk.zip" >&2
    echo "file vulkan_sdk.zip" >&2
    file vulkan_sdk.zip
    echo "unzip -t vulkan_sdk.zip" >&2
    unzip -t vulkan_sdk.zip
    exit 7
  fi
  echo "recognized zip layout 'vulkan_sdk.zip' ${InstallVulkan}.app/Contents" >&2
  local sdk_temp=${VULKAN_SDK}.tmp
  sudo ${InstallVulkan}.app/Contents/MacOS/${InstallVulkan} --root "$sdk_temp" --accept-licenses --default-answer --confirm-command install
  du -hs $sdk_temp
  test -d $sdk_temp/macOS || { echo "unrecognized dmg folder layout: $sdk_temp" ; ls -l $sdk_temp ; exit 10 ; }
  cp -r $sdk_temp/macOS/* $VULKAN_SDK/
  if [[ -d ${InstallVulkan}.app/Contents ]] ; then
    sudo rm -rf "$sdk_temp"
    rm -rf ${InstallVulkan}.app
  fi
}
