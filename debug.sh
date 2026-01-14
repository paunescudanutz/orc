#!/bin/bash

BIN_NAME="orc"
BIN_PATH="bin/$BIN_NAME"

# compile
./build.sh
gdb $BIN_PATH
