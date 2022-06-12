#/bin/bash
export VULKAN_SDK=/Users/danny/VulkanSDK/1.3.211.0/macOS
cmake . -G Xcode -B build -DCMAKE_TOOLCHAIN_FILE=ios.toolchain.cmake -DPLATFORM=OS64 -DENABLE_BITCODE=FALSE -DDEPLOYMENT_TARGET=13
