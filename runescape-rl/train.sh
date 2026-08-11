#!/bin/bash
# Train Fight Caves RL agent with wandb logging.
# Usage: ./train.sh [--no-wandb]
# Optional:
#   LOAD_MODEL_PATH=/path/to/checkpoint.bin ./train.sh
#   LOAD_MODEL_PATH=latest ./train.sh

SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SRC_DIR/.." && pwd)"
PUFFER_DIR="${PUFFER_DIR:-$ROOT_DIR/pufferlib_4}"
CONFIG_PATH="${CONFIG_PATH:-$SRC_DIR/config/fight_caves.ini}"
TRAINING_BUILD_SH="$SRC_DIR/fc-training/build.sh"
CUDA_ARCH_SH="$SRC_DIR/fc-training/cuda_arch.sh"
CONTRACT_PREFLIGHT="$SRC_DIR/tools/validation/contract_preflight.py"

if [ ! -d "$PUFFER_DIR" ]; then
    echo "Error: PufferLib not found at $PUFFER_DIR"
    exit 1
fi

if [ ! -f "$CONFIG_PATH" ]; then
    echo "Error: config not found at $CONFIG_PATH"
    exit 1
fi

if [ ! -f "$TRAINING_BUILD_SH" ]; then
    echo "Error: local training backend build script not found at $TRAINING_BUILD_SH"
    exit 1
fi
if [ ! -f "$CUDA_ARCH_SH" ]; then
    echo "Error: CUDA architecture helper not found at $CUDA_ARCH_SH"
    exit 1
fi
if [ ! -f "$CONTRACT_PREFLIGHT" ]; then
    echo "Error: compiled-contract preflight helper not found at $CONTRACT_PREFLIGHT" >&2
    exit 1
fi
source "$CUDA_ARCH_SH"

# Sync config to where PufferLib reads it
cp "$CONFIG_PATH" "$PUFFER_DIR/config/fight_caves.ini"
echo "[train.sh] Synced config from $CONFIG_PATH to $PUFFER_DIR/config/fight_caves.ini"

mkdir -p \
    "$PUFFER_DIR/checkpoints" \
    "$PUFFER_DIR/logs/fight_caves" \
    "$PUFFER_DIR/wandb"

cd "$PUFFER_DIR"
VENV_DIR="$SRC_DIR/.venv"
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
        export PATH="$VENV_DIR/bin:/usr/local/cuda/bin:$PATH"
        ;;
    *)
        unset VIRTUAL_ENV
        export PATH="/usr/local/cuda/bin:$PATH"
        ;;
esac
export PUFFERLIB_DIR="$PUFFER_DIR"
export FC_COLLISION_PATH="$SRC_DIR/fc-core/assets/fightcaves.collision"
export FC_LOS_PATH="$SRC_DIR/fc-core/assets/fightcaves.los"
export WANDB_DIR="$PUFFER_DIR/wandb"
export WANDB_CACHE_DIR="$PUFFER_DIR/wandb/.cache"
export WANDB_CONFIG_DIR="$PUFFER_DIR/wandb/.config"
export WANDB_DATA_DIR="$PUFFER_DIR/wandb/.data"
export WANDB_PROJECT="${WANDB_PROJECT:-fight caves rl}"
CUDNN_LIB="$("$PYTHON_BIN" -c "import nvidia.cudnn, os; print(os.path.join(nvidia.cudnn.__path__[0], 'lib'))" 2>/dev/null || true)"
if [ -n "$CUDNN_LIB" ]; then
    export LD_LIBRARY_PATH="$CUDNN_LIB${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
export CC="${CC:-gcc}"
export CXX="${CXX:-g++}"

MODE="train"
if [ "$#" -gt 0 ]; then
    case "$1" in
        train|eval|sweep|paretosweep)
            MODE="$1"
            shift
            ;;
    esac
fi

WANDB_FLAG="--wandb"
EXTRA_ARGS=()
HELP_REQUESTED=0
for arg in "$@"; do
    if [ "$arg" = "--no-wandb" ]; then
        WANDB_FLAG=""
    else
        EXTRA_ARGS+=("$arg")
    fi
    if [ "$arg" = "--help" ] || [ "$arg" = "-h" ]; then
        HELP_REQUESTED=1
    fi
done

# Help only needs the Python command surface. Never replace a working backend
# as a side effect of inspecting CLI options.
if [ "$HELP_REQUESTED" = "1" ]; then
    exec "$PYTHON_BIN" -m pufferlib.pufferl "$MODE" fight_caves "${EXTRA_ARGS[@]}"
fi

