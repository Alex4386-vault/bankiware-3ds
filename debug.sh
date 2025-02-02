#!/bin/bash
export TARGET_IP="192.168.0.120"

export DEVKITPRO="/opt/devkitpro"
export DEVKITARM="$DEVKITPRO/devkitARM"

$DEVKITARM/bin/arm-none-eabi-gdb bankiware-3ds.elf -ex "target remote $TARGET_IP:4000"