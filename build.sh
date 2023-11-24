#!/bin/bash

# format code
# clang-format -i src/porporo.c

SRC="src/uxn.c src/devices/system.c src/porporo.c"

# remove old
rm -f bin/porporo
mkdir -p bin

# debug(slow)
cc -std=c89 -DDEBUG -Wall -Wno-unknown-pragmas -Wpedantic -Wshadow -Wextra -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=int-conversion -Wvla -g -Og -fsanitize=address -fsanitize=undefined $SRC -L/usr/local/lib -lSDL2 -o bin/porporo

# build(fast)
# cc main.c -std=c89 -Os -DNDEBUG -g0 -s -Wall -Wno-unknown-pragmas -L/usr/local/lib -lSDL2 -lportmidi -o porporo

# roms
cc src/uxnasm.c -o bin/uxnasm
bin/uxnasm etc/hello.tal bin/hello.rom


# run
./bin/porporo