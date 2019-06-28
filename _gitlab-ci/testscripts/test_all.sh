#!/bin/bash 

builddir=$1
workdir=$2

oldpwd=`pwd`
scriptdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

cd $scriptdir

#exit immediately if any command fails
set -e

./test_build.sh $builddir $workdir

cd $oldpwd
