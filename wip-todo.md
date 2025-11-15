# WIP - Remaining Tasks for MyS3 Client

This document outlines the remaining work required to complete the MyS3 Client application based on the original project specification.

## Phase 4: Core File Operations (CRUD)

## Phase 5: Embedded Scintilla Text Editor
- [ ] **Build Fix:** Resolve the `SciLexer.h` not found build error by correctly cloning the Scintilla repository and configuring the include path in `meson.build`.

**AI Developer Notes:**

I encountered a significant roadblock while trying to build the project to verify the Phase 4 changes. The Scintilla library is failing to build with GTK4. Here is a summary of the approaches I've tried:

*   **Manual Build with Makefile:**
    *   Extracted the `scintilla558.tgz` archive.
    *   Modified `scintilla/gtk/makefile` to use `GTK_VERSION = gtk-4`.
    *   Manually added GTK4 cflags and libs to the makefile.
    *   The build failed with a "gdk/gdkwayland.h: No such file or directory" error, even after installing `libwayland-dev` and adding the include path to the makefile.

*   **Meson Subproject:**
    *   Created a `meson.build` file in the `scintilla` directory.
    *   Modified the root `meson.build` to use Scintilla as a subproject.
    *   The build failed because the subproject could not find the GTK4 dependency.
    *   Attempted to pass the GTK4 dependency to the subproject, but this also failed.

*   **Meson Wrap File:**
    *   Created a `scintilla.wrap` file to have Meson download and build Scintilla automatically.
    *   The build timed out, likely due to the long download and build time.

*   **Simplified Meson Subproject:**
    *   Created a simplified `meson.build` file in the `scintilla` directory, only including the necessary source files.
    *   This also failed due to the GTK4 dependency issue.

The next agent should focus on resolving this Scintilla build issue before proceeding with the rest of the tasks. It may be necessary to find a different version of Scintilla or to use a different build approach.
- [ ] **Save Functionality:** Implement saving changes from the editor back to S3 (S3 PUT Object).
- [ ] **Find/Replace:** Add a basic find/replace dialog and functionality to the editor.
- [ ] **Unsaved Changes:** Implement a confirmation dialog to warn the user about unsaved changes before closing an editor tab.

## Phase 6: Internationalization (i18n)
- [ ] **Full Translation:** Complete the translation of all UI strings in the `.po` files for all specified languages (en, fr, de, es).
- [ ] **Dynamic Language Switching:** Implement the logic to dynamically apply the selected language, ideally without requiring a full application restart.

## Advanced Features & UX
- [ ] **Multipart Upload:**
  - [ ] Implement the full multipart upload flow (`CreateMultipartUpload`, `UploadPart`, `CompleteMultipartUpload`, `AbortMultipartUpload`).
  - [ ] Add configuration for multipart threshold, part size, and concurrency to the settings dialog.
- [ ] **Drag and Drop:**
  - [ ] Implement drag-and-drop from the OS to the file list for uploads.
  - [ ] Implement drag-and-drop from the file list to the OS for downloads.
- [ ] **Context Menus:** Add right-click context menus to the bucket/folder tree and the file list for quick access to CRUD operations.
- [ ] **Credential Storage (Platform-Specific):**
  - [ ] **macOS:** Replace the mock implementation with native Keychain integration for storing credentials.
  - [ ] **Windows:** Replace the mock implementation with native Credential Manager/DPAPI integration.

## Build, Packaging, and Finalization
- [ ] **Windows Installer:** Create a build pipeline to package the application as a Windows installer (`.msi` or `.exe`), including all necessary GTK runtime libraries.
- [ ] **macOS App Bundle:** Create a build pipeline to package the application as a macOS App Bundle (`.app`) and a `.dmg` disk image.
- [ ] **Code Signing:** (Optional but recommended) Implement code signing for both Windows and macOS packages.
- [ ] **Resolve Deprecation Warnings:** Refactor the code to replace deprecated GTK functions (e.g., `GtkTreeView`, `GtkTreeStore`) with their modern equivalents (`GtkListView`, `GtkColumnView`, `GtkTreeListModel`) to ensure future compatibility.
- [ ] **Testing:** Perform comprehensive manual testing for all features on both Windows and macOS.
