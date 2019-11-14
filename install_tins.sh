#!/bin/bash

git clone https://github.com/mfontanini/libtins.git
cd libtins
mkdir build
cd build
cmake ../
make
make install
ldconfig

