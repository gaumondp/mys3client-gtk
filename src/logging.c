#include "logging.h"
#include <glib/gstdio.h>
#include <glib.h>
#include <stdio.h>
#include <time.h>

static FILE* log_file = NULL;
static LogLevel current_log_level = LOG_LEVEL_DISABLED;
static gchar* log_dir = NULL;

// Forward declaration
static GLogWriterOutput log_writer(GLogLevelFlags log_level, const GLogField *fields, gsize n_fields, gpointer user_data);

const gchar* logging_get_directory() {
    return log_dir;
}

static gchar* get_log_directory() {
#ifdef __APPLE__
    const gchar* home_dir = g_get_home_dir();
    return g_build_filename(home_dir, "Library", "Logs", "MyS3Client", NULL);
#elif defined(_WIN32)
    const gchar* appdata_dir = g_get_user_special_dir(G_USER_DIRECTORY_APPDATA);
    return g_build_filename(appdata_dir, "MyS3Client", "Logs", NULL);
#else // Linux
    const gchar* state_dir = g_get_user_state_dir();
    return g_build_filename(state_dir, "MyS3Client", "logs", NULL);
#endif
}

static void cleanup_old_logs() {
    GDir *dir;
    const gchar *filename;
    GError *error = NULL;
    GList *files = NULL;

    dir = g_dir_open(log_dir, 0, &error);
    if (error) {
        g_warning("Error opening log directory: %s", error->message);
        g_error_free(error);
        return;
    }

    while ((filename = g_dir_read_name(dir))) {
        if (g_str_has_suffix(filename, ".log")) {
            gchar *full_path = g_build_filename(log_dir, filename, NULL);
            files = g_list_prepend(files, full_path);
        }
    }
    g_dir_close(dir);

    files = g_list_sort(files, (GCompareFunc)g_strcmp0);

    guint file_count = g_list_length(files);
    if (file_count > 5) {
        guint files_to_delete = file_count - 5;
        for (guint i = 0; i < files_to_delete; i++) {
            gchar *file_to_delete = (gchar*)g_list_nth_data(files, i);
            g_remove(file_to_delete);
        }
    }

    g_list_free_full(files, g_free);
}

void logging_init() {
    log_dir = get_log_directory();
    g_mkdir_with_parents(log_dir, 0755);

    cleanup_old_logs();

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    gchar filename[256];
    strftime(filename, sizeof(filename), "mys3-client-%Y-%m-%d-%H-%M-%S.log", tm);

    gchar* log_file_path = g_build_filename(log_dir, filename, NULL);

    log_file = g_fopen(log_file_path, "a");
    g_free(log_file_path);

    if (!log_file) {
        g_warning("Could not open log file.");
        g_free(log_dir);
        log_dir = NULL;
        return;
    }

    g_log_set_writer_func(log_writer, NULL, NULL);
}

void logging_cleanup() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    g_log_set_writer_func(g_log_writer_default, NULL, NULL);
    g_free(log_dir);
    log_dir = NULL;
}

void logging_set_level(LogLevel level) {
    current_log_level = level;
}

LogLevel logging_get_level() {
    return current_log_level;
}

static const gchar* level_to_string(GLogLevelFlags level) {
    if (level & G_LOG_LEVEL_ERROR) return "ERROR";
    if (level & G_LOG_LEVEL_CRITICAL) return "CRITICAL";
    if (level & G_LOG_LEVEL_WARNING) return "WARNING";
    if (level & G_LOG_LEVEL_MESSAGE) return "MESSAGE";
    if (level & G_LOG_LEVEL_INFO) return "INFO";
    if (level & G_LOG_LEVEL_DEBUG) return "DEBUG";
    return "LOG";
}

static GLogWriterOutput log_writer(GLogLevelFlags log_level, const GLogField *fields, gsize n_fields, gpointer user_data) {
    (void)user_data;
    if (current_log_level == LOG_LEVEL_DISABLED) {
        return g_log_writer_default(log_level, fields, n_fields, user_data);
    }

    gboolean should_log = FALSE;
    switch (current_log_level) {
        case LOG_LEVEL_DEBUG: should_log = TRUE; break;
        case LOG_LEVEL_WARNING: should_log = (log_level & (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_ERROR)); break;
        case LOG_LEVEL_ERROR: should_log = (log_level & (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_ERROR)); break;
        case LOG_LEVEL_DISABLED: break;
    }

    if (should_log && log_file) {
        const gchar *message = NULL;
        const gchar *log_domain = NULL;
        const gchar *code_file = NULL;
        const gchar *code_line = NULL;
        const gchar *code_func = NULL;

        for (gsize i = 0; i < n_fields; i++) {
            if (g_strcmp0(fields[i].key, "MESSAGE") == 0) message = fields[i].value;
            else if (g_strcmp0(fields[i].key, "GLIB_DOMAIN") == 0) log_domain = fields[i].value;
            else if (g_strcmp0(fields[i].key, "CODE_FILE") == 0) code_file = fields[i].value;
            else if (g_strcmp0(fields[i].key, "CODE_LINE") == 0) code_line = fields[i].value;
            else if (g_strcmp0(fields[i].key, "CODE_FUNC") == 0) code_func = fields[i].value;
        }

        GDateTime *now = g_date_time_new_now_local();
        gchar *iso8601_time = g_date_time_format_iso8601(now);

        fprintf(log_file, "[%s] [%s] [%s:%s:%s] %s: %s\n",
                iso8601_time,
                level_to_string(log_level),
                code_file ? code_file : "unknown",
                code_line ? code_line : "0",
                code_func ? code_func : "unknown",
                log_domain ? log_domain : "default",
                message ? message : "");
        fflush(log_file);

        g_free(iso8601_time);
        g_date_time_unref(now);
    }

    return g_log_writer_default(log_level, fields, n_fields, user_data);
}
