#!/bin/bash

mkdir lua_tmp && cd lua_tmp

# Install various requirements
sudo apt install build-essential libreadline-dev unzip

# Acquire and install Lua
curl -R -O http://www.lua.org/ftp/lua-5.1.5.tar.gz
tar -zxf lua-5.1.5.tar.gz
cd lua-5.1.5
make linux
sudo make install
cd ..

# Acquire and install LuaRocks
wget https://luarocks.org/releases/luarocks-3.9.1.tar.gz
tar zxpf luarocks-3.9.1.tar.gz
cd luarocks-3.9.1
./configure && make
sudo make install

# Install ldoc
luarocks install --local ldoc