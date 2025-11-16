#include "s3_client.h"
#include <libs3.h>
#include <glib.h>
#include <stdio.h>
#include <errno.h>

// #############################################################################
// # Test Connection
// #############################################################################

typedef struct {
    FILE *file;
    S3Status status;
    GError *error;
} UploadObjectContext;

static int upload_data_callback(int bufferSize, char *buffer, void *callback_data) {
    UploadObjectContext *context = (UploadObjectContext *)callback_data;
    return fread(buffer, 1, bufferSize, context->file);
}

static void response_complete_callback_upload(S3Status status, const S3ErrorDetails *error_details, void *callback_data) {
    UploadObjectContext *context = (UploadObjectContext *)callback_data;
    context->status = status;
    if (status != S3StatusOK && error_details && error_details->message) {
        g_set_error(&context->error, g_quark_from_static_string("S3Client"), status, "S3 Error: %s", error_details->message);
    }
}

typedef struct {
    S3Status status;
    GError *error;
} TestConnectionContext;

static S3Status
response_properties_callback_test(const S3ResponseProperties *properties, void *callback_data) {
    (void)properties; (void)callback_data; return S3StatusOK;
}

static void
response_complete_callback_test(S3Status status, const S3ErrorDetails *error_details, void *callback_data) {
    TestConnectionContext *context = (TestConnectionContext *)callback_data;
    context->status = status;
    if (status != S3StatusOK && error_details && error_details->message) {
        g_set_error(&context->error, g_quark_from_static_string("S3Client"), status, "S3 Error: %s", error_details->message);
    }
}

S3ConnectionStatus
s3_client_test_connection(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, gboolean use_ssl) {
    TestConnectionContext request_context = { S3StatusInternalError, NULL };
    S3ResponseHandler response_handler = { &response_properties_callback_test, &response_complete_callback_test };
    S3Protocol protocol = use_ssl ? S3ProtocolHTTPS : S3ProtocolHTTP;

    if (S3_initialize("s3", S3_INIT_ALL, NULL) != S3StatusOK) return S3_ERROR_UNKNOWN;
    S3_test_bucket(protocol, S3UriStyleVirtualHost, access_key, secret_key, endpoint, bucket, 0, NULL, NULL, &response_handler, &request_context);
    S3_deinitialize();

    S3ConnectionStatus final_status;
    if (request_context.status == S3StatusOK) final_status = S3_CONNECTION_OK;
    else if (request_context.status == S3StatusHttpErrorForbidden || request_context.status == S3StatusErrorAccessDenied) final_status = S3_FORBIDDEN;
    else final_status = S3_CONNECTION_FAILED;

    g_clear_error(&request_context.error);
    return final_status;
}

// #############################################################################
// # List Buckets
// #############################################################################

typedef struct {
    GList *buckets;
    S3Status status;
    GError *error;
} ListBucketsContext;

static S3Status
list_buckets_callback(const char *ownerId, const char *ownerDisplayName, const char *bucketName, int64_t creationDate, void *callback_data) {
    (void)ownerId; (void)ownerDisplayName;
    ListBucketsContext *context = (ListBucketsContext *)callback_data;
    S3Bucket *bucket = g_new0(S3Bucket, 1);
    bucket->name = g_strdup(bucketName);
    bucket->creation_date = creationDate;
    context->buckets = g_list_prepend(context->buckets, bucket);
    return S3StatusOK;
}

static void
response_complete_callback_list_buckets(S3Status status, const S3ErrorDetails *error_details, void *callback_data) {
    ListBucketsContext *context = (ListBucketsContext *)callback_data;
    context->status = status;
    if (status != S3StatusOK && error_details && error_details->message) {
        g_set_error(&context->error, g_quark_from_static_string("S3Client"), status, "S3 Error: %s", error_details->message);
    }
}

