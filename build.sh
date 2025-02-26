#!/bin/sh

[ -z "$DEST" ] && { 1>&2 echo "Environment variable DEST must be set."; exit 2; }

if [ "$USE_MINGW" != 0 ]; then
    cmake.exe -G "Unix Makefiles" -S . -B build \
        -D CMAKE_C_COMPILER=gcc.exe \
        -D CMAKE_CXX_COMPILER=g++.exe \
        -D CMAKE_MAKE_PROGRAM=mingw32-make.exe \
        || exit 1
else
    cmake.exe -S . -B build || exit 1
fi

cmake.exe --build build || exit 1

tgt()
{
    rm -f "$DEST/$1.exe"

    if [ "$USE_MINGW" != 0 ]; then
        built="./build/$1.exe"
    else
        built="./build/Debug/$1.exe"
    fi

    chmod +x "$built" && cp "$built" "$DEST/"
}

tgt tea
