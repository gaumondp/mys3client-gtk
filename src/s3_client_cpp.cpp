#include "s3_client_cpp.h"
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListBucketsResult.h>
#include <aws/s3/model/Bucket.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/CopyObjectRequest.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <iostream>
#include <fstream>
#include <memory>

static Aws::SDKOptions options;

void s3_client_cpp_init() {
    Aws::InitAPI(options);
}

void s3_client_cpp_cleanup() {
    Aws::ShutdownAPI(options);
}

#include <aws/core/auth/AWSCredentialsProvider.h>

namespace {
    Aws::S3::S3Client create_s3_client(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, gboolean use_ssl) {
        Aws::Client::ClientConfiguration clientConfig;
        if (use_ssl) {
            clientConfig.scheme = Aws::Http::Scheme::HTTPS;
        } else {
            clientConfig.scheme = Aws::Http::Scheme::HTTP;
        }
        clientConfig.endpointOverride = endpoint;

        auto credentialsProvider = Aws::MakeShared<Aws::Auth::SimpleAWSCredentialsProvider>("S3Client", access_key, secret_key);
        return Aws::S3::S3Client(credentialsProvider, clientConfig, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);
    }
} // namespace

S3ConnectionStatus s3_client_cpp_test_connection(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, gboolean use_ssl) {
    auto s3_client = create_s3_client(endpoint, access_key, secret_key, use_ssl);

    auto outcome = s3_client.ListBuckets();

    if (outcome.IsSuccess()) {
        return S3_CONNECTION_OK;
    } else if (outcome.GetError().GetErrorType() == Aws::S3::S3Errors::ACCESS_DENIED) {
        return S3_FORBIDDEN;
    } else {
        return S3_CONNECTION_FAILED;
    }
}

GList* s3_client_cpp_list_buckets(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, gboolean use_ssl, GError **error) {
    auto s3_client = create_s3_client(endpoint, access_key, secret_key, use_ssl);

    auto outcome = s3_client.ListBuckets();

    if (outcome.IsSuccess()) {
        GList *buckets = NULL;
        const auto &result_buckets = outcome.GetResult().GetBuckets();
        for (const auto &bucket : result_buckets) {
            S3Bucket *b = g_new0(S3Bucket, 1);
            b->name = g_strdup(bucket.GetName().c_str());
            b->creation_date = bucket.GetCreationDate().Millis();
            buckets = g_list_prepend(buckets, b);
        }
        return g_list_reverse(buckets);
    } else {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "%s", outcome.GetError().GetMessage().c_str());
        return NULL;
    }
}

GList* s3_client_cpp_list_objects(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *prefix, gboolean use_ssl, GError **error) {
    auto s3_client = create_s3_client(endpoint, access_key, secret_key, use_ssl);

    Aws::S3::Model::ListObjectsV2Request request;
    request.SetBucket(bucket);
    if (prefix) {
        request.SetPrefix(prefix);
    }

    auto outcome = s3_client.ListObjectsV2(request);

    if (outcome.IsSuccess()) {
        GList *objects = NULL;
        const auto &result_objects = outcome.GetResult().GetContents();
        for (const auto &object : result_objects) {
            S3Object *o = g_new0(S3Object, 1);
            o->key = g_strdup(object.GetKey().c_str());
            o->size = object.GetSize();
            o->last_modified = object.GetLastModified().Millis();
            objects = g_list_prepend(objects, o);
        }
        return g_list_reverse(objects);
    } else {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "%s", outcome.GetError().GetMessage().c_str());
        return NULL;
    }
}

gboolean s3_client_cpp_create_folder(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *folder_path, gboolean use_ssl, GError **error) {
    auto s3_client = create_s3_client(endpoint, access_key, secret_key, use_ssl);

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(bucket);
    request.SetKey(folder_path);

    auto outcome = s3_client.PutObject(request);

    if (outcome.IsSuccess()) {
        return TRUE;
    } else {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "%s", outcome.GetError().GetMessage().c_str());
        return FALSE;
    }
}

