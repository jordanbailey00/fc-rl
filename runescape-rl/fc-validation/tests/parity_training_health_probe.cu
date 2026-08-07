#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>

/*
 * Validation-only diagnostics for the unmodified compiled Puffer trainer.
 *
 * This translation unit deliberately consumes PufferLib's native structures
 * without adding a test hook to its production binding.  The Python runner
 * imports pufferlib._C first and passes the real _C.PuffeRL instance here;
 * pybind11's process-wide type registry then supplies the native reference.
 * No rollout, loss, optimizer, or environment behavior is implemented here.
 */
#include "pufferlib.cu"

namespace py = pybind11;

struct HealthProbeResult {
    int observations_finite;
    int logits_finite;
    int rewards_finite;
    int logprobs_finite;
    int values_finite;
    int advantages_finite;
    int returns_finite;
    int losses_finite;
    int gradients_finite;
    int masks_valid;
    int every_head_has_legal_action;
    unsigned long long invalid_buffer_events;
    unsigned long long prayer_action_counts[8];
    unsigned long long invalid_prayer_action_counts[8];
};

static HealthProbeResult initial_result() {
    HealthProbeResult result = {};
    result.observations_finite = 1;
    result.logits_finite = 1;
    result.rewards_finite = 1;
    result.logprobs_finite = 1;
    result.values_finite = 1;
    result.advantages_finite = 1;
    result.returns_finite = 1;
    result.losses_finite = 1;
    result.gradients_finite = 1;
    result.masks_valid = 1;
    result.every_head_has_legal_action = 1;
    return result;
}

static void check_cuda(cudaError_t status, const char* operation) {
    if (status != cudaSuccess) {
        throw std::runtime_error(
            std::string(operation) + ": " + cudaGetErrorString(status));
    }
}

__global__ static void mark_precision_finite(
        const precision_t* values, long count, int* finite) {
    long index = (long)blockIdx.x * blockDim.x + threadIdx.x;
    if (index < count && !isfinite(to_float(values[index]))) {
        atomicExch(finite, 0);
    }
}

__global__ static void mark_float_finite(
        const float* values, long count, int* finite) {
    long index = (long)blockIdx.x * blockDim.x + threadIdx.x;
    if (index < count && !isfinite(values[index])) {
        atomicExch(finite, 0);
    }
}

__device__ static bool mask_value_valid(float value) {
    return isfinite(value) && (value == 0.0f || value == 1.0f);
}

__global__ static void validate_action_masks(
        const precision_t* actions,
        const precision_t* masks,
        long rows,
        int first_dim,
        int second_dim,
        int prayer_dim,
        HealthProbeResult* result) {
    long row = (long)blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= rows) return;

    const int dimensions[3] = {first_dim, second_dim, prayer_dim};
    const int offsets[3] = {0, first_dim, first_dim + second_dim};

    for (int head = 0; head < 3; ++head) {
        bool any_legal = false;
        for (int action = 0; action < dimensions[head]; ++action) {
            float mask = to_float(
                masks[row * (first_dim + second_dim + prayer_dim)
                    + offsets[head] + action]);
            if (!mask_value_valid(mask)) {
                atomicExch(&result->masks_valid, 0);
            }
            if (mask == 1.0f) any_legal = true;
        }
        if (!any_legal) {
            atomicExch(&result->every_head_has_legal_action, 0);
        }

        float selected_value = to_float(actions[row * 3 + head]);
        int selected = (int)selected_value;
        bool selected_is_id = isfinite(selected_value)
            && selected_value == (float)selected
            && selected >= 0
            && selected < dimensions[head];
        bool selected_is_legal = false;
        if (selected_is_id) {
            selected_is_legal = to_float(
                masks[row * (first_dim + second_dim + prayer_dim)
                    + offsets[head] + selected]) == 1.0f;
        }
        if (!selected_is_id || !selected_is_legal) {
            atomicAdd(&result->invalid_buffer_events, 1ULL);
        }

        if (head == 2 && selected_is_id && selected < 8) {
            atomicAdd(&result->prayer_action_counts[selected], 1ULL);
            if (!selected_is_legal) {
                atomicAdd(
                    &result->invalid_prayer_action_counts[selected], 1ULL);
            }
        }
    }
}

