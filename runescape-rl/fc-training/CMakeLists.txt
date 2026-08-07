# fc-training: Headless Fight Caves backend for RL training.
# Builds libfc_capi.so — loaded by Python via ctypes for PufferLib integration.
# Links against the shared fc-core backend and compiles the shared C API shim.

add_library(fc_capi SHARED ../fc-core/src/fc_capi.c)
target_include_directories(fc_capi PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(fc_capi PRIVATE fc_core m)

set_target_properties(fc_capi PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "fc_capi"
)

add_executable(parity_fix_capi_contract
    ../fc-validation/tests/parity_fix_capi.c
)
target_include_directories(parity_fix_capi_contract PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(parity_fix_capi_contract PRIVATE fc_capi dl m)

add_test(NAME parity_contract_002_capi_dimensions
    COMMAND parity_fix_capi_contract dimension_getters)
set_tests_properties(parity_contract_002_capi_dimensions PROPERTIES
    LABELS "parity_fix;contract;capi")

add_test(NAME parity_contract_003_capi_batch_canaries
    COMMAND parity_fix_capi_contract batch_strides)
set_tests_properties(parity_contract_003_capi_batch_canaries PROPERTIES
    LABELS "parity_fix;contract;capi;batch")