CONTRACT_VALUES_OUTPUT="$(
    "$PYTHON_BIN" "$CONTRACT_PREFLIGHT" config-values \
        --config-path "$CONFIG_PATH"
)" || exit 1
mapfile -t SELECTED_CONTRACT_VALUES <<< "$CONTRACT_VALUES_OUTPUT"
if [ "${#SELECTED_CONTRACT_VALUES[@]}" -ne 4 ]; then
    echo "Error: config contract helper returned ${#SELECTED_CONTRACT_VALUES[@]} values, expected 4" >&2
    exit 1
fi
export FC_OBSERVATION_VERSION="${SELECTED_CONTRACT_VALUES[0]}"
export FC_ACTION_VERSION="${SELECTED_CONTRACT_VALUES[1]}"
export FC_REWARD_VERSION="${SELECTED_CONTRACT_VALUES[2]}"
export FC_PRAYER_TIMING_VERSION="${SELECTED_CONTRACT_VALUES[3]}"

BACKEND_SOURCE_SHA256="$(
    "$PYTHON_BIN" "$CONTRACT_PREFLIGHT" source-hash \
        --runescape-dir "$SRC_DIR" \
        --puffer-dir "$PUFFER_DIR"
)" || exit 1

NVCC_BIN="$(command -v nvcc 2>/dev/null || true)"
if [ -z "$NVCC_BIN" ]; then
    echo "Error: nvcc not found. Add CUDA to PATH or set CUDA_HOME." >&2
    exit 1
fi
EXPECTED_CUDA_ARCH="$(fc_resolve_cuda_arch "${NVCC_ARCH:-}")" || exit 1
export NVCC_ARCH="$EXPECTED_CUDA_ARCH"
CUOBJDUMP_BIN="$(fc_find_cuobjdump "$NVCC_BIN")" || exit 1

EXT_SUFFIX="$("$PYTHON_BIN" -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX') or '')")"
BACKEND_SO="$PUFFER_DIR/pufferlib/_C${EXT_SUFFIX}"
BACKEND_STAMP="$PUFFER_DIR/build/fight_caves_build.env"
ACTIVE_LOADOUT_KEY="${FC_ACTIVE_LOADOUT:-FC_LOADOUT_SOTA_TBOW}"
BACKEND_REBUILD_REASON=""
if [ ! -f "$BACKEND_SO" ]; then
    BACKEND_REBUILD_REASON="missing backend"
elif ! "$PYTHON_BIN" -c "import importlib.util, sys; spec=importlib.util.spec_from_file_location('pufferlib._C', sys.argv[1]); mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod); ok=(getattr(mod, 'env_name', None) == 'fight_caves' and hasattr(mod, 'create_pufferl')); raise SystemExit(0 if ok else 1)" "$BACKEND_SO"; then
    BACKEND_REBUILD_REASON="backend missing compiled trainer API"
elif ! fc_verify_cuda_binary_arch \
        "$BACKEND_SO" "$EXPECTED_CUDA_ARCH" "$CUOBJDUMP_BIN" \
        >/dev/null 2>&1; then
    BACKEND_REBUILD_REASON="backend does not contain $EXPECTED_CUDA_ARCH device code"
elif find "$SRC_DIR/fc-core" "$SRC_DIR/fc-training" \
        "$PUFFER_DIR/src/vecenv.h" "$PUFFER_DIR/src/pufferlib.cu" \
        "$PUFFER_DIR/src/bindings.cu" \
        -type f -newer "$BACKEND_SO" -print -quit | grep -q .; then
    BACKEND_REBUILD_REASON="backend sources newer than compiled extension"
elif [ "${FORCE_BACKEND_REBUILD:-0}" = "1" ]; then
    BACKEND_REBUILD_REASON="FORCE_BACKEND_REBUILD=1"
elif [ ! -f "$BACKEND_STAMP" ]; then
    BACKEND_REBUILD_REASON="missing backend build stamp"
elif ! grep -Fxq "FC_ACTIVE_LOADOUT=$ACTIVE_LOADOUT_KEY" "$BACKEND_STAMP"; then
    BACKEND_REBUILD_REASON="FC_ACTIVE_LOADOUT changed to $ACTIVE_LOADOUT_KEY"
elif ! grep -Fxq "NVCC_ARCH=$EXPECTED_CUDA_ARCH" "$BACKEND_STAMP"; then
    BACKEND_REBUILD_REASON="missing or stale CUDA architecture build stamp"
elif ! grep -Fxq "SOURCE_SHA256=$BACKEND_SOURCE_SHA256" "$BACKEND_STAMP"; then
    BACKEND_REBUILD_REASON="backend source identity changed"
elif ! grep -Fxq "FC_OBSERVATION_VERSION=$FC_OBSERVATION_VERSION" "$BACKEND_STAMP"; then
    BACKEND_REBUILD_REASON="observation contract changed"
elif ! grep -Fxq "FC_ACTION_VERSION=$FC_ACTION_VERSION" "$BACKEND_STAMP"; then
    BACKEND_REBUILD_REASON="action contract changed"
