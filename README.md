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
**Linux:** While the application may compile and run on Linux, it is not officially supported.

## Dependencies

To build and run this application, you will need the following dependencies:

*   A C compiler (e.g., GCC)
*   Meson build system
*   Ninja build tool
*   GTK4
*   GtkSourceView 5
*   GLib 2.0
*   libs3
*   gettext

## Installation

For detailed instructions on how to compile and install MyS3 Client on your platform, please refer to the [INSTALL.md](INSTALL.md) file.
