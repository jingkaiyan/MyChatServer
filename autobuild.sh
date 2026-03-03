#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

BUILD_SERVER="${BUILD_SERVER:-ON}"
BUILD_CLIENT="${BUILD_CLIENT:-ON}"
CLEAN_BUILD="${CLEAN_BUILD:-OFF}"

if [[ "${CLEAN_BUILD}" == "ON" ]]; then
	rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
	-DBUILD_SERVER="${BUILD_SERVER}" \
	-DBUILD_CLIENT="${BUILD_CLIENT}"

cmake --build "${BUILD_DIR}" -j"$(nproc)"

echo "Build finished."
echo "- BUILD_SERVER=${BUILD_SERVER}"
echo "- BUILD_CLIENT=${BUILD_CLIENT}"
echo "- binaries in ${PROJECT_ROOT}/bin"

