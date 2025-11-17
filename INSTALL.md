# Installation Instructions

This document provides a complete guide for compiling and installing MyS3 Client from source on macOS and Windows.

## Table of Contents

- [Step 1: Get the Source Code (Crucial)](#step-1-get-the-source-code-crucial)
- [Step 2: Install Dependencies & Compile](#step-2-install-dependencies--compile)
  - [macOS](#macos)
    - [1. Install Homebrew](#1-install-homebrew)
    - [2. Install Dependencies](#2-install-dependencies)
    - [3. Configure Environment](#3-configure-environment)
    - [4. Compile the Application](#4-compile-the-application)
  - [Windows](#windows)
    - [1. Install MSYS2](#1-install-msys2)
    - [2. Update MSYS2](#2-update-msys2)
    - [3. Install Dependencies](#3-install-dependencies-windows)
    - [4. Compile the Application](#4-compile-the-application-windows)

## Prerequisites

Compiling from source requires some command-line familiarity. You will need:

*   **A C/C++ Compiler** (like GCC or Clang)
*   **Build Tools** (Git, Meson, Ninja, CMake)
*   **A Command-Line Interface (CLI):** All commands are run in a terminal.

---

## Step 1: Get the Source Code (Crucial)

This project uses a Git submodule to manage the AWS SDK for C++. It is **essential** that you download this submodule's source code.

#### Option A: Clone with Submodules (Recommended)

If you are cloning the repository for the first time, use the `--recursive` flag:

```bash
git clone --recursive https://github.com/your-repo/mys3-client.git
cd mys3-client
```

#### Option B: Initialize Submodules in an Existing Clone

If you have already cloned the repository without the submodule, navigate to the project's root directory and run:

```bash
git submodule update --init --recursive
```

This will download the necessary AWS SDK source code into the `subprojects` directory.

---

## Step 2: Install Dependencies & Compile

Follow the instructions for your operating system.

### macOS

These instructions use the [Homebrew](https://brew.sh/) package manager.

#### 1. Install Homebrew

If you don't have Homebrew, open the Terminal and run:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 2. Install Dependencies

This command installs the compiler, build tools, and all required libraries.

```bash
brew install meson ninja cmake gettext glib pkg-config gtk4 gtksourceview5
```

**Important:** Do NOT install `aws-sdk-cpp` via Homebrew. The SDK will be compiled from the source code in the submodule.

#### 3. Configure Environment

To ensure the build system can find the libraries installed by Homebrew, you must configure your environment. Run the following commands:

```bash
export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig"
export PKG_CONFIG="$(brew --prefix)/bin/pkg-config"
```

**Note:** For this change to be permanent, add these lines to your shell's startup file (e.g., `~/.zshrc` or `~/.bash_profile`).

#### 4. Compile the Application

With your environment configured, you can now build the application:

```bash
meson setup builddir --wipe
meson compile -C builddir
```

The compiled application will be in the `builddir` directory.

---

### Windows

These instructions use [MSYS2](https://www.msys2.org/) to create a suitable development environment.

#### 1. Install MSYS2

Download and run the installer from the [MSYS2 website](https://www.msys2.org/).

#### 2. Update MSYS2

After installation, open the **MSYS2 MinGW 64-bit** shell from the Start Menu and update the core packages:

```bash
pacman -Syu
```

The shell may close. If it does, reopen it and run the command again to finish the update.

#### 3. Install Dependencies (Windows)

In the MSYS2 shell, install the required toolchain, build tools (including Git), and libraries:

```bash
pacman -S git mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-cmake mingw-w64-x86_64-gtk4 mingw-w64-x86_64-gtksourceview5 mingw-w64-x86_64-gettext
```

Press `Enter` to accept the default selections.

#### 4. Compile the Application (Windows)

Navigate to the project's root directory in the MSYS2 shell. For example:

```bash
cd /c/Users/YourUser/mys3-client
```

Now, compile the application. The `meson setup` command will configure the build and use the AWS SDK source from the submodule.

```bash
meson setup builddir --wipe
meson compile -C builddir
```

The compiled application (`.exe`) will be in the `builddir` directory.
