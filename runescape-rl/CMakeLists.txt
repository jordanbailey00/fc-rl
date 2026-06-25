cmake_minimum_required(VERSION 3.20)
project(runescape_rl C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

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

# Fight Caves training backend
add_subdirectory(fc-training)
