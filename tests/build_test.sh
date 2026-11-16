#!/usr/bin/env bash
# Builds one tests/<name>.cpp against the engine objects in build/obj (ARMv7, runs under qemu-arm).
#   tests/build_test.sh <name> [extra flags]   -> build/<name>_arm
set -euo pipefail
cd "$(dirname "$0")/.."
MPC_VST="${MPC_VST:-$PWD/third_party/mpc-vst-plugins}"
ZIG="${ZIG:-python3 -m ziglang}"
TGT="-target arm-linux-gnueabihf.2.31 -mcpu=generic+v7a+vfp3d16-d32-neon+thumb2"
FP="-ffp-contract=off -frounding-math -Wno-unsupported-floating-point-opt"
INC="-Isrc/engine -Isrc/gui -Isrc/gui/logic -Isrc/gui/controls -Ibuild -I$MPC_VST/wrapper"
n="$1"; shift
OBJS=$(ls build/obj/*.o | grep -v vst2_wrap)
$ZIG c++ $TGT -O2 -w -std=c++17 $FP $INC "$@" tests/$n.cpp $OBJS -lm -ldl -lpthread -o build/${n}_arm
echo "-> build/${n}_arm"
