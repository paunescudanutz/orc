#!/bin/bash

BIN_NAME="orc"
BIN_PATH="bin/$BIN_NAME"

# remove old binary
rm $BIN_PATH

# compile
gcc -g lib/*.c src/*.c  test/*.c -lm -o $BIN_PATH

