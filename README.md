# CrystalByte

A modern C++ application using Vulkan for graphics rendering, GLFW for window management, and GLM for mathematics.

## Prerequisites

### Windows
- CMake 3.21 or higher
- Visual Studio 2019 or newer (with C++ development tools)
- Vulkan SDK (https://vulkan.lunarg.com/)
- GLFW 3.3+ (via vcpkg or manual installation)
- GLM (header-only library)

### macOS
- CMake 3.21 or higher
- Xcode Command Line Tools
- MoltenVK for Vulkan support (part of Vulkan SDK)
- GLFW 3.3+
- GLM

### Linux
- CMake 3.21 or higher
- GCC or Clang
- Vulkan development packages (`libvulkan-dev` on Ubuntu/Debian)
- GLFW development packages (`libglfw3-dev` on Ubuntu/Debian)
- GLM development packages (`libglm-dev` on Ubuntu/Debian)

## Installation

### Install Dependencies

#### Windows (using vcpkg)
```bash
vcpkg install vulkan:x64-windows glfw3:x64-windows glm:x64-windows
```

#### Ubuntu/Debian
```bash
sudo apt-get install cmake vulkan-tools libvulkan-dev glfw3-dev libglm-dev
```

#### macOS (using Homebrew)
```bash
brew install cmake vulkan-headers glfw glm molten-vk
```

## Building

### Generate Build Files
```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Compile
```bash
cmake --build . --config Release
```

Or using platform-specific build tools:

#### Windows
```bash
cmake --build . --config Release -- /m
```

#### Unix-like (macOS, Linux)
```bash
make -j$(nproc)
```

## Running

After building, the executable will be located at `build/bin/CrystalByte`:

```bash
./build/bin/CrystalByte
```

## Project Structure

```
CrystalByte/
├── src/
│   └── main.cpp          # Main application entry point
├── include/              # Header files for future expansion
├── assets/               # Game assets (textures, models, etc.)
├── build/                # Build output directory (generated)
├── CMakeLists.txt        # CMake configuration
├── README.md            # This file
└── .github/
    └── copilot-instructions.md  # Project setup documentation
```

## Features

- ✅ Vulkan instance creation
- ✅ Physical device enumeration and selection
- ✅ Logical device creation
- ✅ GLFW window management
- ✅ GLM matrix mathematics
- 🎯 Ready for graphics pipeline implementation

## Architecture

The project uses a clean C++20 class-based architecture with:
- `VulkanApplication`: Main application class managing Vulkan lifecycle
- Separation of concerns with initialization, main loop, and cleanup phases
- Error handling with exceptions

## Next Steps

1. Implement graphics pipeline (shaders, render passes)
2. Add vertex/fragment buffers
3. Implement render loop with command buffers
4. Add input handling
5. Create scene management

## Troubleshooting

### "Vulkan SDK not found"
Ensure the Vulkan SDK is installed and the `VULKAN_SDK` environment variable is set:
```bash
# Windows
set VULKAN_SDK=C:\VulkanSDK\1.x.x.x

# Unix-like
export VULKAN_SDK=/path/to/VulkanSDK
```

### Build fails with missing headers
Ensure all dependencies are properly installed and CMake can find them. On Windows with vcpkg, specify the toolchain:
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake ..
```

## License

This is a starter template. Modify as needed for your project.

## Resources

- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [GLFW Documentation](https://www.glfw.org/documentation.html)
- [GLM Documentation](https://glm.g-truc.net/)
- [Vulkan SDK](https://vulkan.lunarg.com/)
