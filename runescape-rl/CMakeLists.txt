cmake_minimum_required(VERSION 3.20)
project(runescape_rl C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

option(FC_ENABLE_ASAN "Enable AddressSanitizer for all C targets" OFF)
option(FC_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for all C targets" OFF)

if(FC_ENABLE_ASAN OR FC_ENABLE_UBSAN)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR
            "FC sanitizer options require a GNU- or Clang-compatible compiler")
    endif()

    set(FC_SANITIZER_FLAGS "")
    if(FC_ENABLE_ASAN)
        list(APPEND FC_SANITIZER_FLAGS -fsanitize=address)
    endif()
    if(FC_ENABLE_UBSAN)
        list(APPEND FC_SANITIZER_FLAGS -fsanitize=undefined)
    endif()
    list(APPEND FC_SANITIZER_FLAGS
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )
    add_compile_options(${FC_SANITIZER_FLAGS})
    add_link_options(${FC_SANITIZER_FLAGS})
    message(STATUS
        "Fight Caves sanitizers enabled: ASan=${FC_ENABLE_ASAN}, UBSan=${FC_ENABLE_UBSAN}, compiler=${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
endif()

# Build type defaults to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Shared Fight Caves backend
add_subdirectory(fc-core)

# Fight Caves viewer and tests
add_subdirectory(fc-viewer)

# Fight Caves validation guardrails
add_subdirectory(fc-validation)
