# MyS3 Client

MyS3 Client is a native desktop application for macOS and Windows designed for managing files and folders stored in S3-compatible services like Amazon S3 or MinIO.

## Features

*   **Native Performance:** Built with C and GTK4 for a fast, responsive, and platform-native experience.
*   **Full CRUD Operations:** List, upload, download, rename, and delete files and folders.
*   **Visual Interface:** A collapsible tree view for easy navigation and a detailed file list.
*   **Embedded Editor:** A built-in text editor based on Scintilla for viewing and editing text-based files directly within the app.
*   **Secure:** Credentials are stored securely in the operating system's native keychain (macOS Keychain, Windows Credential Manager).

## Build System

This project uses the **Meson** build system. Meson was chosen for its speed, simplicity, and excellent integration with the GNOME/GTK ecosystem.

## Getting Started

To build the project, you will need to have Meson, a C compiler, and the required development libraries (GTK4, AWS SDK for C++, etc.) installed.

```bash
# Configure the build
meson setup builddir

# Compile the application
meson compile -C builddir
```