GList*
s3_client_list_buckets(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, gboolean use_ssl, GError **error) {
    ListBucketsContext context = { NULL, S3StatusInternalError, NULL };
    S3ListServiceHandler handler = {
        { NULL, &response_complete_callback_list_buckets },
        &list_buckets_callback
    };
    S3Protocol protocol = use_ssl ? S3ProtocolHTTPS : S3ProtocolHTTP;

    if (S3_initialize("s3", S3_INIT_ALL, NULL) != S3StatusOK) {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "Failed to initialize libs3");
        return NULL;
    }
    S3_list_service(protocol, access_key, secret_key, endpoint, NULL, &handler, &context);
    S3_deinitialize();

    if (context.status != S3StatusOK) {
        if (context.error) {
            g_propagate_error(error, context.error);
        } else {
            g_set_error(error, g_quark_from_static_string("S3Client"), context.status, "Failed to list buckets with status: %d", context.status);
        }
        s3_client_free_bucket_list(context.buckets);
        return NULL;
    }
    return g_list_reverse(context.buckets);
}

// #############################################################################
// # List Objects
// #############################################################################

typedef struct {
    GList *objects;
    S3Status status;
    GError *error;
} ListObjectsContext;

static S3Status
list_objects_callback(int isTruncated, const char *nextMarker, int contentsCount, const S3ListBucketContent *contents, int commonPrefixesCount, const char **commonPrefixes, void *callback_data) {
    (void)isTruncated; (void)nextMarker; (void)commonPrefixesCount; (void)commonPrefixes;
    ListObjectsContext *context = (ListObjectsContext *)callback_data;
    for (int i = 0; i < contentsCount; i++) {
        S3Object *obj = g_new0(S3Object, 1);
        obj->key = g_strdup(contents[i].key);
        obj->size = contents[i].size;
        obj->last_modified = contents[i].lastModified;
        context->objects = g_list_prepend(context->objects, obj);
    }
    return S3StatusOK;
}

static void
response_complete_callback_list_objects(S3Status status, const S3ErrorDetails *error_details, void *callback_data) {
    ListObjectsContext *context = (ListObjectsContext *)callback_data;
    context->status = status;
    if (status != S3StatusOK && error_details && error_details->message) {
        g_set_error(&context->error, g_quark_from_static_string("S3Client"), status, "S3 Error: %s", error_details->message);
    }
}

GList*
s3_client_list_objects(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *prefix, gboolean use_ssl, GError **error) {
    ListObjectsContext context = { NULL, S3StatusInternalError, NULL };
    S3ListBucketHandler handler = {
        { NULL, &response_complete_callback_list_objects },
        &list_objects_callback
    };

    S3BucketContext bucket_context = {
        .hostName = endpoint,
        .bucketName = bucket,
        .protocol = use_ssl ? S3ProtocolHTTPS : S3ProtocolHTTP,
        .uriStyle = S3UriStyleVirtualHost,
        .accessKeyId = access_key,
        .secretAccessKey = secret_key,
    };

    if (S3_initialize("s3", S3_INIT_ALL, NULL) != S3StatusOK) {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "Failed to initialize libs3");
        return NULL;
    }
    S3_list_bucket(&bucket_context, prefix, NULL, "/", 1000, NULL, &handler, &context);
    S3_deinitialize();

    if (context.status != S3StatusOK) {
        if (context.error) {
            g_propagate_error(error, context.error);
        } else {
            g_set_error(error, g_quark_from_static_string("S3Client"), context.status, "Failed to list objects with status: %d", context.status);
        }
        s3_client_free_object_list(context.objects);
        return NULL;
    }
    return g_list_reverse(context.objects);
}

// #############################################################################
// # Memory Management
// #############################################################################

void s3_client_free_object_list(GList *object_list) {
    g_list_free_full(object_list, (GDestroyNotify)g_free);
}

void s3_client_free_bucket_list(GList *bucket_list) {
    g_list_free_full(bucket_list, (GDestroyNotify)g_free);
}

// #############################################################################
// # Create Folder
// #############################################################################

typedef struct {
    S3Status status;
    GError *error;
} CreateFolderContext;

static void
response_complete_callback_create_folder(S3Status status, const S3ErrorDetails *error_details, void *callback_data) {
    CreateFolderContext *context = (CreateFolderContext *)callback_data;
    context->status = status;
    if (status != S3StatusOK && error_details && error_details->message) {
        g_set_error(&context->error, g_quark_from_static_string("S3Client"), status, "S3 Error: %s", error_details->message);
    }
}

