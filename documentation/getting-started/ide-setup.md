# IDE Setup

This guide covers setting up your development environment for Simulant game development.

## VS Code (Recommended)

Visual Studio Code is a free, cross-platform editor with excellent C++ support.

### 1. Install VS Code

Download from [code.visualstudio.com](https://code.visualstudio.com/)

### 2. Install Extensions

Install these extensions:

- **C/C++** (Microsoft) - IntelliSense, debugging
- **CMake Tools** (Microsoft) - CMake integration
- **C++ Intellisense** - Enhanced code completion

### 3. Configure C++ Settings

Create `.vscode/c_cpp_properties.json` in your project:

```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "/usr/include",
                "/path/to/simulant/include"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/g++",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```

### 4. Build Tasks

Create `.vscode/tasks.json`:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build",
            "type": "shell",
            "command": "simulant build",
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Run",
            "type": "shell",
            "command": "simulant run --rebuild",
            "group": "build"
        },
        {
            "label": "Test",
            "type": "shell",
            "command": "simulant test",
            "group": "test"
        }
    ]
}
```

### 5. Debugging

Create `.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Game",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/linux/your-game-name",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "Build"
        }
    ]
}
```

## CLion

JetBrains CLion is a premium C++ IDE with excellent CMake support.

### 1. Open Project

File → Open → Select your project's `CMakeLists.txt`

### 2. Configure CMake

CLion should auto-detect CMake. If not:

- Settings → Build, Execution, Deployment → CMake
- Set CMake options if needed

### 3. Run Configurations

Add run configurations:

- Run → Edit Configurations
- Add Application:
  - **Executable**: Path to built executable
  - **Working directory**: Your project root

## Qt Creator

Qt Creator is a lightweight alternative with good CMake support.

### 1. Open Project

File → Open File or Project → Select `CMakeLists.txt`

### 2. Configure Kit

Ensure you have a desktop kit configured (GCC + CMake + Debugger).

## Vim/Neovim

For terminal-based development:

### Using coc.nvim

Install `coc-cmake` and `coc-clangd` for language server support:

```vim
:CocInstall coc-cmake coc-clangd
```

Configure `.clangd` in your project root:

```yaml
CompileFlags:
  Add: [-std=c++17]
```

## Common Issues

### IntelliSense Can't Find Simulant Headers

Add Simulant's include paths to your IDE configuration:

```json
"includePath": [
    "/path/to/simulant/simulant/**"
]
```

### CMake Configuration Errors

Ensure you have CMake 3.10+:

```bash
cmake --version
```

### Build Fails with C++17 Errors

Ensure your compiler supports C++17 features:

```bash
g++ --version  # Should be GCC 7 or later
```

## Next Steps

- **[Your First Game](first-game.md)** - Start building!
- **[C++ Guidelines](../reference/cpp-guidelines.md)** - Code style recommendations
- **[Debugging Tips](../reference/debugging.md)** - Effective debugging strategies
