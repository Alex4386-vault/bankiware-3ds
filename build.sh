#!/bin/bash
export DEVKITPRO="/opt/devkitpro"
export DEVKITARM="$DEVKITPRO/devkitARM"

if [ "$1" = "codeonly" ]; then
    make codeonly
else
    make clean
    make
fi

# This is WSL2, copy out the stuff out
rm -rf /mnt/c/Users/alex4386/Documents/bankiware/bankiware-3ds.*
cp ./bankiware-3ds.* /mnt/c/Users/alex4386/Documents/bankiware/
