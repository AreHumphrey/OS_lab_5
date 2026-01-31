#!/usr/bin/env bash
set -euo pipefail

# Usage: ./scripts/build.sh [BuildType]
# Default: Debug. Requires cmake and C++17 toolchain.

BUILD_TYPE="${1:-Debug}"
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
LAB5_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"

echo "========================================"
echo " Building lab5 (Linux) "
echo "========================================"
echo "Build type: ${BUILD_TYPE}"
echo "Project dir: ${LAB5_ROOT}"
echo ""

mkdir -p "${LAB5_ROOT}/build"
cd "${LAB5_ROOT}/build"

cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" ..
make -j4

echo ""
echo "========================================"
echo " Build completed successfully "
echo " Executable: ${LAB5_ROOT}/build/lab5_main"
echo "========================================"
