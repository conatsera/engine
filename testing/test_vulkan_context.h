// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_TESTING_TEST_VULKAN_CONTEXT_H_
#define FLUTTER_TESTING_TEST_VULKAN_CONTEXT_H_

#include <map>
#include <mutex>

#include "third_party/skia/include/gpu/GrDirectContext.h"
#include "third_party/skia/include/gpu/vk/GrVkTypes.h"

#include "flutter/vulkan/vulkan_application.h"
#include "flutter/vulkan/vulkan_native_surface.h"

namespace flutter {

// A wrapper around vulkan::VulkanWindow to provide just what is needed for testing
class TestVulkanContext {
 public:
  typedef struct {
    VkImage image;
    VkImageView view;
  } VulkanTexture;
  struct TextureInfo {
    int64_t texture_id;
    VkImage texture;
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
  bool CreateSkiaBackendContext(GrVkBackendContext* context);
  sk_sp<GrDirectContext> skia_context_;

  fml::RefPtr<vulkan::VulkanProcTable> proc_table_;

  std::unique_ptr<vulkan::VulkanNativeSurface> vulkan_native_surface_;
  std::unique_ptr<vulkan::VulkanApplication> vulkan_app_;

  std::mutex textures_mutex; // Guards both texture_id_ctr_ and textures_
  int64_t texture_id_ctr_ = 1;
  std::map<int64_t, VkImage> textures_;
};

}  // namespace flutter

#endif  // FLUTTER_TESTING_TEST_VULKAN_CONTEXT_H_
