# WIP - Remaining Tasks for MyS3 Client

## Step 7 Advanced Features & UX
- [ ] **Drag and Drop:**
  - [ ] Implement drag-and-drop from the OS to the file list for uploads only.

## Step 8
- [ ] **Credential Storage (Platform-Specific):**
  - [x] **macOS:** Replace the mock implementation with native Keychain integration for storing credentials.
  - [ ] **Windows:** Replace the mock implementation with native Credential Manager/DPAPI integration.

## Step 9 Build, Packaging & Deprecations
- [x] **GTK Deprecation Warnings:** Resolve all GTK deprecation warnings that occur during the build process.

## Step 10
- [ ]  Add extensive logging as texte files for both compiling and for the apps running.

## Step 11 test
- [ ] **Testing:** Perform comprehensive manual testing for all features on both Windows and macOS.

## Phase 12 docs
- [ ] **Revise readme and other documentation** based on actual code so it is complete with instructions for Mac and Windows to install what is need to compile and create binaries on both platforms. Mac only needs Arm64 support, not Intel.
