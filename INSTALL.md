# Installation Instructions

This document provides a guide for compiling and installing MyS3 Client from source on macOS and Windows.

## Table of Contents

- [Prerequisites](#prerequisites)
- [macOS](#macos)
- [Windows](#windows)

## Prerequisites

Compiling from source requires a C/C++ compiler and a few standard build tools. A basic familiarity with using a command-line interface (CLI) is assumed.

**The build system will automatically download and configure all necessary libraries.**

---

## macOS

These instructions use the [Homebrew](https://brew.sh/) package manager.

### 1. Install Homebrew

If you don't have Homebrew, open the Terminal and run:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. Install Build Dependencies

This command installs the compiler, build tools, and essential libraries.

```bash
brew install meson ninja cmake gettext gtk4 gtksourceview5
```

### 3. Compile the Application

Navigate to the project's root directory in your terminal and run the following commands. The `meson setup` command will automatically download and prepare the AWS SDK for C++, which may take a few minutes.

```bash
meson setup builddir --wipe
meson compile -C builddir
```

The compiled application will be located in the `builddir` directory.

---

## Windows

These instructions use [MSYS2](https://www.msys2.org/) to create a suitable development environment.

### 1. Install MSYS2

Download and run the installer from the [MSYS2 website](https://www.msys2.org/).

### 2. Update MSYS2

After installation, open the **MSYS2 MinGW 64-bit** shell from the Start Menu and update the core packages:

```bash
pacman -Syu
```

The shell may close. If it does, reopen it and run the command again to finish the update.

### 3. Install Build Dependencies

In the MSYS2 shell, install the required toolchain and build tools:

```bash
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-cmake mingw-w64-x86_64-gtk4 mingw-w64-x86_64-gtksourceview5 mingw-w64-x86_64-gettext
```

### 4. Compile the Application

Navigate to the project's root directory in the MSYS2 shell. For example:

```bash
cd /c/Users/YourUser/mys3-client
```

Now, compile the application. The `meson setup` command will automatically download and prepare the AWS SDK for C++, which may take a few minutes.

```bash
meson setup builddir --wipe
meson compile -C builddir
```

The compiled application (`.exe`) will be located in the `builddir` directory.

---

## Troubleshooting

### `Subproject exists but has no CMakeLists.txt file` Error

This error can happen if a previous attempt to run `meson setup` failed after creating the `subprojects/aws-sdk-cpp` directory but before downloading the source code.

To fix it, completely remove the build directory and the empty subproject directory, then run the setup again:

```bash
rm -rf builddir subprojects/aws-sdk-cpp
meson setup builddir --wipe
```
