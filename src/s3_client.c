#include "s3_client.h"
#include "s3_client_cpp.h"
#include <glib.h>

void s3_client_init() {
    s3_client_cpp_init();
}

void s3_client_cleanup() {
    s3_client_cpp_cleanup();
}

S3ConnectionStatus
s3_client_test_connection(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, gboolean use_ssl) {
    return s3_client_cpp_test_connection(endpoint, access_key, secret_key, bucket, use_ssl);
}

GList*
s3_client_list_buckets(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, gboolean use_ssl, GError **error) {
    return s3_client_cpp_list_buckets(endpoint, access_key, secret_key, use_ssl, error);
}

GList*
s3_client_list_objects(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *prefix, gboolean use_ssl, GError **error) {
    return s3_client_cpp_list_objects(endpoint, access_key, secret_key, bucket, prefix, use_ssl, error);
}

gboolean
s3_client_create_folder(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *folder_path, gboolean use_ssl, GError **error) {
    return s3_client_cpp_create_folder(endpoint, access_key, secret_key, bucket, folder_path, use_ssl, error);
}

gboolean
s3_client_upload_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, const gchar *local_file_path, gboolean use_ssl, GError **error) {
    return s3_client_cpp_upload_object(endpoint, access_key, secret_key, bucket, key, local_file_path, use_ssl, error);
}

gchar*
s3_client_download_object_to_buffer(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, gboolean use_ssl, gsize *length, GError **error) {
    return s3_client_cpp_download_object_to_buffer(endpoint, access_key, secret_key, bucket, key, use_ssl, length, error);
}

gboolean
s3_client_download_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, const gchar *local_file_path, gboolean use_ssl, S3DownloadProgressCallback progress_callback, gpointer progress_user_data, GError **error) {
    return s3_client_cpp_download_object(endpoint, access_key, secret_key, bucket, key, local_file_path, use_ssl, progress_callback, progress_user_data, error);
}

gboolean
s3_client_rename_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *old_key, const gchar *new_key, gboolean use_ssl, GError **error) {
    return s3_client_cpp_rename_object(endpoint, access_key, secret_key, bucket, old_key, new_key, use_ssl, error);
}

gboolean
s3_client_delete_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, gboolean use_ssl, GError **error) {
    return s3_client_cpp_delete_object(endpoint, access_key, secret_key, bucket, key, use_ssl, error);
}

void s3_client_free_object_list(GList *object_list) {
    g_list_free_full(object_list, (GDestroyNotify)g_free);
}

void s3_client_free_bucket_list(GList *bucket_list) {
    g_list_free_full(bucket_list, (GDestroyNotify)g_free);
}
