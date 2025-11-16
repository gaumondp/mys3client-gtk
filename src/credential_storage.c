#include "credential_storage.h"
#include <glib.h>
#include <stdio.h>

#if defined(__APPLE__)
#include <Security/Security.h>

// Helper function to save or update a password in the Keychain.
static gboolean save_or_update_password(const gchar *service, const gchar *account, const gchar *password) {
    OSStatus status = SecKeychainAddGenericPassword(NULL,
                                                    strlen(service), service,
                                                    strlen(account), account,
                                                    strlen(password), password,
                                                    NULL);

    if (status == errSecDuplicateItem) {
        SecKeychainItemRef itemRef = NULL;
        status = SecKeychainFindGenericPassword(NULL,
                                                strlen(service), service,
                                                strlen(account), account,
                                                NULL, NULL, &itemRef);
        if (status == errSecSuccess) {
            status = SecKeychainItemModifyAttributesAndData(itemRef, NULL, strlen(password), password);
            CFRelease(itemRef);
        }
    }
    return status == errSecSuccess;
}

// Helper function to find a password in the Keychain.
static gboolean find_password(const gchar *service, const gchar *account, gchar **password) {
    void *passwordData = NULL;
    UInt32 passwordLength = 0;
    OSStatus status = SecKeychainFindGenericPassword(NULL,
                                                     strlen(service), service,
                                                     strlen(account), account,
                                                     &passwordLength, &passwordData,
                                                     NULL);

    if (status == errSecSuccess) {
        *password = g_strndup((const gchar *)passwordData, passwordLength);
        SecKeychainItemFreeContent(NULL, passwordData);
        return TRUE;
    }
    *password = NULL;
    return FALSE;
}

// Helper function to delete a password from the Keychain.
static gboolean delete_password(const gchar *service, const gchar *account) {
    SecKeychainItemRef itemRef = NULL;
    OSStatus status = SecKeychainFindGenericPassword(NULL,
                                                     strlen(service), service,
                                                     strlen(account), account,
                                                     NULL, NULL, &itemRef);

    if (status == errSecSuccess) {
        status = SecKeychainItemDelete(itemRef);
        CFRelease(itemRef);
        return status == errSecSuccess;
    }
    return status == errSecItemNotFound;
}

gboolean credential_storage_save(const gchar *endpoint,
                                 const gchar *access_key,
                                 const gchar *secret_key) {
    const gchar *access_key_service = "MyS3Client Access Key";
    const gchar *secret_key_service = "MyS3Client Secret Key";

    gboolean access_ok = save_or_update_password(access_key_service, endpoint, access_key);
    gboolean secret_ok = save_or_update_password(secret_key_service, endpoint, secret_key);

    return access_ok && secret_ok;
}

gboolean credential_storage_load(const gchar *endpoint,
                                 gchar **access_key,
                                 gchar **secret_key) {
    const gchar *access_key_service = "MyS3Client Access Key";
    const gchar *secret_key_service = "MyS3Client Secret Key";

    gboolean access_ok = find_password(access_key_service, endpoint, access_key);
    gboolean secret_ok = find_password(secret_key_service, endpoint, secret_key);

    if (!access_ok || !secret_ok) {
        g_free(*access_key);
        g_free(*secret_key);
        *access_key = NULL;
        *secret_key = NULL;
        return FALSE;
    }
    return TRUE;
}

gboolean credential_storage_delete(const gchar *endpoint) {
    const gchar *access_key_service = "MyS3Client Access Key";
    const gchar *secret_key_service = "MyS3Client Secret Key";

    gboolean access_ok = delete_password(access_key_service, endpoint);
    gboolean secret_ok = delete_password(secret_key_service, endpoint);
    return access_ok && secret_ok;
}

#elif defined(_WIN32)

#include <windows.h>
#include <wincred.h>

// Helper to convert UTF-8 to wide-char, caller must free the result.
static WCHAR* utf8_to_wchar(const gchar* utf8_str) {
    if (!utf8_str) return NULL;
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    WCHAR* wide_str = g_new(WCHAR, size_needed);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wide_str, size_needed);
    return wide_str;
}

// Helper to convert wide-char to UTF-8, caller must free the result.
static gchar* wchar_to_utf8(const WCHAR* wide_str) {
    if (!wide_str) return NULL;
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, NULL, 0, NULL, NULL);
    gchar* utf8_str = g_new(gchar, size_needed);
    WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, utf8_str, size_needed, NULL, NULL);
    return utf8_str;
}


gboolean credential_storage_save(const gchar *endpoint,
                                 const gchar *access_key,
                                 const gchar *secret_key) {
    WCHAR* target_name = utf8_to_wchar(endpoint);
    WCHAR* user_name = utf8_to_wchar(access_key);

    CREDENTIALW cred = {0};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = target_name;
    cred.CredentialBlobSize = (DWORD)strlen(secret_key);
    cred.CredentialBlob = (LPBYTE)secret_key;
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = user_name;

    BOOL result = CredWriteW(&cred, 0);

    g_free(target_name);
    g_free(user_name);

    return result;
}

gboolean credential_storage_load(const gchar *endpoint,
                                 gchar **access_key,
                                 gchar **secret_key) {
    WCHAR* target_name = utf8_to_wchar(endpoint);
    PCREDENTIALW pcred;

    BOOL result = CredReadW(target_name, CRED_TYPE_GENERIC, 0, &pcred);

    if (result) {
        *access_key = wchar_to_utf8(pcred->UserName);
        *secret_key = g_strndup((const gchar*)pcred->CredentialBlob, pcred->CredentialBlobSize);
        CredFree(pcred);
    } else {
        *access_key = NULL;
        *secret_key = NULL;
    }

    g_free(target_name);
    return result;
}

gboolean credential_storage_delete(const gchar *endpoint) {
    WCHAR* target_name = utf8_to_wchar(endpoint);
    BOOL result = CredDeleteW(target_name, CRED_TYPE_GENERIC, 0);
    g_free(target_name);
    return result;
}

#else

// Fallback for other platforms (e.g., Linux)
gboolean credential_storage_save(const gchar *endpoint,
                                 const gchar *access_key,
                                 const gchar *secret_key) {
    (void)endpoint;
    (void)access_key;
    (void)secret_key;
    g_warning("Credential storage is not implemented on this platform. "
              "Secrets will NOT be saved.");
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
    return FALSE;
}

gboolean credential_storage_delete(const gchar *endpoint) {
    (void)endpoint;
    g_warning("Credential storage is not implemented on this platform. "
              "Nothing to delete.");
    return TRUE;
}

#endif
