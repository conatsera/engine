#include "flutter/testing/test_vulkan_context.h"

#include "flutter/vulkan/vulkan_native_surface_linux.h"

namespace flutter {

TestVulkanContext::TestVulkanContext() {
    auto vulkan_surface_linux =
        std::make_unique<vulkan::VulkanNativeSurfaceLinux>();

    if (!vulkan_surface_linux->IsValid()) {
        return;
    }

}

}  // namespace flutter