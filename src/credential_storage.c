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

// MOCK for Windows
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
