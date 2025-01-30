#!/bin/bash
export DEVKITPRO="/opt/devkitpro"
export DEVKITARM="$DEVKITPRO/devkitARM"

if [ "$1" = "codeonly" ]; then
    make codeonly
else
    make clean
    make
fi
