#!/usr/bin/env bash

update_cmake (){
    # Update cmake
    CMAKE_DIR="${HOME}/cmake_binary"
    mkdir -p ${CMAKE_DIR} && cd ${CMAKE_DIR}
    curl -LO https://github.com/Kitware/CMake/releases/download/v3.14.0/cmake-3.14.0-Linux-x86_64.sh
    sudo sh cmake-3.14.0-Linux-x86_64.sh --prefix=/usr/local/ --exclude-subdir --skip-license
    PATH=/usr/local/bin:${PATH} # add it to path in front of everything else
}

update_cmake

echo "SUCCESS"

