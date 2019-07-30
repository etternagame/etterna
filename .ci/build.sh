#!/usr/bin/env bash

update_cmake (){
    CMAKE_DIR=$(mktemp -d)
    mkdir -p ${CMAKE_DIR} && cd ${CMAKE_DIR}
    curl -LO https://github.com/Kitware/CMake/releases/download/v3.14.0/cmake-3.14.0-Linux-x86_64.sh
    sudo sh cmake-3.14.0-Linux-x86_64.sh --prefix=/usr/local/ --exclude-subdir --skip-license
    cd .. && rm -rf $CMAKE_DIR
}

pushd .
update_cmake
popd

echo "CMake Updated!"