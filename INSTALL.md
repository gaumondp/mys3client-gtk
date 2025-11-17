# Installation Instructions

This document provides detailed instructions for compiling and installing MyS3 Client on macOS and Windows.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Dependencies](#dependencies)
- [macOS](#macos)
  - [1. Install Homebrew](#1-install-homebrew)
  - [2. Install Dependencies](#2-install-dependencies)
  - [3. Compile the Application](#3-compile-the-application)
- [Windows](#windows)
  - [1. Install MSYS2](#1-install-msys2)
  - [2. Update MSYS2](#2-update-msys2)
  - [3. Install Dependencies](#3-install-dependencies-windows)
  - [4. Compile the Application](#4-compile-the-application-windows)

## Prerequisites

Compiling a C application from source can be a new experience if you've never done it before. This guide will walk you through the process.

At a high level, compiling is the process of turning human-readable source code (like the `.c` files in this project) into an executable program that your computer can run. To do this, you need a few tools:

*   **A C Compiler:** This is the core tool that does the translation. Common examples are GCC and Clang.
*   **Build System (Meson & Ninja):** For a project with multiple files, a build system automates the compilation process. This project uses Meson to configure the build and Ninja to run the compilation steps efficiently.
*   **A Command-Line Interface (CLI):** All the commands in this guide are intended to be run in a terminal (like Terminal.app on macOS or the MSYS2 shell on Windows). A basic understanding of how to navigate directories in a CLI is assumed.

The instructions below will guide you through installing all the necessary tools for your operating system.

## Dependencies

Before you begin, ensure you have the following dependencies installed:

*   A C/C++ compiler (e.g., GCC or Clang)
*   Meson build system
*   Ninja build tool
*   CMake
*   GTK4
*   GtkSourceView 5
*   GLib 2.0
*   AWS SDK for C++ (handled automatically by the build system)
*   gettext

The following sections provide platform-specific instructions for installing these dependencies.

---

## macOS

These instructions assume you are using the [Homebrew](https://brew.sh/) package manager, which is the standard way to install development tools on macOS.

### 1. Install Homebrew

If you do not have Homebrew installed, open the Terminal application and paste the following command:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Follow the on-screen instructions to complete the installation.

### 2. Install Dependencies

Once Homebrew is installed, you can install all the required dependencies with a single command. This command will install the compilers, build tools, and libraries needed by the project.

Open the Terminal and run:

```bash
brew install meson ninja cmake gtk4 gtksourceview5 gettext glib
```

### 3. Configure Environment

Homebrew installs libraries in a location that the build system doesn't check by default. To fix this, you need to set an environment variable to tell the build tools where to find the necessary configuration files.

Run the following command in your terminal. This will configure the path for your current terminal session.

```bash
export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig"
```

**Note:** For this change to be permanent, you should add this line to your shell's startup file (e.g., `~/.zshrc` or `~/.bash_profile`).

### 4. Compile the Application

After the dependencies are installed and your environment is configured, navigate to the project's root directory in your terminal. The `meson setup` command inspects your system and prepares the project for compilation. The `meson compile` command then runs the compiler to build the application.

```bash
meson setup builddir --wipe
meson compile -C builddir
```

The compiled application will be located in the `builddir` directory.

---

## Windows

Compiling on Windows requires a specific development environment to provide the necessary tools and libraries. This project uses [MSYS2](https://www.msys2.org/), a software distribution that provides a Unix-like environment and a package manager, `pacman`.

### 1. Install MSYS2

Download and run the installer from the [MSYS2 website](https://www.msys2.org/). Follow the instructions in the installation wizard.

### 2. Update MSYS2

After installation, open the **MSYS2 MinGW 64-bit** shell from the Start Menu. It is crucial to update the package database and core packages to ensure you have the latest versions. Run the following command:

```bash
pacman -Syu
```

The shell may close during this process. This is normal. If it does, reopen it and run the command again to complete the update.

### 3. Install Dependencies (Windows)

In the **MSYS2 MinGW 64-bit** shell, install the required toolchain (which includes the compiler), build tools, and all other necessary libraries with the following command:

```bash
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-cmake mingw-w64-x86_64-gtk4 mingw-w64-x86_64-gtksourceview5 mingw-w64-x86_64-gettext
```

Press `Enter` to accept the default selections when prompted.

### 4. Compile the Application (Windows)

Ensure you are still in the **MSYS2 MinGW 64-bit** shell. Navigate to the project's root directory. For example, if your project is in `C:\Users\YourUser\mys3-client`, you would run:

```bash
cd /c/Users/YourUser/mys3-client
```

Now, compile the application. The `meson setup` command inspects your system and downloads the AWS SDK for C++ automatically. The `meson compile` command then builds the entire project.

```bash
meson setup builddir --wipe
meson compile -C builddir
```

The compiled application (`.exe`) will be located in the `builddir` directory.