elif ! grep -Fxq "FC_REWARD_VERSION=$FC_REWARD_VERSION" "$BACKEND_STAMP"; then
    BACKEND_REBUILD_REASON="reward contract changed"
elif ! grep -Fxq "FC_PRAYER_TIMING_VERSION=$FC_PRAYER_TIMING_VERSION" "$BACKEND_STAMP"; then
    BACKEND_REBUILD_REASON="Prayer timing contract changed"
elif ! grep -Eq '^CC_VERSION=.' "$BACKEND_STAMP" \
        || ! grep -Eq '^CXX_VERSION=.' "$BACKEND_STAMP"; then
    BACKEND_REBUILD_REASON="missing compiler build identity"
fi

if [ -n "$BACKEND_REBUILD_REASON" ]; then
    echo "[train.sh] Rebuilding fight_caves backend: $BACKEND_REBUILD_REASON"
    bash "$TRAINING_BUILD_SH"
    fc_verify_cuda_binary_arch \
        "$BACKEND_SO" "$EXPECTED_CUDA_ARCH" "$CUOBJDUMP_BIN" || exit 1
    mkdir -p "$(dirname "$BACKEND_STAMP")"
    {
        echo "FC_ACTIVE_LOADOUT=$ACTIVE_LOADOUT_KEY"
        echo "FC_OBSERVATION_VERSION=$FC_OBSERVATION_VERSION"
        echo "FC_ACTION_VERSION=$FC_ACTION_VERSION"
        echo "FC_REWARD_VERSION=$FC_REWARD_VERSION"
        echo "FC_PRAYER_TIMING_VERSION=$FC_PRAYER_TIMING_VERSION"
        echo "BUILD_MODE=cuda"
        echo "SOURCE_SHA256=$BACKEND_SOURCE_SHA256"
        echo "PYTHON_BIN=$PYTHON_BIN"
        echo "CC=$CC"
        echo "CC_VERSION=$($CC --version | head -n 1)"
        echo "CXX=$CXX"
        echo "CXX_VERSION=$($CXX --version | head -n 1)"
        echo "NVCC_ARCH=$EXPECTED_CUDA_ARCH"
        echo "NVCC_VERSION=$($NVCC_BIN --version | tail -n 1)"
        echo "BACKEND_SHA256=$(sha256sum "$BACKEND_SO" | awk '{print $1}')"
    } > "$BACKEND_STAMP"
fi

CONTRACT_PREFLIGHT_PATH="${FC_CONTRACT_PREFLIGHT_PATH:-$PUFFER_DIR/build/fight_caves_contract_$$.json}"
if ! "$PYTHON_BIN" "$CONTRACT_PREFLIGHT" check \
    --backend-so "$BACKEND_SO" \
    --config-path "$CONFIG_PATH" \
    --synced-config-path "$PUFFER_DIR/config/fight_caves.ini" \
    --active-loadout "$ACTIVE_LOADOUT_KEY" \
    --output-path "$CONTRACT_PREFLIGHT_PATH"; then
    echo "Error: Fight Caves compiled-contract preflight failed" >&2
    exit 1
fi
echo "[train.sh] Compiled-contract preflight passed: $CONTRACT_PREFLIGHT_PATH"

CMD=("$PYTHON_BIN" -m pufferlib.pufferl "$MODE" fight_caves)
if [ -n "$WANDB_FLAG" ]; then
    CMD+=("$WANDB_FLAG")
fi
CMD+=(--wandb-project "$WANDB_PROJECT")
if [ -n "${WANDB_TAG:-}" ]; then
    CMD+=(--tag "$WANDB_TAG")
fi

CHECKPOINT_ROOT="${FC_CHECKPOINT_ROOT:-$PUFFER_DIR/checkpoints}"
CHECKPOINT_PREP_OUTPUT="$(
    "$PYTHON_BIN" "$CONTRACT_PREFLIGHT" prepare-checkpoint-dir \
        --checkpoint-root "$CHECKPOINT_ROOT" \
        --preflight-path "$CONTRACT_PREFLIGHT_PATH"
)" || exit 1
mapfile -t CHECKPOINT_PREP_VALUES <<< "$CHECKPOINT_PREP_OUTPUT"
if [ "${#CHECKPOINT_PREP_VALUES[@]}" -ne 2 ]; then
    echo "Error: checkpoint directory helper returned ${#CHECKPOINT_PREP_VALUES[@]} values, expected 2" >&2
    exit 1
fi
CONTRACT_CHECKPOINT_DIR="${CHECKPOINT_PREP_VALUES[0]}"
CHECKPOINT_CONTRACT_SIDECAR="${CHECKPOINT_PREP_VALUES[1]}"
CMD+=(--checkpoint-dir "$CONTRACT_CHECKPOINT_DIR")
echo "[train.sh] Contract checkpoint directory: $CONTRACT_CHECKPOINT_DIR"

