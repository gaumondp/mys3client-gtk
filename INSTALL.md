# Installation Instructions

This document provides detailed instructions for compiling and installing MyS3 Client on macOS and Windows.

## Table of Contents

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

## Dependencies

Before you begin, ensure you have the following dependencies installed:

*   A C compiler (e.g., GCC or Clang)
*   Meson build system
*   Ninja build tool
*   GTK4
*   GtkSourceView 5
*   GLib 2.0
*   libs3
*   gettext

The following sections provide platform-specific instructions for installing these dependencies.

---

## macOS

These instructions assume you are using the [Homebrew](https://brew.sh/) package manager.

### 1. Install Homebrew

If you do not have Homebrew installed, open the Terminal application and paste the following command:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Follow the on-screen instructions to complete the installation.

### 2. Install Dependencies

Once Homebrew is installed, you can install all the required dependencies with a single command. Open the Terminal and run:

```bash
brew install meson ninja gtk4 gtksourceview5 libs3 gettext
```

### 3. Compile the Application

After the dependencies are installed, navigate to the project's root directory in your terminal and run the following commands to compile the application:

```bash
meson setup builddir
meson compile -C builddir
```

The compiled application will be located in the `builddir` directory.

---

## Windows

Compiling on Windows requires [MSYS2](https://www.msys2.org/), a software distribution that provides a Unix-like environment.

### 1. Install MSYS2

Download and run the installer from the [MSYS2 website](https://www.msys2.org/). Follow the instructions in the installation wizard.

### 2. Update MSYS2

After installation, open the **MSYS2 MinGW 64-bit** shell from the Start Menu. First, update the package database and core packages by running:

```bash
pacman -Syu
```

The shell may close during this process. If it does, reopen it and run the command again to complete the update.

### 3. Install Dependencies (Windows)

In the **MSYS2 MinGW 64-bit** shell, install the required toolchain and all dependencies with the following command:

```bash
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-gtk4 mingw-w64-x86_64-gtksourceview5 mingw-w64-x86_64-libs3 mingw-w64-x86_64-gettext
```

Press `Enter` to accept the default selections when prompted.

### 4. Compile the Application (Windows)

Ensure you are still in the **MSYS2 MinGW 64-bit** shell. Navigate to the project's root directory. For example, if your project is in `C:\Users\YourUser\mys3-client`, you would run:

```bash
cd /c/Users/YourUser/mys3-client
```

Now, compile the application using the following commands:

```bash
meson setup builddir
meson compile -C builddir
```

The compiled application (`.exe`) will be located in the `builddir` directory.
