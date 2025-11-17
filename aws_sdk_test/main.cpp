#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListBucketsResult.h>
#include <iostream>

int main() {
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    {
        Aws::Client::ClientConfiguration config;
        config.scheme = Aws::Http::Scheme::HTTP;
        config.endpointOverride = "localhost:9000"; // For MinIO or other local S3-compatible storage

        Aws::S3::S3Client s3_client(config);
        auto outcome = s3_client.ListBuckets();

        if (outcome.IsSuccess()) {
            std::cout << "Successfully listed buckets:" << std::endl;
            for (const auto& bucket : outcome.GetResult().GetBuckets()) {
                std::cout << "  " << bucket.GetName() << std::endl;
            }
        } else {
            std::cerr << "Failed to list buckets. Error: " << outcome.GetError().GetMessage() << std::endl;
            Aws::ShutdownAPI(options);
            return 1;
        }
    }

    Aws::ShutdownAPI(options);
    std::cout << "AWS SDK shut down successfully." << std::endl;
    return 0;
}
