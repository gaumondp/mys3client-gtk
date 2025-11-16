# MyS3 Client

MyS3 Client is a native desktop application for macOS and Windows designed for managing files and folders stored in S3-compatible services like Amazon S3 or MinIO.

## Features

*   **Native Performance:** Built with C and GTK4 for a fast, responsive, and platform-native experience.
*   **Full CRUD Operations:** List, upload, download, rename, and delete files and folders.
*   **Visual Interface:** A collapsible tree view for easy navigation and a detailed file list.
*   **Embedded Editor:** A built-in text editor based on Scintilla for viewing and editing text-based files directly within the app.
*   **Secure:** Credentials are stored securely in the operating system's native keychain. **Note:** This feature is currently only implemented for macOS (using the Keychain).

## Platform Support

**macOS:** Fully supported, including secure credential storage.
**Windows:** Compiles and runs, but uses a mock credential storage system (secrets are not saved).
**Linux:** Not currently supported for secure credential storage. The application will compile and run, but with the same mock credential storage as Windows.

## Build System

This project uses the **Meson** build system. Meson was chosen for its speed, simplicity, and excellent integration with the GNOME/GTK ecosystem.

## Getting Started

### Dependencies

To build and run this application, you will need the following dependencies:

*   A C compiler (e.g., GCC)
*   Meson build system
*   Ninja build tool
*   GTK4
*   GtkSourceView 5
*   GLib 2.0
*   libs3
*   gettext

On a Debian-based system (like Ubuntu), you can install these with the following command:

```bash
sudo apt-get install -y build-essential meson ninja-build libgtk-4-dev libgtksourceview-5-dev libglib2.0-dev libs3-dev gettext
```

### Building

Once you have the dependencies installed, you can build the application with the following commands:

```bash
meson setup builddir
meson compile -C builddir
```

To save a log of the build process, you can redirect the output of the compile command to a file:
```bash
meson compile -C builddir > build_log.txt 2>&1
```

### Installation

To install the application on your system, run the following command from the project root:

```bash
sudo meson install -C builddir
```

## Logging

The application generates runtime logs that can be useful for debugging. The log files are stored in the following locations:

*   **macOS:** `~/Library/Logs/MyS3Client/`
*   **Windows:** `%APPDATA%\\MyS3Client\\Logs\\`
*   **Linux:** `~/.local/state/MyS3Client/logs/`

Logging preferences can be configured in the application's settings dialog.
