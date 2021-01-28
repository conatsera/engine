// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_TESTING_TEST_VULKAN_CONTEXT_H_
#define FLUTTER_TESTING_TEST_VULKAN_CONTEXT_H_

#include <map>
#include <mutex>

#include "third_party/skia/include/gpu/GrDirectContext.h"
#include "third_party/skia/include/gpu/vk/GrVkTypes.h"

#include "flutter/shell/gpu/gpu_surface_vulkan.h"

namespace flutter {

// A wrapper around vulkan::VulkanWindow to provide just what is needed for testing
class TestVulkanContext {
 public:
  struct TextureInfo {
    int64_t texture_id;
    void* texture;
  };

  TestVulkanContext();

  ~TestVulkanContext();

  void* GetVulkanDevice() const;

  void* GetVulkanPresentCommandPool() const;

  sk_sp<GrDirectContext> GetSkiaContext() const;

  /// Returns texture_id = -1 when texture creation fails.
  TextureInfo CreateVulkanTexture(const SkISize& size);

  bool Present(int64_t texture_id);

  TextureInfo GetTextureInfo(int64_t texture_id);

 private:
  sk_sp<GrDirectContext> skia_context_;

  std::unique_ptr<GPUSurfaceVulkan> vulkan_surface_;
};

}  // namespace flutter

#endif  // FLUTTER_TESTING_TEST_VULKAN_CONTEXT_H_
