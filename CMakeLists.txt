cmake_minimum_required(VERSION 3.21)

# Auto-install vcpkg packages whenever they are missing (e.g. after deleting the build folder).
# Uses the binary cache so this is near-instant when packages were previously built.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/share/glfw3/glfw3Config.cmake")
    find_program(VCPKG_EXE vcpkg HINTS "$ENV{VCPKG_ROOT}" "C:/vcpkg" REQUIRED)
    message(STATUS "vcpkg packages missing — running install...")
    execute_process(
        COMMAND "${VCPKG_EXE}" install --triplet x64-windows
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE VCPKG_RESULT
    )
    if(VCPKG_RESULT)
        message(FATAL_ERROR "vcpkg install failed (exit code ${VCPKG_RESULT})")
    endif()
endif()

project(CrystalByte)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project version
set(PROJECT_VERSION 0.1.0)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Compile GLSL shaders to SPIR-V at build time
set(SHADER_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${SHADER_DIR}")

set(GLSL_SHADERS
    "${CMAKE_SOURCE_DIR}/shaders/triangle.vert"
    "${CMAKE_SOURCE_DIR}/shaders/triangle.frag"
    "${CMAKE_SOURCE_DIR}/shaders/crosshair.vert"
    "${CMAKE_SOURCE_DIR}/shaders/crosshair.frag"
)

foreach(GLSL ${GLSL_SHADERS})
    get_filename_component(NAME ${GLSL} NAME)
    set(SPV "${SHADER_DIR}/${NAME}.spv")
    add_custom_command(
        OUTPUT  "${SPV}"
        COMMAND Vulkan::glslc "${GLSL}" -o "${SPV}"
        DEPENDS "${GLSL}"
        VERBATIM
    )
    list(APPEND SPV_SHADERS "${SPV}")
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPV_SHADERS})

# Create executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Application.cpp
    src/BVH.cpp
    src/Camera.cpp
    src/Mesh.cpp
    src/LaunchSettings.cpp
    src/stb_impl.cpp
)

add_dependencies(${PROJECT_NAME} Shaders)

# Let the C++ code know where the compiled shaders live.
# GLM flags apply project-wide so every .cpp sees them without per-file #defines.
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "SHADER_DIR=\"${SHADER_DIR}\""
    "ASSETS_DIR=\"${CMAKE_SOURCE_DIR}/assets\""
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${VULKAN_INCLUDE_DIRS}
    ${Stb_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    tinyobjloader::tinyobjloader
)

# Enable debug symbols
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(${PROJECT_NAME} PRIVATE -g)
endif()

# Optional: Add compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Print build info
message(STATUS "CrystalByte - Build Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Vulkan: ${VULKAN_FOUND}")
message(STATUS "  GLFW: Found")
message(STATUS "  GLM: Found")
