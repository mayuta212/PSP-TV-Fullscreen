#!/bin/sh
set -eu
cd "$(dirname "$0")"
if ! command -v psp-config >/dev/null || ! command -v psp-gcc >/dev/null; then
  echo "PSPSDK not found: configure PSPDEV and PATH before building." >&2
  exit 1
fi
make clean
make
[ -s PSPTVFullscreen.prx ]
sha256sum PSPTVFullscreen.prx
