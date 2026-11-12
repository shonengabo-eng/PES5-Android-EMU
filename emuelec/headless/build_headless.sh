#!/bin/bash
# Compila el juego recompilado para aarch64 SIN ventana, GPU ni audio (el "harness" del SDK: backend nulo, audio
# nulo, mando por guion), para comprobar en la caja que el juego arranca. Es sdk/harness/build.sh adaptado a aarch64
# (sin -march=x86-64-v2, con -fsigned-char) y con los sustitutos de EmuELEC.
#   build_headless.sh <repo> <generated/game> <salida>
# entorno: CXX (envoltorio de clang), JOBS
set -e
REPO=$(realpath "$1"); GEN=$(realpath "$2"); mkdir -p "$3"; OUT=$(realpath "$3")
EM=$REPO/emuelec
H=$REPO/sdk/harness; RT=$REPO/sdk/runtime
CXX=${CXX:-/opt/wrap/clang++}; JOBS=${JOBS:-$(nproc)}
GEN_OBJ=$OUT/gen; HOST_OBJ=$OUT/host; mkdir -p "$GEN_OBJ" "$HOST_OBJ"

# char con signo, como el codigo x86 y el runtime esperan (en aarch64 es sin signo por defecto)
BASE="--target=aarch64-linux-gnu -std=c++20 -fno-strict-aliasing -fsigned-char"
HOST="$BASE -O2 -g0 -w -I$REPO/src -I$RT -I$GEN -I$H -I$H/compat -isystem $EM/shim/cxx20 \
 -include $REPO/native/compat/crt.h -DXBE_RAW_VERTICES=0 -DXBE_RENDER_THREAD=0"

echo "== codigo recompilado (256 particiones) =="
export CXX BASE RT GEN GEN_OBJ
ls "$GEN"/x86_recomp.*.cpp "$GEN"/x86_func_mapping.cpp | xargs -P "$JOBS" -I{} sh -c '
    o="$GEN_OBJ/$(basename {} .cpp).o"; [ "$o" -nt {} ] && exit 0
    $CXX $BASE -O2 -g0 -w -isystem '"$EM"'/shim/cxx20 -I"$RT" -I"$GEN" -c {} -o "$o.tmp" && mv "$o.tmp" "$o"'

echo "== runtime, juego y harness =="
# como la compilacion de Android: sin la ventana (app/), la entrada SDL3, D3D12, XAudio2 ni el hilo de render del SDK
# (el del juego esta en src/d3d); y sin los fuentes del juego que dependen de ImGui, SDL3 o JNI (los sustituye
# emuelec/headless/game_stubs.cpp)
SRC=$(ls "$RT"/*/*.cpp | grep -Ev "/runtime/app/|/core/input\.cpp|/gpu/d3d12\.cpp|/audio/xaudio2\.cpp|/runtime/gpu/render_thread\.cpp")
SRC="$SRC $(ls "$REPO"/src/*/*.cpp | grep -Ev "/src/common/(game_info|keyboard|cheats)\.cpp|/src/audio/sdl_linux\.cpp")"
SRC="$SRC $(ls "$H"/*.cpp) $EM/headless/game_stubs.cpp"

objs=()
for f in $SRC; do
    o=$HOST_OBJ/$(echo "${f#/}" | tr / _ | sed 's/\.cpp$/.o/'); objs+=("$o")
    [ "$o" -nt "$f" ] && continue
    echo "compile $f"
    rm -f "$o"
    $CXX $HOST -c "$f" -o "$o" &
    [ "$(jobs -r | wc -l)" -ge "$JOBS" ] && { wait -n || true; }
done
wait
for o in "${objs[@]}"; do [ -f "$o" ] || { echo "FALTA $o: fallo la compilacion de su fuente"; exit 1; }; done

echo "== enlazar =="
# GCC 8: std::filesystem esta en libstdc++fs
$CXX --target=aarch64-linux-gnu -o "$OUT/pes5_core" "${objs[@]}" "$GEN_OBJ"/*.o -lstdc++fs -lpthread
echo "built $OUT/pes5_core"
