#/bin/bash
rm -rf build
mkdir build
cd build
export VULKAN_SDK=/Users/danny/VulkanSDK/1.3.211.0/macOS
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios.toolchain.cmake -DPLATFORM=OS64
