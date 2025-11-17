# About This Directory

This directory contains a minimal, standalone CMake project for testing and debugging the AWS SDK for C++ integration in isolation from the main MyS3Client GTK application.

## Purpose

The primary purpose of this project is to provide a clean and simple environment to:

*   **Verify the AWS SDK Build:** Ensure that the AWS SDK for C++ is correctly built and configured.
*   **Test S3 Connectivity:** Run simple S3 operations (like `ListBuckets`) to confirm that the SDK can communicate with an S3-compatible service.
*   **Debug SDK Issues:** Isolate and debug any potential issues with the AWS SDK without the added complexity of the GTK application's event loop and UI.

## How to Use

1.  **Configure:** From the `aws_sdk_test/build` directory, run CMake and point it to the local SDK installation:
    ```bash
    cmake -DCMAKE_PREFIX_PATH=/path/to/mys3-client/subprojects/aws-sdk-cpp/install ..
    ```

2.  **Build:**
    ```bash
    make
    ```

3.  **Run:**
    ```bash
    ./aws_sdk_test
    ```

**Note:** The `main.cpp` file may be modified to test different S3 operations or configurations as needed for debugging.
