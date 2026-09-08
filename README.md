# LoadingScreen

**LoadingScreen** is a modern, high-performance loading screen ASI plugin for **SA-MP 0.3.7-R1** built with Dear ImGui, DirectX 9, and `stb_image`.

## Features

- **Modern UI Design**: Sleek and modern loading overlay featuring dynamic background effects, custom branding, and smooth progress tracking.
- **Server Lock (IP Whitelist)**: Built-in protection that activates the plugin only when connecting to authorized server IPs.
- **DirectX 9 & ImGui Integration**: Efficient in-game rendering hook into SA-MP's rendering pipeline.
- **Lightweight Asset Loading**: Uses `stb_image` for fast, lightweight texture loading without external DirectX SDK dependencies for images.

## Requirements

- **SA-MP 0.3.7-R1** (Client)
- **Visual Studio 2019+** (C++20 support)
- **CMake 3.15+** and **Git**

## Building

### 1. Clone the repository

```bash
git clone https://github.com/GJobb5/LoadingScreen.git
cd LoadingScreen
```

### 2. Configure with CMake (32-bit Win32)

```bash
cmake -B build -A Win32
```

### 3. Build the plugin

```bash
cmake --build build --config Release
```

The compiled ASI plugin (`LoadingScreen.asi`) will be generated in `build/bin/` or `build/Release/`.