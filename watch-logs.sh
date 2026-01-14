#!/bin/bash

tail -f log.txt | sed -u '
  /ERROR/   {s/.*/\x1b[1;31m&\x1b[0m/}
  /DEBUG/   {s/.*/\x1b[38;5;208m&\x1b[0m/}
  /INFO/    {s/.*/\x1b[1;32m&\x1b[0m/}
' | fzf -e --tac --ansi --no-clear --no-sort
