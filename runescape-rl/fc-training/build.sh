#!/bin/bash
set -e

# Build Fight Caves for PufferLib 4.0 training.
#
# Usage:
#   ./build.sh                  # Default CUDA build
#   ./build.sh --cpu            # CPU-only (no CUDA)
#   ./build.sh --local          # Standalone debug binary
#   ./build.sh --fast           # Standalone optimized binary
#
# Prerequisites:
#   - PufferLib 4.0 cloned at $PUFFERLIB_DIR
#   - Python with pybind11, numpy installed
#   - CUDA toolkit (for GPU build) or --cpu flag

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(dirname "$SCRIPT_DIR")"
CUDA_ARCH_SH="$SCRIPT_DIR/cuda_arch.sh"
DEFAULT_PUFFERLIB_DIR="$(cd "$REPO_DIR/.." && pwd)/pufferlib_4"
PUFFERLIB_DIR="${PUFFERLIB_DIR:-$DEFAULT_PUFFERLIB_DIR}"
VENV_DIR="$REPO_DIR/.venv"

if [ ! -f "$CUDA_ARCH_SH" ]; then
    echo "Error: CUDA architecture helper not found at $CUDA_ARCH_SH" >&2
    exit 1
fi
source "$CUDA_ARCH_SH"

python_has_training_deps() {
    "$1" -c "import numpy, pybind11, torch" >/dev/null 2>&1
}

if [ -z "${PYTHON_BIN:-}" ]; then
    if [ -x "$VENV_DIR/bin/python3" ] && python_has_training_deps "$VENV_DIR/bin/python3"; then
        PYTHON_BIN="$VENV_DIR/bin/python3"
    elif [ -x "$VENV_DIR/bin/python" ] && python_has_training_deps "$VENV_DIR/bin/python"; then
        PYTHON_BIN="$VENV_DIR/bin/python"
    elif command -v python3 >/dev/null 2>&1 && python_has_training_deps "$(command -v python3)"; then
        PYTHON_BIN="$(command -v python3)"
    elif command -v python >/dev/null 2>&1 && python_has_training_deps "$(command -v python)"; then
        PYTHON_BIN="$(command -v python)"
    else
        echo "Error: no usable python found. Install numpy, pybind11, and torch in $VENV_DIR or set PYTHON_BIN." >&2
        exit 1
    fi
elif ! python_has_training_deps "$PYTHON_BIN"; then
    echo "Error: PYTHON_BIN=$PYTHON_BIN is missing numpy, pybind11, or torch." >&2
    exit 1
