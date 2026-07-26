#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RUNESCAPE_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
source "$RUNESCAPE_DIR/fc-training/cuda_arch.sh"

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

[ "$(fc_resolve_cuda_arch sm_120)" = "sm_120" ] \
    || fail "explicit sm_120 architecture was not preserved"
if fc_resolve_cuda_arch native >/dev/null 2>&1; then
    fail "native architecture fallback was accepted"
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

printf '%s\n' \
    '#!/bin/bash' \
    "printf '%s\\n' '12.0'" \
    > "$TMP_DIR/nvidia-smi-good"
printf '%s\n' \
    '#!/bin/bash' \
    'exit 1' \
    > "$TMP_DIR/nvidia-smi-fail"
printf '%s\n' \
    '#!/bin/bash' \
    "printf '%s\\n' 'ELF file    1: backend.1.sm_120.cubin'" \
    > "$TMP_DIR/cuobjdump-good"
printf '%s\n' \
    '#!/bin/bash' \
    "printf '%s\\n' 'ELF file    1: backend.1.sm_75.cubin'" \
    > "$TMP_DIR/cuobjdump-wrong"
chmod +x "$TMP_DIR"/nvidia-smi-* "$TMP_DIR"/cuobjdump-*
touch "$TMP_DIR/backend.so"

[ "$(FC_NVIDIA_SMI_BIN="$TMP_DIR/nvidia-smi-good" fc_resolve_cuda_arch)" = "sm_120" ] \
    || fail "12.0 compute capability did not resolve to sm_120"
if FC_NVIDIA_SMI_BIN="$TMP_DIR/nvidia-smi-fail" fc_resolve_cuda_arch \
        >/dev/null 2>&1; then
    fail "failed GPU detection did not stop architecture resolution"
fi

fc_verify_cuda_binary_arch \
    "$TMP_DIR/backend.so" sm_120 "$TMP_DIR/cuobjdump-good" \
    || fail "matching sm_120 cubin was rejected"
if fc_verify_cuda_binary_arch \
        "$TMP_DIR/backend.so" sm_120 "$TMP_DIR/cuobjdump-wrong" \
        >/dev/null 2>&1; then
    fail "mismatched sm_75 cubin was accepted for sm_120"
fi

echo "PASS: CUDA architecture detection fails closed and verifies device code"
