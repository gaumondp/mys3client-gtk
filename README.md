<div align="center">
  <img src="res/logo-mys3client96.png" alt="MyS3 Client Logo" />
</div>

# MyS3 Client

MyS3 Client is a native desktop application for macOS and Windows designed for managing files and folders stored in S3-compatible services like Amazon S3 or MinIO.

## Features

*   **Native Performance:** Built with C and GTK4 for a fast, responsive, and platform-native experience.
*   **Full CRUD Operations:** List, upload, download, rename, and delete files and folders.
*   **Visual Interface:** A collapsible tree view for easy navigation and a detailed file list.
*   **Embedded Editor:** A built-in text editor based on GtkSourceView 5 for viewing and editing text-based files directly within the app.
*   **Secure:** Credentials are stored securely in the operating system's native keychain (macOS Keychain, Windows Credential Manager).

## Platform Support

**macOS:** Fully supported, including secure credential storage.

**Windows:** Fully supported, including secure credential storage using the Windows Credential Manager.
**Linux:** While the application may compile and run on Linux, it is not officially supported.

## Dependencies

To build and run this application, you will need the following dependencies:

*   A C compiler (e.g., GCC)
*   Meson build system
*   Ninja build tool
*   CMake
*   GTK4
*   GtkSourceView 5
*   GLib 2.0
*   AWS SDK for C++ (specifically `s3` and `core` components)
*   gettext


## Installation

For detailed instructions on how to compile and install MyS3 Client on your platform, please refer to the [INSTALL.md](INSTALL.md) file.