gboolean
s3_client_create_folder(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *folder_path, gboolean use_ssl, GError **error) {
    CreateFolderContext context = { S3StatusInternalError, NULL };
    S3PutObjectHandler handler = {
        { NULL, &response_complete_callback_create_folder },
        NULL // No data callback needed for a zero-byte file
    };

    S3BucketContext bucket_context = {
        .hostName = endpoint,
        .bucketName = bucket,
        .protocol = use_ssl ? S3ProtocolHTTPS : S3ProtocolHTTP,
        .uriStyle = S3UriStyleVirtualHost,
        .accessKeyId = access_key,
        .secretAccessKey = secret_key,
    };

    if (S3_initialize("s3", S3_INIT_ALL, NULL) != S3StatusOK) {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "Failed to initialize libs3");
        return FALSE;
    }

    // Create a zero-byte object with a trailing slash
    S3_put_object(&bucket_context, folder_path, 0, NULL, NULL, &handler, &context);

    S3_deinitialize();

    if (context.status != S3StatusOK) {
        if (context.error) {
            g_propagate_error(error, context.error);
        } else {
            g_set_error(error, g_quark_from_static_string("S3Client"), context.status, "Failed to create folder with status: %d", context.status);
        }
        return FALSE;
    }

    return TRUE;
}

gboolean
s3_client_rename_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *old_key, const gchar *new_key, gboolean use_ssl, GError **error) {
    // Stub
    return TRUE;
}

gboolean
s3_client_delete_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, gboolean use_ssl, GError **error) {
    // Stub
    return TRUE;
}

// #############################################################################
// # Download Object
// #############################################################################

typedef struct {
    FILE *file;
    S3Status status;
    GError *error;
    guint64 total_bytes;
    guint64 downloaded_bytes;
    S3DownloadProgressCallback progress_callback;
    gpointer progress_user_data;
} DownloadObjectContext;


static S3Status
response_properties_callback_download_object(const S3ResponseProperties *properties, void *callback_data) {
    DownloadObjectContext *context = (DownloadObjectContext *)callback_data;
    if (properties->contentLength > 0) {
        context->total_bytes = properties->contentLength;
    }
    return S3StatusOK;
}

static S3Status
download_object_data_callback(int bufferSize, const char *buffer, void *callback_data) {
    DownloadObjectContext *context = (DownloadObjectContext *)callback_data;
    size_t wrote = fwrite(buffer, 1, bufferSize, context->file);
    if (wrote < (size_t)bufferSize) {
        return S3StatusAbortedByCallback;
    }

    context->downloaded_bytes += bufferSize;
    if (context->progress_callback) {
        if (!context->progress_callback(context->downloaded_bytes, context->total_bytes, context->progress_user_data)) {
            return S3StatusAbortedByCallback;
        }
    }

    return S3StatusOK;
}

static void
response_complete_callback_download_object(S3Status status, const S3ErrorDetails *error_details, void *callback_data) {
    DownloadObjectContext *context = (DownloadObjectContext *)callback_data;
    context->status = status;
    if (status != S3StatusOK && error_details && error_details->message) {
        g_set_error(&context->error, g_quark_from_static_string("S3Client"), status, "S3 Error: %s", error_details->message);
    }
}

gboolean
s3_client_download_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, const gchar *local_file_path, gboolean use_ssl, S3DownloadProgressCallback progress_callback, gpointer progress_user_data, GError **error) {

    FILE *file = fopen(local_file_path, "w");
    if (!file) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno), "Failed to open file '%s'", local_file_path);
        return FALSE;
    }

    DownloadObjectContext context = { file, S3StatusInternalError, NULL, 0, 0, progress_callback, progress_user_data };
    S3GetObjectHandler handler = {
        { &response_properties_callback_download_object, &response_complete_callback_download_object },
        &download_object_data_callback
    };

    S3BucketContext bucket_context = {
        .hostName = endpoint,
        .bucketName = bucket,
        .protocol = use_ssl ? S3ProtocolHTTPS : S3ProtocolHTTP,
        .uriStyle = S3UriStyleVirtualHost,
        .accessKeyId = access_key,
        .secretAccessKey = secret_key,
    };

    if (S3_initialize("s3", S3_INIT_ALL, NULL) != S3StatusOK) {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "Failed to initialize libs3");
        fclose(file);
        return FALSE;
    }

    S3_get_object(&bucket_context, key, NULL, 0, 0, NULL, &handler, &context);

    S3_deinitialize();
    fclose(file);

    if (context.status != S3StatusOK) {
        if (context.error) {
            g_propagate_error(error, context.error);
        } else {
            g_set_error(error, g_quark_from_static_string("S3Client"), context.status, "Failed to download object with status: %d", context.status);
        }
        remove(local_file_path);
        return FALSE;
    }

    return TRUE;
}

