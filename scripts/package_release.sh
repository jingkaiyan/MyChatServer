#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
DIST_DIR="${PROJECT_ROOT}/dist"

mkdir -p "${BUILD_DIR}" "${DIST_DIR}"

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
  -DBUILD_SERVER=OFF \
  -DBUILD_CLIENT=ON

cmake --build "${BUILD_DIR}" -j"$(nproc)"

pushd "${BUILD_DIR}" >/dev/null
cpack -G TGZ
popd >/dev/null

cp -f "${BUILD_DIR}"/*.tar.gz "${DIST_DIR}"/

echo "Release packages generated in ${DIST_DIR}"
