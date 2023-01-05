#!/bin/bash

# Put luadoc into our PATH
eval "$(luarocks path --bin)"

mkdir ldoc && cd ldoc

# Run luadoc
cp ../Docs/LDoc ./config.ld
ldoc .

# Cleanup
rm config.ld
cd ..

cp -r ldoc Docs/