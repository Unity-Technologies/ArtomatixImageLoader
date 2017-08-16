#!/bin/bash 

TOP_DIR=$(pwd)
NUNIT="$(pwd)/hooks/binfiles/NUnit-2.6.4/bin/nunit-console.exe"
NUNIT_ARGS='-noshadow -exclude "JenkinsDisable" -xml=nunit-result.xml -nologo'

function quitOnFailure
{
    if [ ! $? -eq 0  ]; then exit 1
    fi
}

function doTests
{
        mono $NUNIT $1 $NUNIT_ARGS;
        quitOnFailure;
}

cd ${TOP_DIR}
git reset .
git checkout .
git clean -dxf

bash .paket/get_paket.sh

mono .paket/paket.exe restore

cd src_c
rm -rf build
mkdir build
cd build

cmake .. && make -j4 && make aitest
quitOnFailure

cd ${TOP_DIR}

cd bindings/csharp/ArtomatixImageLoader

xbuild ArtomatixImageLoader.sln
doTests ArtomatixImageLoaderTests/bin/x64/Debug/ArtomatixImageLoaderTests.dll






exit 0