gboolean s3_client_cpp_upload_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, const gchar *local_file_path, gboolean use_ssl, GError **error) {
    auto s3_client = create_s3_client(endpoint, access_key, secret_key, use_ssl);

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(bucket);
    request.SetKey(key);

    std::shared_ptr<Aws::IOStream> input_data =
        Aws::MakeShared<Aws::FStream>("SampleAllocationTag",
                                      local_file_path,
                                      std::ios_base::in | std::ios_base::binary);
    if (!input_data->good()) {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "Failed to open file %s", local_file_path);
        return FALSE;
    }
    request.SetBody(input_data);

    auto outcome = s3_client.PutObject(request);

    if (outcome.IsSuccess()) {
        return TRUE;
    } else {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "%s", outcome.GetError().GetMessage().c_str());
        return FALSE;
    }
}

gchar* s3_client_cpp_download_object_to_buffer(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, gboolean use_ssl, gsize *length, GError **error) {
    auto s3_client = create_s3_client(endpoint, access_key, secret_key, use_ssl);

    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(bucket);
    request.SetKey(key);

    auto outcome = s3_client.GetObject(request);

    if (outcome.IsSuccess()) {
        auto &stream = outcome.GetResult().GetBody();
        std::string str((std::istreambuf_iterator<char>(stream)),
                        std::istreambuf_iterator<char>());
        *length = str.length();
        return g_strdup(str.c_str());
    } else {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "%s", outcome.GetError().GetMessage().c_str());
        return NULL;
    }
}

gboolean s3_client_cpp_download_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, const gchar *local_file_path, gboolean use_ssl, S3DownloadProgressCallback progress_callback, gpointer progress_user_data, GError **error) {
    auto s3_client = create_s3_client(endpoint, access_key, secret_key, use_ssl);

    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(bucket);
    request.SetKey(key);

    request.SetResponseStreamFactory([=]() { return Aws::New<Aws::FStream>("SampleAllocationTag", local_file_path, std::ios_base::out | std::ios_base::binary); });

    if (progress_callback) {
        request.SetDataReceivedEventHandler(std::function<void(const Aws::Http::HttpRequest*, Aws::Http::HttpResponse*, long long)>(
            [=](const Aws::Http::HttpRequest* req, Aws::Http::HttpResponse* res, long long progress) {
                if (res) {
                    const auto& headers = res->GetHeaders();
                    const auto& range = headers.find("content-range");
                    if (range != headers.end()) {
                        const std::string& range_str = range->second;
                        size_t pos = range_str.find('/');
                        if (pos != std::string::npos) {
                            long long total_bytes = std::stoll(range_str.substr(pos + 1));
                            progress_callback(progress, total_bytes, progress_user_data);
                        }
                    }
                }
            }
        ));
    }

    auto outcome = s3_client.GetObject(request);

    if (outcome.IsSuccess()) {
        return TRUE;
    } else {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "%s", outcome.GetError().GetMessage().c_str());
        return FALSE;
    }
}

gboolean s3_client_cpp_rename_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *old_key, const gchar *new_key, gboolean use_ssl, GError **error) {
    auto s3_client = create_s3_client(endpoint, access_key, secret_key, use_ssl);

    Aws::S3::Model::CopyObjectRequest copy_request;
    copy_request.SetCopySource(Aws::String(bucket) + "/" + old_key);
    copy_request.SetBucket(bucket);
    copy_request.SetKey(new_key);

    auto copy_outcome = s3_client.CopyObject(copy_request);

    if (copy_outcome.IsSuccess()) {
        Aws::S3::Model::DeleteObjectRequest delete_request;
        delete_request.SetBucket(bucket);
        delete_request.SetKey(old_key);

        auto delete_outcome = s3_client.DeleteObject(delete_request);

        if (delete_outcome.IsSuccess()) {
            return TRUE;
        } else {
            g_set_error(error, g_quark_from_static_string("S3Client"), 0, "%s", delete_outcome.GetError().GetMessage().c_str());
            return FALSE;
        }
    } else {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "%s", copy_outcome.GetError().GetMessage().c_str());
        return FALSE;
    }
}

gboolean s3_client_cpp_delete_object(const gchar *endpoint, const gchar *access_key, const gchar *secret_key, const gchar *bucket, const gchar *key, gboolean use_ssl, GError **error) {
    auto s3_client = create_s3_client(endpoint, access_key, secret_key, use_ssl);

    Aws::S3::Model::DeleteObjectRequest request;
    request.SetBucket(bucket);
    request.SetKey(key);

    auto outcome = s3_client.DeleteObject(request);

    if (outcome.IsSuccess()) {
        return TRUE;
    } else {
        g_set_error(error, g_quark_from_static_string("S3Client"), 0, "%s", outcome.GetError().GetMessage().c_str());
        return FALSE;
    }
}
