#!/usr/bin/env bash
# Build SQ8LMPC for MPC OS (armv7-a hard-float, glibc <= 2.31 so it also loads on MPC OS 2.x).
#   ./build.sh                     (MPC_VST=... to use another framework checkout)
# Needs: python3 + Pillow, numpy and scipy, a host gcc (for shadow_art), and Zig (pip install ziglang). No Docker.
# Only the Force (ARM) binary is built; there is no desktop build of this port.
# Output: build/package/L'Cronx - VST - SQ8LMPC/  (skin + sq8lmpc.so)
set -euo pipefail
cd "$(dirname "$0")"
MPC_VST="${MPC_VST:-$PWD/third_party/mpc-vst-plugins}"
ZIG="${ZIG:-python3 -m ziglang}"
NAME="L'Cronx - VST - SQ8LMPC"
python3 tools/gen_params.py
python3 tools/panel.py --check

mkdir -p build/tools build/arm build/obj
[ -x build/tools/shadow_art ] || gcc -O2 -w -I"$MPC_VST/tools/vendor/force-shadow/tools" -x c -o build/tools/shadow_art "$MPC_VST/tools/shadow_art.c" -lm
rm -rf build/skin
SHADOW_ART="$PWD/build/tools/shadow_art" \
  python3 "$MPC_VST/tools/gen_vst.py" vst.json 2>&1 | grep -v -e "TOUCH" -e "EDGE" -e "MODULE_DIR" || true
[ -f build/params.h ] && [ -d "build/skin/$NAME/Plugin Skins" ]
python3 tools/panel_art.py "build/skin/$NAME/Plugin Skins"
python3 "$MPC_VST/tools/skin_check.py" "build/skin/$NAME"

TGT="-target arm-linux-gnueabihf.2.31 -mcpu=generic+v7a+vfp3d16-d32-neon+thumb2"
FP="-ffp-contract=off -frounding-math -Wno-unsupported-floating-point-opt"   # the engine's x87-exact arithmetic (see src/engine/Fpu.h)
INC="-Isrc/engine -Isrc/gui -Isrc/gui/logic -Isrc/gui/controls -Ibuild -I$MPC_VST/wrapper"
CXXF="$TGT -O2 -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections -DNDEBUG -w -std=c++17 -D_FILE_OFFSET_BITS=64 $FP $INC"
OBJS=""
cxx() { o="build/obj/$(echo "$1" | tr / _).o"; $ZIG c++ $CXXF -c "$1" -o "$o"; OBJS="$OBJS $o"; }
for f in src/engine/*.cpp src/engine/data/*.cpp src/gui/Canvas.cpp src/gui/controls/*.cpp src/gui/logic/*.cpp \
         src/gui/logic/data/*.cpp src/sq8lmpc/engine.cpp src/sq8lmpc/sprite_stub.cpp; do cxx "$f"; done
# core-math: correctly rounded exp/log/sin/cos/tan (needs 128-bit integers: clang's _BitInt(128) on ARMv7)
for f in exp log sin cos tan; do
  o="build/obj/cm_$f.o"; $ZIG cc $TGT -O2 -fPIC -w $FP -c third_party/core-math/$f.c -o "$o"; OBJS="$OBJS $o"
done
$ZIG cc $TGT -O2 -fPIC -fvisibility=hidden -DNDEBUG -w -std=gnu11 -Ibuild -I"$MPC_VST/wrapper" -c src/wrapper/vst2_wrap.c -o build/obj/vst2_wrap.o 2>/dev/null \
  || $ZIG cc $TGT -O2 -fPIC -fvisibility=hidden -DNDEBUG -w -std=gnu11 -Ibuild -I"$MPC_VST/wrapper" -c "$MPC_VST/wrapper/vst2_wrap.c" -o build/obj/vst2_wrap.o
OBJS="$OBJS build/obj/vst2_wrap.o"
printf '{\n  global: VSTPluginMain;\n  local: *;\n};\n' > build/arm/exports.map
$ZIG c++ $TGT -shared -fPIC -Wl,--no-undefined -Wl,--version-script=build/arm/exports.map -Wl,--gc-sections -Wl,-s \
  $OBJS -lm -ldl -lpthread -o build/arm/sq8lmpc.so
PKG="build/package/$NAME"
rm -rf build/package && mkdir -p "$PKG"
cp -r "build/skin/$NAME/." "$PKG/"
cp build/arm/sq8lmpc.so "$PKG/"
echo "-> $PKG"
