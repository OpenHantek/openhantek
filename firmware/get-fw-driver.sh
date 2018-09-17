#!/bin/bash

rm -rf build
mkdir build
cd build
cmake ../
make
cd ../
mkdir -p hex
mv $(ls | grep -e "DSO.*Driver") hex

for f in hex/**/*.sys; do
    ./build/FirmwareExtractor $f && rm $f
done