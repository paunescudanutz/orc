#!/bin/bash

BIN_NAME="orc"
BIN_PATH="bin/$BIN_NAME"

./build.sh
sudo cp ./bin/orc /usr/bin
orc init ./src/main.c
# Run the binary
# ./$BIN_PATH 
