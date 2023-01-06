#!/bin/bash

# Install Etterna requirements
sudo apt-get install build-essential libssl-dev libx11-dev libxrandr-dev libcurl4-openssl-dev libglu1-mesa-dev libpulse-dev libogg-dev libasound-dev libjack-dev

#Install doxygen
sudo apt-get install doxygen graphviz

eval "$(luarocks path --bin)"

# Generate Etterna CMake
mkdir build && cd build
cmake -G "Unix Makefiles" -DWITH_CRASHPAD=OFF ..