#include "credential_storage.h"
#include <glib.h>

// This is a mock implementation for Linux/other platforms.
// The actual implementation will need to be written for macOS (using Keychain)
// and Windows (using Credential Manager).

#if defined(__APPLE__)
// #############################################################################
// # MAC OS IMPLEMENTATION (using Keychain Services)
// #############################################################################

#error "macOS Keychain implementation is not yet written."

#elif defined(_WIN32)
// #############################################################################
// # WINDOWS IMPLEMENTATION (using Credential Manager)
// #############################################################################

#error "Windows Credential Manager implementation is not yet written."

#else
// #############################################################################
// # MOCK IMPLEMENTATION (for Linux/development)
// #############################################################################

#include <stdio.h>

// MOCK: In a real Linux implementation, we would use something like libsecret.
// For this project, we will just print a warning and do nothing to avoid
// storing secrets in plaintext.

gboolean credential_storage_save(const gchar *endpoint,
                                 const gchar *access_key,
                                 const gchar *secret_key) {
    (void)endpoint;
    (void)access_key;
    (void)secret_key;
    g_warning("Credential storage is not implemented on this platform. "
              "Secrets will NOT be saved.");
    // Return TRUE to allow the app to function, even though secrets aren't saved.
    return TRUE;
}

gboolean credential_storage_load(const gchar *endpoint,
                                 gchar **access_key,
                                 gchar **secret_key) {
    (void)endpoint;
    *access_key = NULL;
    *secret_key = NULL;
    g_message("Credential storage is not implemented on this platform. "
              "No secrets loaded.");
    return FALSE; // Return FALSE to indicate no credentials were loaded
}

gboolean credential_storage_delete(const gchar *endpoint) {
    (void)endpoint;
    g_warning("Credential storage is not implemented on this platform. "
              "Nothing to delete.");
    return TRUE;
}

#endif
