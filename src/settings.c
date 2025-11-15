#include "settings.h"
#include <glib.h>
#include <glib/gi18n.h>

// Helper function to get the config file path
static gchar *
settings_get_config_path(void) {
    const gchar *config_dir = g_get_user_config_dir();
    return g_build_filename(config_dir, "mys3-client", "settings.ini", NULL);
}

MyS3Settings *
settings_load(void) {
    MyS3Settings *settings = g_new0(MyS3Settings, 1);
    g_autoptr(GKeyFile) key_file = g_key_file_new();
    gchar *file_path = settings_get_config_path();
    g_autoptr(GError) error = NULL;

    // Set defaults
    settings->use_ssl = TRUE;
    settings->use_path_style = FALSE;

    if (g_key_file_load_from_file(key_file, file_path, G_KEY_FILE_NONE, &error)) {
        settings->endpoint = g_key_file_get_string(key_file, "Connection", "Endpoint", NULL);
        settings->region = g_key_file_get_string(key_file, "Connection", "Region", NULL);
        settings->bucket = g_key_file_get_string(key_file, "Connection", "Bucket", NULL);
        settings->use_ssl = g_key_file_get_boolean(key_file, "Connection", "UseSSL", &error);
        if (error) {
            g_warning("Could not parse UseSSL: %s. Defaulting to true.", error->message);
            g_clear_error(&error);
            settings->use_ssl = TRUE;
        }
        settings->use_path_style = g_key_file_get_boolean(key_file, "Connection", "PathStyle", &error);
        if (error) {
            g_warning("Could not parse PathStyle: %s. Defaulting to false.", error->message);
            g_clear_error(&error);
            settings->use_path_style = FALSE;
        }
    } else {
        g_debug("Could not load settings file: %s", error->message);
    }

    g_free(file_path);
    return settings;
}

void
settings_save(MyS3Settings *settings) {
    g_autoptr(GKeyFile) key_file = g_key_file_new();
    gchar *file_path = settings_get_config_path();
    gchar *dir_path = g_path_get_dirname(file_path);
    g_autoptr(GError) error = NULL;

    g_mkdir_with_parents(dir_path, 0755);

    g_key_file_set_string(key_file, "Connection", "Endpoint", settings->endpoint);
    g_key_file_set_string(key_file, "Connection", "Region", settings->region);
    g_key_file_set_string(key_file, "Connection", "Bucket", settings->bucket);
    g_key_file_set_boolean(key_file, "Connection", "UseSSL", settings->use_ssl);
    g_key_file_set_boolean(key_file, "Connection", "PathStyle", settings->use_path_style);

    if (!g_key_file_save_to_file(key_file, file_path, &error)) {
        g_warning("Failed to save settings: %s", error->message);
    }

    g_free(file_path);
    g_free(dir_path);
}

void
settings_free(MyS3Settings *settings) {
    if (!settings) return;
    g_free(settings->endpoint);
    g_free(settings->region);
    g_free(settings->bucket);
    g_free(settings);
}