static void launch_precision_check(
        const PrecisionTensor& tensor, int* device_flag) {
    long count = numel(tensor.shape);
    if (tensor.data == nullptr || count <= 0) {
        int zero = 0;
        check_cuda(cudaMemcpy(
            device_flag, &zero, sizeof(zero), cudaMemcpyHostToDevice),
            "mark missing precision tensor");
        return;
    }
    mark_precision_finite<<<grid_size(count), BLOCK_SIZE>>>(
        tensor.data, count, device_flag);
}

static void launch_float_check(const FloatTensor& tensor, int* device_flag) {
    long count = numel(tensor.shape);
    if (tensor.data == nullptr || count <= 0) {
        int zero = 0;
        check_cuda(cudaMemcpy(
            device_flag, &zero, sizeof(zero), cudaMemcpyHostToDevice),
            "mark missing float tensor");
        return;
    }
    mark_float_finite<<<grid_size(count), BLOCK_SIZE>>>(
        tensor.data, count, device_flag);
}

static py::list count_list(const unsigned long long counts[8]) {
    py::list values;
    for (int i = 0; i < 8; ++i) values.append(counts[i]);
    return values;
}

static py::dict common_result_dict(const HealthProbeResult& result) {
    py::dict output;
    output["masks_valid"] = result.masks_valid == 1;
    output["every_head_has_legal_action"] =
        result.every_head_has_legal_action == 1;
    output["invalid_buffer_events"] = result.invalid_buffer_events;
    output["prayer_action_counts"] =
        count_list(result.prayer_action_counts);
    output["invalid_prayer_action_counts"] =
        count_list(result.invalid_prayer_action_counts);
    return output;
}

static void require_contract(const PuffeRL& pufferl) {
    int action_heads = (int)pufferl.rollouts.actions.shape[2];
    int mask_size = (int)pufferl.rollouts.action_mask.shape[2];
    int host_dims[3] = {};
    check_cuda(cudaMemcpy(
        host_dims, pufferl.act_sizes_puf.data, sizeof(host_dims),
        cudaMemcpyDeviceToHost), "copy action dimensions");
    if (action_heads != 3 || mask_size != 34
            || host_dims[0] != 17 || host_dims[1] != 9
            || host_dims[2] != 8) {
        throw std::runtime_error(
            "TRAIN-001 probe requires the 319/34/{17,9,8} parity contract");
    }
}

static py::dict inspect_rollout(py::object pufferl_object) {
    PuffeRL& pufferl = pufferl_object.cast<PuffeRL&>();
    require_contract(pufferl);

    HealthProbeResult initial = initial_result();
    HealthProbeResult* device = nullptr;
    check_cuda(cudaMalloc((void**)&device, sizeof(initial)),
               "allocate rollout health result");
    try {
        check_cuda(cudaMemcpy(
            device, &initial, sizeof(initial), cudaMemcpyHostToDevice),
            "initialize rollout health result");
        launch_precision_check(
            pufferl.rollouts.observations, &device->observations_finite);
        launch_precision_check(
            pufferl.rollouts.rewards, &device->rewards_finite);
        launch_precision_check(
            pufferl.rollouts.logprobs, &device->logprobs_finite);
        launch_precision_check(
            pufferl.rollouts.values, &device->values_finite);

        for (int buffer = 0; buffer < pufferl.hypers.num_buffers; ++buffer) {
            DecoderActivations* decoder = (DecoderActivations*)
                pufferl.buffer_activations[buffer].decoder;
            launch_precision_check(decoder->out, &device->logits_finite);
        }

        long rows = pufferl.rollouts.actions.shape[0]
            * pufferl.rollouts.actions.shape[1];
        validate_action_masks<<<grid_size(rows), BLOCK_SIZE>>>(
            pufferl.rollouts.actions.data,
            pufferl.rollouts.action_mask.data,
            rows, 17, 9, 8, device);
        check_cuda(cudaGetLastError(), "launch rollout health kernels");
        check_cuda(cudaDeviceSynchronize(), "synchronize rollout health kernels");
        check_cuda(cudaMemcpy(
            &initial, device, sizeof(initial), cudaMemcpyDeviceToHost),
            "copy rollout health result");
    } catch (...) {
        cudaFree(device);
        throw;
    }
    check_cuda(cudaFree(device), "free rollout health result");

    py::dict output = common_result_dict(initial);
    py::dict finite;
    finite["observations"] = initial.observations_finite == 1;
    finite["logits"] = initial.logits_finite == 1
        && initial.logprobs_finite == 1
        && initial.values_finite == 1;
    finite["rewards"] = initial.rewards_finite == 1;
    output["finite"] = finite;
    return output;
}

