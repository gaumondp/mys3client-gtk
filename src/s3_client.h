#ifndef MYS3_S3_CLIENT_H
#define MYS3_S3_CLIENT_H

#include <glib.h>

// Represents a single S3 object (a file)
typedef struct {
    gchar *key;
    guint64 size;
    gint64 last_modified;
} S3Object;

// Represents a single bucket
typedef struct {
    gchar *name;
    gint64 creation_date;
} S3Bucket;


typedef enum {
  S3_CONNECTION_OK,
  S3_CONNECTION_FAILED,
  S3_FORBIDDEN,
  S3_ERROR_UNKNOWN
} S3ConnectionStatus;

S3ConnectionStatus s3_client_test_connection(const gchar *endpoint,
                                             const gchar *access_key,
                                             const gchar *secret_key,
                                             const gchar *bucket,
                                             gboolean use_ssl);

GList* s3_client_list_buckets(const gchar *endpoint,
                              const gchar *access_key,
                              const gchar *secret_key,
                              gboolean use_ssl,
                              GError **error);

GList* s3_client_list_objects(const gchar *endpoint,
                               const gchar *access_key,
                               const gchar *secret_key,
                               const gchar *bucket,
                               const gchar *prefix,
                               gboolean use_ssl,
                               GError **error);

gboolean s3_client_create_folder(const gchar *endpoint,
                                 const gchar *access_key,
                                 const gchar *secret_key,
                                 const gchar *bucket,
                                 const gchar *folder_path,
                                 gboolean use_ssl,
                                 GError **error);

gboolean s3_client_upload_object(const gchar *endpoint,
                                 const gchar *access_key,
                                 const gchar *secret_key,
                                 const gchar *bucket,
                                 const gchar *key,
                                 const gchar *local_file_path,
                                 gboolean use_ssl,
                                 GError **error);

gchar* s3_client_download_object_to_buffer(const gchar *endpoint,
                                           const gchar *access_key,
                                           const gchar *secret_key,
                                           const gchar *bucket,
                                           const gchar *key,
                                           gboolean use_ssl,
                                           gsize *length,
                                           GError **error);

gboolean s3_client_download_object(const gchar *endpoint,
                                   const gchar *access_key,
                                   const gchar *secret_key,
                                   const gchar *bucket,
                                   const gchar *key,
                                   const gchar *local_file_path,
                                   gboolean use_ssl,
                                   S3DownloadProgressCallback progress_callback,
                                   gpointer progress_user_data,
                                   GError **error);

typedef gboolean (*S3DownloadProgressCallback)(guint64 downloaded_bytes,
                                             guint64 total_bytes,
                                             gpointer user_data);

void s3_client_free_object_list(GList *object_list);
void s3_client_free_bucket_list(GList *bucket_list);


#endif // MYS3_S3_CLIENT_H
