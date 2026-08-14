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

function preset_env() {
    basedir=$PWD
    runner_os=${RUNNER_OS:-`uname -s`}
    case $runner_os in
        macOS|Darwin) os=mac ;;
        Linux) os=linux ;;
        Windows|MINGW*) os=windows ; basedir=$(pwd -W) ;;
        *) echo "unknown runner_os: $runner_os" ; exit 7 ; ;;
    esac
    version='1.4.341.1' # hi hello
    sdk_dir=${VULKAN_SDK:-$basedir/VULKAN_SDK}
    test -d $sdk_dir || mkdir -pv $sdk_dir
    if [[ $version == 'latest' ]] ; then
        url=https://vulkan.lunarg.com/sdk/latest/$os.txt
        echo "note: resolving '$version' for '$os' via webservices lookup: $url" >&2
        version=$(curl -sL $url)
        test -n "$version" || { echo "could not resolve latest version" ; exit 9 ; }
        echo "::notice title=Using Vulkan SDK $version::resolved via '$url'"
    fi

    export VULKAN_SDK="$sdk_dir"
    export VULKAN_SDK_VERSION="$version"
    export VULKAN_SDK_PLATFORM="$os"

    echo "VULKAN_SDK=$sdk_dir" >> "$GITHUB_ENV"
    echo "VULKAN_SDK_VERSION=$version" >> "$GITHUB_ENV"
    echo "VULKAN_SDK_PLATFORM=$os" >> "$GITHUB_ENV"
}