fi
export PYTHON_BIN
case "$PYTHON_BIN" in
    "$VENV_DIR"/*)
        export VIRTUAL_ENV="$VENV_DIR"
        export PATH="$VENV_DIR/bin:$PATH"
        ;;
    *)
        unset VIRTUAL_ENV
        ;;
esac

if [ ! -d "$PUFFERLIB_DIR/src" ]; then
    echo "Error: PufferLib not found at $PUFFERLIB_DIR"
    echo "Set PUFFERLIB_DIR or clone PufferLib 4.0"
    exit 1
fi

# Parse args
MODE=""
PRECISION=""
RENDER=""
for arg in "$@"; do
    case $arg in
        --cpu)    MODE=cpu; PRECISION="-DPRECISION_FLOAT" ;;
        --local)  MODE=local ;;
        --fast)   MODE=fast ;;
        --float)  PRECISION="-DPRECISION_FLOAT" ;;
        --debug)  DEBUG=1 ;;
        --render) RENDER=1 ;;
        *) echo "Unknown arg: $arg" && exit 1 ;;
    esac
done

# Render mode: add FC_RENDER define and fc-viewer include path
RENDER_FLAGS=""
RENDER_LIBS=""
VIEWER_SRC_DIR="$REPO_DIR/fc-viewer/src"
if [ -n "$RENDER" ]; then
    RENDER_FLAGS="-DFC_RENDER -I$VIEWER_SRC_DIR"
    # Use full paths for X11 libs (dev symlinks may not exist without -dev packages)
    X11_LIB="/usr/lib/x86_64-linux-gnu"
    RENDER_LIBS="-ldl -lX11 $X11_LIB/libXrandr.so.2 $X11_LIB/libXinerama.so.1 $X11_LIB/libXcursor.so.1 $X11_LIB/libXi.so.6"
    echo "Render mode: Raylib rendering enabled"
fi

ENV=fight_caves
SRC_DIR="$SCRIPT_DIR"
BINDING_SRC="$SRC_DIR/binding.c"

# Work from PufferLib directory (so relative paths to src/ work)
cd "$PUFFERLIB_DIR"

# Raylib
RAYLIB_NAME='raylib-5.5_linux_amd64'
if [ ! -d "$RAYLIB_NAME" ]; then
    echo "Downloading Raylib..."
    curl -sL "https://github.com/raysan5/raylib/releases/download/5.5/$RAYLIB_NAME.tar.gz" \
        -o "$RAYLIB_NAME.tar.gz" && tar xf "$RAYLIB_NAME.tar.gz" && rm "$RAYLIB_NAME.tar.gz"
fi
RAYLIB_A="$RAYLIB_NAME/lib/libraylib.a"

# Compiler settings
CLANG_WARN=(-Wall -Werror=return-type -Wno-unused-but-set-variable)
FC_CORE_INCLUDE="$REPO_DIR/fc-core/include"
FC_CORE_SRC="$REPO_DIR/fc-core/src"

if [ -n "$DEBUG" ] || [ "$MODE" = "local" ]; then
    CLANG_OPT=(-g -O0 "${CLANG_WARN[@]}")
else
    CLANG_OPT=(-O2 -DNDEBUG "${CLANG_WARN[@]}")
fi

ACTIVE_LOADOUT_FLAGS=()
if [ -n "${FC_ACTIVE_LOADOUT:-}" ]; then
    case "$FC_ACTIVE_LOADOUT" in
        FC_LOADOUT_*|[0-9]*)
            ACTIVE_LOADOUT_FLAGS=(-DFC_ACTIVE_LOADOUT="$FC_ACTIVE_LOADOUT")
            echo "Active loadout override: $FC_ACTIVE_LOADOUT"
            ;;
        *)
            echo "Error: invalid FC_ACTIVE_LOADOUT '$FC_ACTIVE_LOADOUT'" >&2
            exit 1
            ;;
    esac
fi

CONTRACT_VERSION_FLAGS=()
for contract_name in \
    FC_OBSERVATION_VERSION \
    FC_ACTION_VERSION \
    FC_REWARD_VERSION \
    FC_PRAYER_TIMING_VERSION; do
    contract_value="${!contract_name:-}"
    if [ -n "$contract_value" ]; then
        CONTRACT_VERSION_FLAGS+=("-D${contract_name}=\"${contract_value}\"")
    fi
done

mkdir -p build

# Standalone executable (for testing without Python)
if [ "$MODE" = "local" ] || [ "$MODE" = "fast" ]; then
    echo "Compiling standalone $ENV..."
    ${CC:-gcc} "${CLANG_OPT[@]}" \
        -I"$PUFFERLIB_DIR/src" -I"$SRC_DIR" -I"$FC_CORE_INCLUDE" -I"$FC_CORE_SRC" \
        -I"$RAYLIB_NAME/include" \
        -DPLATFORM_DESKTOP "${ACTIVE_LOADOUT_FLAGS[@]}" \
        "${CONTRACT_VERSION_FLAGS[@]}" $RENDER_FLAGS \
        "$SRC_DIR/$ENV.c" -o "$ENV" \
        "$RAYLIB_A" \
        /usr/lib/x86_64-linux-gnu/libGL.so.1 -lm -lpthread -fopenmp $RENDER_LIBS
    echo "Built: ./$ENV"
    exit 0
fi

# Compile static library from binding.c
STATIC_OBJ="build/libstatic_${ENV}.o"
STATIC_LIB="build/libstatic_${ENV}.a"

echo "Compiling static library for $ENV..."
${CC:-gcc} -c "${CLANG_OPT[@]}" \
    -I"$PUFFERLIB_DIR/src" -I"$SRC_DIR" -I"$FC_CORE_INCLUDE" -I"$FC_CORE_SRC" \
    -I"$RAYLIB_NAME/include" \
    -DPLATFORM_DESKTOP "${ACTIVE_LOADOUT_FLAGS[@]}" \
    "${CONTRACT_VERSION_FLAGS[@]}" $RENDER_FLAGS \
    -fno-semantic-interposition -fvisibility=hidden \
    -fPIC -fopenmp \
    "$BINDING_SRC" -o "$STATIC_OBJ"
ar rcs "$STATIC_LIB" "$STATIC_OBJ"

# Extract OBS_TENSOR_T from binding.c
OBS_TENSOR_T=$(awk '/^#define OBS_TENSOR_T/{print $3}' "$BINDING_SRC")
if [ -z "$OBS_TENSOR_T" ]; then
    echo "Error: Could not find OBS_TENSOR_T in $BINDING_SRC"
    exit 1
fi

# Python paths
PYTHON_INCLUDE=$("$PYTHON_BIN" -c "import sysconfig; print(sysconfig.get_path('include'))")
PYBIND_INCLUDE=$("$PYTHON_BIN" -c "import pybind11; print(pybind11.get_include())")
NUMPY_INCLUDE=$("$PYTHON_BIN" -c "import numpy; print(numpy.get_include())")
EXT_SUFFIX=$("$PYTHON_BIN" -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")
OUTPUT="pufferlib/_C${EXT_SUFFIX}"

if [ "$MODE" = "cpu" ]; then
    echo "Compiling CPU training backend..."
    ${CXX:-g++} -c -fPIC -fopenmp \
        -D_GLIBCXX_USE_CXX11_ABI=1 \
        -DPLATFORM_DESKTOP \
        -std=c++17 \
        -I. -Isrc \
        -I"$PYTHON_INCLUDE" -I"$PYBIND_INCLUDE" \
        -DOBS_TENSOR_T=$OBS_TENSOR_T \
        -DENV_NAME=$ENV \
        "${ACTIVE_LOADOUT_FLAGS[@]}" \
        $PRECISION -O2 \
        src/bindings_cpu.cpp -o build/bindings_cpu.o

    ${CXX:-g++} -shared -fPIC -fopenmp \
        build/bindings_cpu.o "$STATIC_LIB" "$RAYLIB_A" \
        -lm -lpthread -lgomp -O2 $RENDER_LIBS \
        -Bsymbolic-functions \
        -o "$OUTPUT"
    echo "Built: $OUTPUT"
else
    # CUDA build
    NVCC_BIN=""
    if [ -n "${CUDA_HOME:-}" ] && [ -x "$CUDA_HOME/bin/nvcc" ]; then
        NVCC_BIN="$CUDA_HOME/bin/nvcc"
    elif [ -n "${CUDA_PATH:-}" ] && [ -x "$CUDA_PATH/bin/nvcc" ]; then
        NVCC_BIN="$CUDA_PATH/bin/nvcc"
    elif command -v nvcc >/dev/null 2>&1; then
        NVCC_BIN="$(command -v nvcc)"
    elif [ -x /usr/local/cuda/bin/nvcc ]; then
        NVCC_BIN="/usr/local/cuda/bin/nvcc"
    else
        for candidate in /usr/local/cuda-*/bin/nvcc; do
            if [ -x "$candidate" ]; then
                NVCC_BIN="$candidate"
                break
            fi
        done
    fi

    if [ -z "$NVCC_BIN" ]; then
        echo "Error: nvcc not found. Set CUDA_HOME or add nvcc to PATH."
        exit 1
    fi

    CUDA_HOME="$(dirname "$(dirname "$NVCC_BIN")")"
    CUDNN_IFLAG=""
    CUDNN_LFLAG=""
    for dir in "$CUDA_HOME/include" /usr/local/cuda/include /usr/include; do
        [ -f "$dir/cudnn.h" ] && CUDNN_IFLAG="-I$dir" && break
    done
    for dir in "$CUDA_HOME/lib64" /usr/local/cuda/lib64 /usr/lib/x86_64-linux-gnu; do
        [ -f "$dir/libcudnn.so" ] && CUDNN_LFLAG="-L$dir" && break
    done
    if [ -z "$CUDNN_IFLAG" ]; then
        CUDNN_IFLAG=$("$PYTHON_BIN" -c "import nvidia.cudnn, os; print('-I' + os.path.join(nvidia.cudnn.__path__[0], 'include'))" 2>/dev/null || echo "")
    fi
    if [ -z "$CUDNN_LFLAG" ]; then
        CUDNN_LFLAG=$("$PYTHON_BIN" -c "import nvidia.cudnn, os; print('-L' + os.path.join(nvidia.cudnn.__path__[0], 'lib'))" 2>/dev/null || echo "")
    fi

    ARCH="$(fc_resolve_cuda_arch "${NVCC_ARCH:-}")" || exit 1

    NVCC="$NVCC_BIN"

    echo "Compiling CUDA ($ARCH) training backend..."
    # Build RENDER_NVCC_FLAGS: convert -DFC_RENDER -I/path to -Xcompiler= form
    RENDER_NVCC_FLAGS=""
    if [ -n "$RENDER" ]; then
        RENDER_NVCC_FLAGS="-Xcompiler=-DFC_RENDER -I$VIEWER_SRC_DIR"
    fi

    $NVCC -c -arch=$ARCH -Xcompiler -fPIC \
        -Xcompiler=-D_GLIBCXX_USE_CXX11_ABI=1 \
        -Xcompiler=-DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION \
        -Xcompiler=-DPLATFORM_DESKTOP \
        -std=c++17 \
        -I. -Isrc \
        -I"$PYTHON_INCLUDE" -I"$PYBIND_INCLUDE" -I"$NUMPY_INCLUDE" \
        -I"$CUDA_HOME/include" $CUDNN_IFLAG -I"$RAYLIB_NAME/include" \
        -Xcompiler=-fopenmp \
        -DOBS_TENSOR_T=$OBS_TENSOR_T \
        -DENV_NAME=$ENV \
        "${ACTIVE_LOADOUT_FLAGS[@]}" \
        $PRECISION $RENDER_NVCC_FLAGS -O2 --threads 0 \
        src/bindings.cu -o build/bindings.o

    # Find cuDNN from PyTorch's bundled copy if not system-installed.
    # Some wheels only ship versioned sonames (for example libcudnn.so.9)
    # without the linker-facing libcudnn.so symlink.
    VENV_CUDNN=$("$PYTHON_BIN" -c "import nvidia.cudnn, os; print('-L' + os.path.join(nvidia.cudnn.__path__[0], 'lib'))" 2>/dev/null || echo "")
    if [ -z "$CUDNN_LFLAG" ] && [ -n "$VENV_CUDNN" ]; then
        CUDNN_LFLAG="$VENV_CUDNN"
    fi

    CUDNN_LINK_INPUT="-lcudnn"
    if [ -n "$CUDNN_LFLAG" ]; then
        CUDNN_LIB_DIR="${CUDNN_LFLAG#-L}"
        if [ ! -e "$CUDNN_LIB_DIR/libcudnn.so" ]; then
            VERSIONED_CUDNN=$(find "$CUDNN_LIB_DIR" -maxdepth 1 -name 'libcudnn.so.*' | sort | head -n 1)
            if [ -n "$VERSIONED_CUDNN" ]; then
                CUDNN_LINK_INPUT="$VERSIONED_CUDNN"
                echo "Using versioned cuDNN link target: $VERSIONED_CUDNN"
            fi
        fi
    fi

    NVML_LINK_INPUT="-lnvidia-ml"
    if ! ${CXX:-g++} -Wl,--no-as-needed -lnvidia-ml -Wl,--as-needed -x c++ - -o /tmp/fc_nvml_link_check >/dev/null 2>&1 <<<'int main(){return 0;}'; then
        for dir in "$CUDA_HOME/lib64" /usr/lib/x86_64-linux-gnu /lib/x86_64-linux-gnu; do
            VERSIONED_NVML=$(find "$dir" -maxdepth 1 -name 'libnvidia-ml.so*' 2>/dev/null | sort | head -n 1)
            if [ -n "$VERSIONED_NVML" ]; then
                NVML_LINK_INPUT="$VERSIONED_NVML"
                echo "Using versioned NVML link target: $VERSIONED_NVML"
                break
            fi
        done
    fi

    ${CXX:-g++} -shared -fPIC -fopenmp \
        build/bindings.o "$STATIC_LIB" "$RAYLIB_A" \
        -L"$CUDA_HOME/lib64" $CUDNN_LFLAG \
        -lcudart -lnccl $NVML_LINK_INPUT -lcublas -lcusolver -lcurand $CUDNN_LINK_INPUT \
        -lgomp -O2 $RENDER_LIBS \
        -Bsymbolic-functions \
        -o "$OUTPUT"
    CUOBJDUMP_BIN="$(fc_find_cuobjdump "$NVCC_BIN")" || exit 1
    fc_verify_cuda_binary_arch "$OUTPUT" "$ARCH" "$CUOBJDUMP_BIN" || exit 1
    echo "Verified CUDA backend device code: $ARCH"
    echo "Built: $OUTPUT"
fi

echo "Done. Run: cd $PUFFERLIB_DIR && puffer train fight_caves"
