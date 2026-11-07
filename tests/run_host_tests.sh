#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
command -v cc >/dev/null || { echo "C compiler cc required" >&2; exit 1; }
cc -O1 -std=c99 -Wall -Wextra -Werror -I tests/host_shims \
  tests/mock_state_test.c -o tests/mock_rc7
./tests/mock_rc7
rm -f tests/mock_rc7