// #############################################################################
// # Download Object to Buffer
// #############################################################################

typedef struct {
    GString *buffer;
    S3Status status;
    GError *error;
} DownloadBufferContext;

static S3Status
download_data_callback(int bufferSize, const char *buffer, void *callback_data) {
    DownloadBufferContext *context = (DownloadBufferContext *)callback_data;
    g_string_append_len(context->buffer, buffer, bufferSize);
    return S3StatusOK;
}

static void
response_complete_callback_download(S3Status status, const S3ErrorDetails *error_details, void *callback_data) {
    DownloadBufferContext *context = (DownloadBufferContext *)callback_data;
    context->status = status;
    if (status != S3StatusOK && error_details && error_details->message) {
        g_set_error(&context->error, g_quark_from_static_string("S3Client"), status, "S3 Error: %s", error_details->message);
    }
}

gchar*
s3_client_download_object_to_buffer(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, gboolean use_ssl, gsize *length, GError **error) {
    DownloadBufferContext context = { g_string_new(NULL), S3StatusInternalError, NULL };
    S3GetObjectHandler handler = {
        { NULL, &response_complete_callback_download },
        &download_data_callback
    };

    S3BucketContext bucket_context = {
        .hostName = endpoint,
        .bucketName = bucket,
        .protocol = use_ssl ? S3ProtocolHTTPS : S3ProtocolHTTP,
        .uriStyle = S3UriStyleVirtualHost,
        .accessKeyId = access_key,
        .secretAccessKey = secret_key,
    };

    if (S3_initialize("s3", S3_INIT_ALL, NULL) != S3StatusOK) {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "Failed to initialize libs3");
        g_string_free(context.buffer, TRUE);
        return NULL;
    }

    S3_get_object(&bucket_context, key, NULL, 0, 0, NULL, &handler, &context);

    S3_deinitialize();

    if (context.status != S3StatusOK) {
        if (context.error) {
            g_propagate_error(error, context.error);
        } else {
            g_set_error(error, g_quark_from_static_string("S3Client"), context.status, "Failed to download object with status: %d", context.status);
        }
        g_string_free(context.buffer, TRUE);
        return NULL;
    }

    *length = context.buffer->len;
    return g_string_free(context.buffer, FALSE); // Return the buffer, don't free it
}

// #############################################################################
// # Upload Object
// #############################################################################

gboolean
s3_client_upload_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, const gchar *local_file_path, gboolean use_ssl, GError **error) {

    FILE *file = fopen(local_file_path, "r");
    if (!file) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno), "Failed to open file '%s'", local_file_path);
        return FALSE;
    }

    fseek(file, 0, SEEK_END);
    uint64_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    UploadObjectContext context = { file, S3StatusInternalError, NULL };
    S3PutObjectHandler handler = {
        { NULL, &response_complete_callback_upload },
        &upload_data_callback
    };

    S3BucketContext bucket_context = {
        .hostName = endpoint,
        .bucketName = bucket,
        .protocol = use_ssl ? S3ProtocolHTTPS : S3ProtocolHTTP,
        .uriStyle = S3UriStyleVirtualHost,
        .accessKeyId = access_key,
        .secretAccessKey = secret_key,
    };

    if (S3_initialize("s3", S3_INIT_ALL, NULL) != S3StatusOK) {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "Failed to initialize libs3");
        fclose(file);
        return FALSE;
    }

    S3_put_object(&bucket_context, key, file_size, NULL, NULL, &handler, &context);

    S3_deinitialize();
    fclose(file);

    if (context.status != S3StatusOK) {
        if (context.error) {
            g_propagate_error(error, context.error);
        } else {
            g_set_error(error, g_quark_from_static_string("S3Client"), context.status, "Failed to upload object with status: %d", context.status);
        }
        return FALSE;
    }

    return TRUE;
}
