#!/bin/bash


PROJDIR=$1
CONFIG=$2


DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TOP_DIR="$DIR/../../../.."
BUILD_DIR="$TOP_DIR/src_c/build_$CONFIG"
C_DLL="$BUILD_DIR/libAIL.so"

if [ ! -e "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

cmake .. -DCMAKE_BUILD_TYPE=$CONFIG

make

if [ ! -e "$PROJDIR/embedded_files" ]; then
 mkdir "$PROJDIR/embedded_files"
fi

cp "$C_DLL" "$PROJDIR/embedded_files/native_code"
