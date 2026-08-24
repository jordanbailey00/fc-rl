#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
PUFFER_DIR="${PUFFER_DIR:-$(cd "$REPO_DIR/.." && pwd)/pufferlib_4}"
PYTHON_BIN="${PYTHON_BIN:-python3}"
SOURCE="$SCRIPT_DIR/parity_training_health_probe.cu"
BUILD_DIR="$REPO_DIR/build/parity-training-health"
STATIC_LIB="$PUFFER_DIR/build/libstatic_fight_caves.a"
CUDA_ARCH_SH="$REPO_DIR/fc-training/cuda_arch.sh"

if [ ! -f "$STATIC_LIB" ]; then
    echo "Error: build the canonical Fight Caves CUDA backend first" >&2
    exit 1
fi
if [ ! -f "$CUDA_ARCH_SH" ]; then
    echo "Error: missing CUDA architecture helper: $CUDA_ARCH_SH" >&2
    exit 1
fi

NVCC_BIN="${NVCC:-${CUDA_HOME:-/usr/local/cuda}/bin/nvcc}"
if [ ! -x "$NVCC_BIN" ]; then
    NVCC_BIN="$(command -v nvcc || true)"
fi
if [ -z "$NVCC_BIN" ] || [ ! -x "$NVCC_BIN" ]; then
    echo "Error: nvcc not found" >&2
    exit 1
fi
CUDA_ROOT="$(cd "$(dirname "$NVCC_BIN")/.." && pwd)"

source "$CUDA_ARCH_SH"
STAMP_ARCH=""
if [ -f "$PUFFER_DIR/build/fight_caves_build.env" ]; then
    STAMP_ARCH="$(sed -n 's/^NVCC_ARCH=//p' "$PUFFER_DIR/build/fight_caves_build.env" | head -n 1)"
fi
ARCH="$(fc_resolve_cuda_arch "${NVCC_ARCH:-$STAMP_ARCH}")"

PYTHON_INCLUDE="$($PYTHON_BIN -c 'import sysconfig; print(sysconfig.get_path("include"))')"
PYBIND_INCLUDE="$($PYTHON_BIN -c 'import pybind11; print(pybind11.get_include())')"
NUMPY_INCLUDE="$($PYTHON_BIN -c 'import numpy; print(numpy.get_include())')"
EXT_SUFFIX="$($PYTHON_BIN -c 'import sysconfig; print(sysconfig.get_config_var("EXT_SUFFIX") or ".so")')"
OUTPUT="$BUILD_DIR/_fc_train_health_probe${EXT_SUFFIX}"
OBJECT="$BUILD_DIR/parity_training_health_probe.o"

mkdir -p "$BUILD_DIR"
PUFFER_SOURCE_NEWER="$(
    find "$PUFFER_DIR/src" -type f -newer "$OUTPUT" -print -quit 2>/dev/null || true
)"
if [ -f "$OUTPUT" ] \
        && [ "$OUTPUT" -nt "$SOURCE" ] \
        && [ "$OUTPUT" -nt "$STATIC_LIB" ] \
        && [ -z "$PUFFER_SOURCE_NEWER" ]; then
    echo "$OUTPUT"
    exit 0
fi

CUDNN_INCLUDE="$($PYTHON_BIN -c 'import nvidia.cudnn, os; print(os.path.join(nvidia.cudnn.__path__[0], "include"))' 2>/dev/null || true)"
CUDNN_LIB="$($PYTHON_BIN -c 'import nvidia.cudnn, os; print(os.path.join(nvidia.cudnn.__path__[0], "lib"))' 2>/dev/null || true)"
CUDNN_LINK="-lcudnn"
if [ -n "$CUDNN_LIB" ] && [ ! -e "$CUDNN_LIB/libcudnn.so" ]; then
    CUDNN_VERSIONED="$(find "$CUDNN_LIB" -maxdepth 1 -name 'libcudnn.so.*' | sort | head -n 1)"
    if [ -n "$CUDNN_VERSIONED" ]; then CUDNN_LINK="$CUDNN_VERSIONED"; fi
fi

NVML_LINK="-lnvidia-ml"
for directory in "$CUDA_ROOT/lib64" /usr/lib/x86_64-linux-gnu /lib/x86_64-linux-gnu; do
    candidate="$(find "$directory" -maxdepth 1 -name 'libnvidia-ml.so*' 2>/dev/null | sort | head -n 1)"
    if [ -n "$candidate" ]; then NVML_LINK="$candidate"; break; fi
done

INCLUDES=(
    -I"$PUFFER_DIR" -I"$PUFFER_DIR/src"
    -I"$PYTHON_INCLUDE" -I"$PYBIND_INCLUDE" -I"$NUMPY_INCLUDE"
    -I"$CUDA_ROOT/include"
)
if [ -n "$CUDNN_INCLUDE" ]; then INCLUDES+=(-I"$CUDNN_INCLUDE"); fi

"$NVCC_BIN" -c -arch="$ARCH" -Xcompiler -fPIC \
    -Xcompiler=-D_GLIBCXX_USE_CXX11_ABI=1 \
    -Xcompiler=-DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION \
    -Xcompiler=-fopenmp \
    -std=c++17 "${INCLUDES[@]}" \
    -DOBS_TENSOR_T=FloatTensor -DENV_NAME=fight_caves \
    -O2 --threads 0 "$SOURCE" -o "$OBJECT"

LINK_ARGS=(-L"$CUDA_ROOT/lib64")
if [ -n "$CUDNN_LIB" ]; then LINK_ARGS+=(-L"$CUDNN_LIB"); fi
"${CXX:-g++}" -shared -fPIC -fopenmp \
    "$OBJECT" "$STATIC_LIB" \
    "${LINK_ARGS[@]}" -lcudart -lnccl "$NVML_LINK" \
    -lcublas -lcusolver -lcurand "$CUDNN_LINK" -lgomp -O2 \
    -Bsymbolic-functions -o "$OUTPUT"

echo "$OUTPUT"
