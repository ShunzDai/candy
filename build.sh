#!/bin/bash
build_dir=$(find -name "build")
if [[ ! -n $build_dir ]]
then
  mkdir build
  build_dir=$(find -name "build")
fi
cd build
cmake ..
make -j4
cd ..
if [[ -n $1 ]]
then
  ./build/demo/$1/$1
fi
