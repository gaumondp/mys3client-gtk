# WIP - Remaining Tasks for MyS3 Client

This document outlines the remaining work required to complete the MyS3 Client application based on the original project specification.

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
