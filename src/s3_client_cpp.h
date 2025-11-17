#ifndef MYS3_S3_CLIENT_CPP_H
#define MYS3_S3_CLIENT_CPP_H

#include <glib.h>
#include "s3_client.h"

#ifdef __cplusplus
extern "C" {
#endif

void s3_client_cpp_init();
void s3_client_cpp_cleanup();

S3ConnectionStatus s3_client_cpp_test_connection(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, gboolean use_ssl);
GList* s3_client_cpp_list_buckets(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, gboolean use_ssl, GError **error);
GList* s3_client_cpp_list_objects(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *prefix, gboolean use_ssl, GError **error);
gboolean s3_client_cpp_create_folder(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *folder_path, gboolean use_ssl, GError **error);
gboolean s3_client_cpp_upload_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, const gchar *local_file_path, gboolean use_ssl, GError **error);
gchar* s3_client_cpp_download_object_to_buffer(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, gboolean use_ssl, gsize *length, GError **error);
gboolean s3_client_cpp_download_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, const gchar *local_file_path, gboolean use_ssl, S3DownloadProgressCallback progress_callback, gpointer progress_user_data, GError **error);
gboolean s3_client_cpp_rename_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *old_key, const gchar *new_key, gboolean use_ssl, GError **error);
gboolean s3_client_cpp_delete_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, gboolean use_ssl, GError **error);

#ifdef __cplusplus
}
#endif

#endif // MYS3_S3_CLIENT_CPP_H
