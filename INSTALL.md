# Installation Instructions

This document provides a guide for compiling and installing MyS3 Client from source on macOS and Windows.

## Build Process Overview

Building this project is a two-step process:

1.  **Build the AWS SDK for C++:** A helper script is provided to download and compile the specific version of the AWS SDK for C++ required by this application.
2.  **Build the GTK Application:** Once the SDK is built, you can compile the main application using Meson.

These steps are detailed below for each supported platform.

---

## Step 1: Build the AWS SDK for C++

This step only needs to be performed once. The script will download the AWS SDK source code and install the necessary compiled libraries into a local `dist` directory inside the `scripts` folder.

### macOS and Linux

Open a terminal, navigate to the project's root directory, and run:

```bash
./scripts/build-aws-sdk.sh
```

### Windows (MSYS2)

Open the **MSYS2 MinGW 64-bit** shell, navigate to the project's root directory, and run:

```bash
./scripts/build-aws-sdk.sh
```

If the script fails, ensure you have `git` and `ninja` installed in your MSYS2 environment by running: `pacman -S git mingw-w64-x86_64-ninja`.

---

## Step 2: Build the GTK Application

After successfully building the AWS SDK, you can proceed to build the main application.

### Prerequisites

Compiling from source requires a C/C++ compiler and a few standard build tools. A basic familiarity with using a command-line interface (CLI) is assumed.

### macOS

These instructions use the [Homebrew](https://brew.sh/) package manager.

#### 1. Install Homebrew

If you don't have Homebrew, open the Terminal and run:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 2. Install Build Dependencies

This command installs the compiler, build tools, and essential libraries.

```bash
brew install meson ninja cmake gettext gtk4 gtksourceview5
```

#### 3. Set Environment Variable (IMPORTANT)

Before compiling, you need to tell the build system where to find the libraries installed by Homebrew. Run this command in your terminal:

```bash
export PKG_CONFIG_PATH=$(brew --prefix)/lib/pkgconfig
```

**Note:** You must run this command in every new terminal session before compiling.

#### 4. Compile the Application

Navigate to the project's root directory in your terminal and run the following commands:

```bash
meson setup builddir --wipe
meson compile -C builddir
```

The compiled application will be located in the `builddir` directory.

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

#### 3. Install Build Dependencies

In the MSYS2 shell, install the required toolchain and build tools:

```bash
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-cmake mingw-w64-x86_64-gtk4 mingw-w64-x86_64-gtksourceview5 mingw-w64-x86_64-gettext
```

#### 4. Compile the Application

Navigate to the project's root directory in the MSYS2 shell. For example:

```bash
cd /c/Users/YourUser/mys3-client
```

Now, compile the application:

```bash
meson setup builddir --wipe
meson compile -C builddir
```

The compiled application (`.exe`) will be located in the `builddir` directory.

---

## Troubleshooting

If you encounter issues during the `meson setup` phase related to finding the AWS SDK, ensure that you have successfully run the `./scripts/build-aws-sdk.sh` script and that the `scripts/dist` directory exists and contains the SDK libraries.
