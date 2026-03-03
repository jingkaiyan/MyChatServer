#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CLIENT_BIN="${PROJECT_ROOT}/bin/ChatClient"

if [[ ! -x "${CLIENT_BIN}" ]]; then
  echo "ChatClient not found. Build first:"
  echo "  cd ${PROJECT_ROOT} && BUILD_SERVER=OFF BUILD_CLIENT=ON ./autobuild.sh"
  exit 1
fi

"${CLIENT_BIN}" "$@"
