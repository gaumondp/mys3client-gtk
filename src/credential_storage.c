#include "credential_storage.h"
#include <glib.h>

// This is a mock implementation for Linux/other platforms.
// The actual implementation will need to be written for macOS (using Keychain)
// and Windows (using Credential Manager).

#if defined(__APPLE__)
#include <Security/Security.h>

gboolean credential_storage_save(const gchar *endpoint, const gchar *access_key, const gchar *secret_key) {
    OSStatus status;
    SecKeychainItemRef item = NULL;
    const char *service_name = "MyS3Client";
    g_autofree gchar *password = g_strdup_printf("%s:%s", access_key, secret_key);
    guint32 password_len = strlen(password);

    status = SecKeychainFindGenericPassword(NULL, strlen(service_name), service_name, strlen(endpoint), endpoint, 0, NULL, &item);

    if (status == errSecSuccess) {
        return SecKeychainItemModifyAttributesAndData(item, NULL, password_len, password) == errSecSuccess;
    } else if (status == errSecItemNotFound) {
        return SecKeychainAddGenericPassword(NULL, strlen(service_name), service_name, strlen(endpoint), endpoint, password_len, password, NULL) == errSecSuccess;
    }

    return FALSE;
}

gboolean credential_storage_load(const gchar *endpoint, gchar **access_key, gchar **secret_key) {
    OSStatus status;
    UInt32 passwordLength;
    void *passwordData;
    const char *service_name = "MyS3Client";

    status = SecKeychainFindGenericPassword(NULL, strlen(service_name), service_name, strlen(endpoint), endpoint, &passwordLength, &passwordData, NULL);

    if (status == errSecSuccess) {
        gchar *password = g_strndup(passwordData, passwordLength);
        gchar **parts = g_strsplit(password, ":", 2);
        g_free(password);
        SecKeychainItemFreeContent(NULL, passwordData);

        if (g_strv_length(parts) == 2) {
            *access_key = g_strdup(parts[0]);
            *secret_key = g_strdup(parts[1]);
            g_strfreev(parts);
            return TRUE;
        }
        g_strfreev(parts);
    }

    return FALSE;
}

gboolean credential_storage_delete(const gchar *endpoint) {
    OSStatus status;
    SecKeychainItemRef item = NULL;
    const char *service_name = "MyS3Client";

    status = SecKeychainFindGenericPassword(NULL, strlen(service_name), service_name, strlen(endpoint), endpoint, 0, NULL, &item);

    if (status == errSecSuccess) {
        return SecKeychainItemDelete(item) == errSecSuccess;
    }

    return status == errSecItemNotFound;
}

#elif defined(_WIN32)
#include <windows.h>
#include <wincred.h>

static LPWSTR utf8_to_utf16(const gchar *utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (len == 0) return NULL;
    LPWSTR utf16 = g_malloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, utf16, len);
    return utf16;
}

static gchar* utf16_to_utf8(LPCWSTR utf16) {
    int len = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, NULL, NULL);
    if (len == 0) return NULL;
    gchar *utf8 = g_malloc(len);
    WideCharToMultiByte(CP_UTF8, 0, utf16, -1, utf8, len, NULL, NULL);
    return utf8;
}

gboolean credential_storage_save(const gchar *endpoint, const gchar *access_key, const gchar *secret_key) {
    gboolean success = FALSE;
    LPWSTR endpoint_w = utf8_to_utf16(endpoint);
    LPWSTR access_key_w = utf8_to_utf16(access_key);

    if (endpoint_w && access_key_w) {
        CREDENTIALW cred = {0};
        cred.Type = CRED_TYPE_GENERIC;
        cred.TargetName = endpoint_w;
        cred.CredentialBlobSize = strlen(secret_key);
        cred.CredentialBlob = (LPBYTE)secret_key;
        cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
        cred.UserName = access_key_w;
        success = CredWriteW(&cred, 0);
    }

    g_free(endpoint_w);
    g_free(access_key_w);
    return success;
}

gboolean credential_storage_load(const gchar *endpoint, gchar **access_key, gchar **secret_key) {
    gboolean success = FALSE;
    LPWSTR endpoint_w = utf8_to_utf16(endpoint);
    PCREDENTIALW pcred;

    if (endpoint_w) {
        if (CredReadW(endpoint_w, CRED_TYPE_GENERIC, 0, &pcred)) {
            *access_key = utf16_to_utf8(pcred->UserName);
            *secret_key = g_strndup((gchar*)pcred->CredentialBlob, pcred->CredentialBlobSize);
            CredFree(pcred);
            success = TRUE;
        }
    }

    g_free(endpoint_w);
    return success;
}

gboolean credential_storage_delete(const gchar *endpoint) {
    gboolean success = FALSE;
    LPWSTR endpoint_w = utf8_to_utf16(endpoint);

    if (endpoint_w) {
        success = CredDeleteW(endpoint_w, CRED_TYPE_GENERIC, 0);
    }

    g_free(endpoint_w);
    return success;
}

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
