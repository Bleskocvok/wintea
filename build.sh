#!/bin/sh

if [ "$USE_MINGW" = 1 ]; then
    cmake.exe -G "Unix Makefiles" -S . -B build || exit 1
else
    cmake.exe -S . -B build || exit 1
fi

cmake.exe --build build || exit 1

tgt()
{
    dest="/mnt/c/Users/Frantisek/Desktop"
    rm -f $dest/$1.exe

    if [ "$USE_MINGW" = 1 ]; then
        built="./build/$1.exe"
    else
        built="./build/Debug/$1.exe"
    fi

    chmod +x $built && cp $built $dest/
}

tgt topwin
tgt chat
