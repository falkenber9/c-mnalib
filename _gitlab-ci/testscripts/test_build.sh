#!/bin/bash 

templatedir="../testdata"
builddir=$1
workdir=$2

#exit immediately if any command fails
set -e

echo "Starting to build"

srcdir="/c-mnalib"

mkdir -p $builddir
cd $builddir
cmake $srcdir
make -j

echo "Build complete"

echo "Testing"

src/cmnalib/./testlib

echo "Test complete"

echo "Installing"

make install

echo "Installation complete"
