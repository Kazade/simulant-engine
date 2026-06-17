# Installation Guide

This guide will walk you through installing Simulant on your platform of choice.

## Prerequisites

Before installing Simulant, ensure you have:

- A terminal and basic command-line familiarity
- Git installed for version control
- Sufficient disk space (~1-2 GB for development)
- A working internet connection

---

## Fedora

Fedora is the **recommended platform** for Simulant development due to its up-to-date packages and excellent Docker support.

### 1. Install Dependencies

Open a terminal and install the required packages:

```bash
sudo dnf install \
     cmake make \
     gcc-c++ SDL2-devel \
     openal-soft-devel \
     zlib-devel mesa-libGL-devel \
     docker
```

### 2. Install Simulant CLI

The Simulant CLI tool helps you create, build, and manage projects:

```bash
pip3 install -U --user git+https://gitlab.com/simulant/simulant-tools.git
```

### 3. Verify Installation

Test that the `simulant` command is available:

```bash
simulant --help
```

You should see a list of available commands.

> **Next Steps**: Follow the [IDE Setup](ide-setup.md) guide, or jump straight to [Your First Game](first-game.md).

---

## Ubuntu / Linux Mint

Ubuntu and Linux Mint are also well-supported.

### 1. Install Dependencies

```bash
sudo apt update
sudo apt-get install \
    python3-pip git \
    python3-setuptools \
    cmake build-essential \
    libsdl2-dev \
    libopenal-dev \
    docker.io
```

### 2. Install Simulant

Follow the same steps as the Fedora installation above.

You may need to fix the PATH variable to find the command: 

```
echo "PATH=$HOME/.local/bin:$PATH" >> ~/.profile
source ~/.profile
```

---

## Windows (WSL2)

Simulant supports Windows via the **Windows Subsystem for Linux 2 (WSL2)**. This gives you a full Linux environment while running Windows.

### 1. Install WSL2

1. Open PowerShell as Administrator
2. Run:
   ```powershell
   wsl --install
   ```
3. Restart your computer when prompted
4. Set up your Ubuntu distribution (username and password)

### 2. Install Dependencies

Inside your WSL2 terminal:

```bash
sudo apt update
sudo apt-get install \
    python3-pip git \
    python3-setuptools \
    cmake build-essential \
    libsdl2-dev \
    libopenal-dev \
    docker.io

# Enable WSLg for GUI support (Windows 11)
# On Windows 10, you may need to install an X server like VcXsrv
```

### 3. Install Simulant

Follow the same build steps as above.

> **Note**: For native Windows builds (not WSL2), you'll need to cross-compile using Docker. See the [Dreamcast Development](../guides/dreamcast.md) guide.

---

## Manual Installation (DreamSDK)

If you prefer not to use the CLI tool, you can install Simulant manually:

1. Clone the repository
2. Build using CMake
3. Link against the Simulant library in your own project

See [Manual DreamSDK Installation](install_manual_dreamsdk.md) for details.

---

## Docker

Docker is **required** when using the simulant cli to allow:

- Cross-compilation for Dreamcast and Windows
- Reproducible builds
- CI/CD pipelines

### Installing Docker

**Fedora:**
```bash
sudo dnf install docker
sudo systemctl enable docker
sudo systemctl start docker
# Add your user to the docker group to avoid sudo
sudo usermod -aG docker $USER
```

**Ubuntu:**
```bash
sudo apt install docker.io
sudo systemctl enable docker
sudo systemctl start docker
sudo usermod -aG docker $USER
```

---

## Troubleshooting

### CMake can't find OpenGL

Ensure you have Mesa development packages installed:

```bash
# Fedora
sudo dnf install mesa-libGL-devel

# Ubuntu
sudo apt install libgl1-mesa-dev
```

### OpenAL not found

```bash
# Fedora
sudo dnf install openal-soft-devel

# Ubuntu
sudo apt install libopenal-dev
```

### C++17 errors during compilation

Ensure your compiler supports C++17. You need GCC 7+ or Clang 5+:

```bash
g++ --version
```

If your compiler is too old, consider upgrading your OS or using a newer toolchain.

---

## Next Steps

- **[IDE Setup](ide-setup.md)** - Configure your IDE for Simulant development
- **[Your First Game](first-game.md)** - Start building!
- **[Project Structure](project-structure.md)** - Learn how Simulant projects are organized
