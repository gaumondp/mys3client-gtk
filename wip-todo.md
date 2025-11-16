# WIP - Remaining Tasks for MyS3 Client

## Phase 7 Advanced Features & UX
- [ ] **Drag and Drop:**
  - [ ] Implement drag-and-drop from the OS to the file list for uploads.
  - [ ] Implement drag-and-drop from the file list to the OS for downloads.
- [ ] **Context Menus:** Add right-click context menus to the bucket/folder tree and the file list for quick access to CRUD operations.
- [ ] **Credential Storage (Platform-Specific):**
  - [ ] **macOS:** Replace the mock implementation with native Keychain integration for storing credentials.
  - [ ] **Windows:** Replace the mock implementation with native Credential Manager/DPAPI integration.

## Phase 8 Build, Packaging & Deprecations
- [x] **GTK Deprecation Warnings:** Resolve all GTK deprecation warnings that occur during the build process.
- [ ] **Testing:** Perform comprehensive manual testing for all features on both Windows and macOS.
## Phase 9 Last step
- [ ] **Revise readme and other documentation** so it is complete with instructions for Mac and Windows to install what is need to compile and create binaries on both platforms. Mac only needs Arm64 version, no Intel.
- [ ]  Add extensive logging as texte files for both compiling and for the apps running.
