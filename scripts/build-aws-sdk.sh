#!/bin/bash
set -e

# Configuration
SDK_VERSION="1.9.379"
SDK_REPO="https://github.com/aws/aws-sdk-cpp.git"
SDK_DIR="scripts/aws-sdk-cpp"
INSTALL_DIR="$(pwd)/scripts/dist"

# Clone the SDK if it doesn't exist
if [ ! -d "$SDK_DIR" ]; then
  git clone --recurse-submodules --branch "$SDK_VERSION" "$SDK_REPO" "$SDK_DIR"
else
  echo "SDK directory already exists. Skipping clone."
  cd "$SDK_DIR"
  git fetch
  git checkout "$SDK_VERSION"
  git submodule update --init --recursive
  cd ../..
fi

# Build and install the SDK
cd "$SDK_DIR"
mkdir -p build
cd build

cmake .. \
  -G "Ninja" \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
  -DBUILD_SHARED_LIBS=OFF \
  -DMINIMIZE_SIZE=ON \
  -DENABLE_TESTING=OFF \
  -DAUTORUN_UNIT_TESTS=OFF \
  -DBUILD_ONLY="s3;core" \
  -DCMAKE_CXX_FLAGS="-Wno-error=deprecated-declarations" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5

cmake --build .
cmake --build . --target install

echo "AWS SDK for C++ has been successfully built and installed to $INSTALL_DIR"