CHECKPOINT_REQUEST_MODE="cold"
CHECKPOINT_RESOLUTION_PATH=""
if [ -n "${LOAD_MODEL_PATH:-}" ]; then
    if [ "$LOAD_MODEL_PATH" = "latest" ]; then
        CHECKPOINT_REQUEST_MODE="latest"
    else
        CHECKPOINT_REQUEST_MODE="explicit"
    fi
    CHECKPOINT_RESOLUTION_PATH="${FC_CHECKPOINT_RESOLUTION_PATH:-$PUFFER_DIR/build/fight_caves_checkpoint_$$.json}"
    RESOLVE_ARGS=(
        resolve-checkpoint
        --request "$CHECKPOINT_REQUEST_MODE"
        --checkpoint-root "$CHECKPOINT_ROOT"
        --preflight-path "$CONTRACT_PREFLIGHT_PATH"
        --config-path "$CONFIG_PATH"
        --default-config-path "$PUFFER_DIR/config/default.ini"
        --output-path "$CHECKPOINT_RESOLUTION_PATH"
    )
    if [ "$CHECKPOINT_REQUEST_MODE" = "explicit" ]; then
        RESOLVE_ARGS+=(--checkpoint-path "$LOAD_MODEL_PATH")
    fi
    CHECKPOINT_RESOLVE_OUTPUT="$(
        "$PYTHON_BIN" "$CONTRACT_PREFLIGHT" "${RESOLVE_ARGS[@]}"
    )" || {
        echo "Error: Fight Caves checkpoint resolution failed" >&2
        exit 1
    }
    mapfile -t CHECKPOINT_RESOLVE_VALUES <<< "$CHECKPOINT_RESOLVE_OUTPUT"
    if [ "${#CHECKPOINT_RESOLVE_VALUES[@]}" -ne 2 ]; then
        echo "Error: checkpoint resolver returned ${#CHECKPOINT_RESOLVE_VALUES[@]} values, expected 2" >&2
        exit 1
    fi
    RESOLVED_CHECKPOINT="${CHECKPOINT_RESOLVE_VALUES[1]}"
    echo "[train.sh] Using validated warm-start checkpoint: $RESOLVED_CHECKPOINT"
    CMD+=(--load-model-path "$RESOLVED_CHECKPOINT")
fi
if [ "${#EXTRA_ARGS[@]}" -gt 0 ]; then
    CMD+=("${EXTRA_ARGS[@]}")
fi

RUN_MANIFEST_DIR="${FC_RUN_MANIFEST_DIR:-$PUFFER_DIR/logs/fight_caves/manifests}"
RUN_MANIFEST_TS="$(date -u +%Y%m%dT%H%M%SZ)"
RUN_MANIFEST_PATH="${FC_RUN_MANIFEST_PATH:-$RUN_MANIFEST_DIR/${RUN_MANIFEST_TS}-${MODE}-$$.json}"
export FC_RUN_MANIFEST_PATH="$RUN_MANIFEST_PATH"
if grep -Eq '^\[run\]' "$PUFFER_DIR/config/fight_caves.ini" \
    && grep -Eq '^manifest_path[[:space:]]*=' "$PUFFER_DIR/config/fight_caves.ini"; then
    CMD+=(--run.manifest-path "$RUN_MANIFEST_PATH")
fi
MANIFEST_ARGS=(
    --repo-root "$ROOT_DIR" \
    --runescape-dir "$SRC_DIR" \
    --puffer-dir "$PUFFER_DIR" \
    --config-path "$CONFIG_PATH" \
    --synced-config-path "$PUFFER_DIR/config/fight_caves.ini" \
    --backend-stamp "$BACKEND_STAMP" \
    --backend-path "$BACKEND_SO" \
    --contract-path "$CONTRACT_PREFLIGHT_PATH" \
    --checkpoint-request-mode "$CHECKPOINT_REQUEST_MODE" \
    --active-loadout "$ACTIVE_LOADOUT_KEY" \
    --python-bin "$PYTHON_BIN" \
    --mode "$MODE" \
    --output-path "$RUN_MANIFEST_PATH"
)
if [ -n "$CHECKPOINT_RESOLUTION_PATH" ]; then
    MANIFEST_ARGS+=(--checkpoint-resolution-path "$CHECKPOINT_RESOLUTION_PATH")
fi
if ! "$PYTHON_BIN" "$SRC_DIR/tools/validation/run_manifest.py" \
    "${MANIFEST_ARGS[@]}" -- "${CMD[@]}"; then
    echo "Error: failed to write run manifest at $RUN_MANIFEST_PATH" >&2
    exit 1
fi
echo "[train.sh] Wrote run manifest: $RUN_MANIFEST_PATH"

"${CMD[@]}"
