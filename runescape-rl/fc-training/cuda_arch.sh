#!/bin/bash

# Shared CUDA architecture detection and compiled-backend verification.

fc_validate_cuda_arch() {
    local arch="${1:-}"
    if [[ "$arch" =~ ^sm_[0-9]+[a-z]?$ ]]; then
        return 0
    fi

    echo "Error: invalid CUDA architecture '$arch' (expected values such as sm_120)." >&2
    return 1
}

fc_resolve_cuda_arch() {
    local requested="${1:-}"
    if [ -n "$requested" ]; then
        fc_validate_cuda_arch "$requested" || return 1
        printf '%s\n' "$requested"
        return 0
    fi

    local nvidia_smi="${FC_NVIDIA_SMI_BIN:-}"
    if [ -z "$nvidia_smi" ]; then
        nvidia_smi="$(command -v nvidia-smi 2>/dev/null || true)"
    fi
    if [ -z "$nvidia_smi" ] || [ ! -x "$nvidia_smi" ]; then
        echo "Error: cannot detect GPU compute capability because nvidia-smi is unavailable." >&2
        echo "Set NVCC_ARCH explicitly (for example, NVCC_ARCH=sm_120)." >&2
        return 1
    fi

    local output
    if ! output="$($nvidia_smi --query-gpu=compute_cap --format=csv,noheader 2>/dev/null)"; then
        echo "Error: nvidia-smi could not query GPU compute capability." >&2
        echo "Set NVCC_ARCH explicitly (for example, NVCC_ARCH=sm_120)." >&2
        return 1
    fi

    local compute_cap="${output%%$'\n'*}"
    compute_cap="${compute_cap//[[:space:]]/}"
    local arch
    if [[ "$compute_cap" =~ ^([0-9]+)\.([0-9]+)$ ]]; then
        arch="sm_${BASH_REMATCH[1]}${BASH_REMATCH[2]}"
    elif [[ "$compute_cap" =~ ^[0-9]+$ ]]; then
        arch="sm_$compute_cap"
    else
        echo "Error: nvidia-smi returned an invalid compute capability: '$compute_cap'." >&2
        echo "Set NVCC_ARCH explicitly (for example, NVCC_ARCH=sm_120)." >&2
        return 1
    fi

    fc_validate_cuda_arch "$arch" || return 1
    printf '%s\n' "$arch"
}

fc_find_cuobjdump() {
    local nvcc_bin="${1:-}"
    if [ -n "$nvcc_bin" ]; then
        local adjacent
        adjacent="$(dirname "$nvcc_bin")/cuobjdump"
        if [ -x "$adjacent" ]; then
            printf '%s\n' "$adjacent"
            return 0
        fi
    fi

    local from_path
    from_path="$(command -v cuobjdump 2>/dev/null || true)"
    if [ -n "$from_path" ]; then
        printf '%s\n' "$from_path"
        return 0
    fi

    echo "Error: cuobjdump is required to verify the compiled CUDA backend." >&2
    return 1
}

fc_verify_cuda_binary_arch() {
    local binary="${1:-}"
    local expected_arch="${2:-}"
    local cuobjdump_bin="${3:-}"

    fc_validate_cuda_arch "$expected_arch" || return 1
    if [ ! -f "$binary" ]; then
        echo "Error: CUDA backend not found at $binary." >&2
        return 1
    fi
    if [ -z "$cuobjdump_bin" ] || [ ! -x "$cuobjdump_bin" ]; then
        echo "Error: cuobjdump executable not found at $cuobjdump_bin." >&2
        return 1
    fi

    local listing
    if ! listing="$($cuobjdump_bin --list-elf "$binary" 2>&1)"; then
        echo "Error: failed to inspect CUDA backend $binary:" >&2
        echo "$listing" >&2
        return 1
    fi
    if [[ "$listing" != *".$expected_arch.cubin"* ]]; then
        echo "Error: CUDA backend does not contain device code for $expected_arch." >&2
        echo "$listing" >&2
        return 1
    fi
}
