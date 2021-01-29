// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/embedder/tests/embedder_test_context_vulkan.h"

#include <memory>

#include "flutter/fml/logging.h"

namespace flutter {
namespace testing {

EmbedderTestContextVulkan::EmbedderTestContextVulkan(std::string assets_path)
    : EmbedderTestContext(assets_path) {
  vulkan_context_ = std::make_unique<TestVulkanContext>();
}

EmbedderTestContextVulkan::~EmbedderTestContextVulkan() {}

void EmbedderTestContextVulkan::SetupSurface(SkISize surface_size) {
  FML_CHECK(surface_size_.isEmpty());
  surface_size_ = surface_size;
}

size_t EmbedderTestContextVulkan::GetSurfacePresentCount() const {
  return present_count_;
}

EmbedderTestContextType EmbedderTestContextVulkan::GetContextType() const {
  return EmbedderTestContextType::kVulkanContext;
}

void EmbedderTestContextVulkan::SetupCompositor() {
  FML_CHECK(false) << "Compositor rendering not supported in vulkan.";
}

TestVulkanContext* EmbedderTestContextVulkan::GetTestVulkanContext() {
  return vulkan_context_.get();
}

bool EmbedderTestContextVulkan::Present(int64_t texture_id) {
  //FireRootSurfacePresentCallbackIfPresent([&]() {
  //});
  present_count_++;
  return vulkan_context_->Present(texture_id);
}

}  // namespace testing
}  // namespace flutter