static py::dict inspect_train(py::object pufferl_object) {
    PuffeRL& pufferl = pufferl_object.cast<PuffeRL&>();
    require_contract(pufferl);

    HealthProbeResult initial = initial_result();
    HealthProbeResult* device = nullptr;
    check_cuda(cudaMalloc((void**)&device, sizeof(initial)),
               "allocate learner health result");
    float losses[NUM_LOSSES] = {};
    try {
        check_cuda(cudaMemcpy(
            device, &initial, sizeof(initial), cudaMemcpyHostToDevice),
            "initialize learner health result");
        launch_precision_check(
            pufferl.advantages_puf, &device->advantages_finite);
        launch_precision_check(
            pufferl.train_buf.mb_returns, &device->returns_finite);

        DecoderActivations* decoder =
            (DecoderActivations*)pufferl.train_activations.decoder;
        launch_precision_check(decoder->out, &device->logits_finite);
        launch_precision_check(pufferl.grad_puf, &device->gradients_finite);
        launch_float_check(
            pufferl.ppo_bufs_puf.grad_logits, &device->gradients_finite);
        launch_float_check(
            pufferl.ppo_bufs_puf.grad_values, &device->gradients_finite);
        launch_float_check(pufferl.losses_puf, &device->losses_finite);

        long rows = pufferl.train_buf.mb_actions.shape[0]
            * pufferl.train_buf.mb_actions.shape[1];
        validate_action_masks<<<grid_size(rows), BLOCK_SIZE>>>(
            pufferl.train_buf.mb_actions.data,
            pufferl.train_buf.mb_action_mask.data,
            rows, 17, 9, 8, device);
        check_cuda(cudaGetLastError(), "launch learner health kernels");
        check_cuda(cudaDeviceSynchronize(), "synchronize learner health kernels");
        check_cuda(cudaMemcpy(
            &initial, device, sizeof(initial), cudaMemcpyDeviceToHost),
            "copy learner health result");
        check_cuda(cudaMemcpy(
            losses, pufferl.losses_puf.data, sizeof(losses),
            cudaMemcpyDeviceToHost), "copy learner losses");
    } catch (...) {
        cudaFree(device);
        throw;
    }
    check_cuda(cudaFree(device), "free learner health result");

    float denominator = losses[LOSS_N];
    bool usable_losses = initial.losses_finite == 1
        && std::isfinite(denominator) && denominator > 0.0f;
    py::dict output = common_result_dict(initial);
    py::dict finite;
    finite["logits"] = initial.logits_finite == 1;
    finite["advantages"] = initial.advantages_finite == 1;
    finite["returns"] = initial.returns_finite == 1;
    finite["gradients"] = initial.gradients_finite == 1;
    finite["policy_loss"] = usable_losses;
    finite["value_loss"] = usable_losses;
    finite["entropy"] = usable_losses;
    finite["kl"] = usable_losses;
    output["finite"] = finite;

    py::dict normalized_losses;
    if (usable_losses) {
        normalized_losses["policy"] = losses[LOSS_PG] / denominator;
        normalized_losses["value"] = losses[LOSS_VF] / denominator;
        normalized_losses["entropy"] = losses[LOSS_ENT] / denominator;
        normalized_losses["kl"] = losses[LOSS_APPROX_KL] / denominator;
    }
    output["losses"] = normalized_losses;
    return output;
}

PYBIND11_MODULE(_fc_train_health_probe, module) {
    module.doc() = "Validation-only Puffer training health diagnostics";
    module.def("inspect_rollout", &inspect_rollout);
    module.def("inspect_train", &inspect_train);
}
