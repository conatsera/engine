// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_VULKAN_VULKAN_NATIVE_SURFACE_LINUX_H_
#define FLUTTER_VULKAN_VULKAN_NATIVE_SURFACE_LINUX_H_

#include <thread>
#include <mutex>

#include "flutter/fml/macros.h"
#include "vulkan_native_surface.h"

typedef uint32_t xcb_window_t;

namespace vulkan {

class VulkanNativeSurfaceLinux : public VulkanNativeSurface {
 public:
  VulkanNativeSurfaceLinux();

  ~VulkanNativeSurfaceLinux();

  const char* GetExtensionName() const override;

  uint32_t GetSkiaExtensionName() const override;

  VkSurfaceKHR CreateSurfaceHandle(
      VulkanProcTable& vk,
      const VulkanHandle<VkInstance>& instance) const override;

  bool IsValid() const override;

  SkISize GetSize() const override;

 private:
  xcb_connection_t* xcb_connection_;
  xcb_window_t native_window_;

  bool create_surface_failed_ = false;

  std::mutex native_window_size_mutex_;
  SkISize native_window_size_ = SkISize::Make(0, 0);

  bool xcb_event_handler_end_signal_ = false;
  std::thread xcb_event_handler_thread_;

  FML_DISALLOW_COPY_AND_ASSIGN(VulkanNativeSurfaceLinux);
};

}  // namespace vulkan

#endif  // FLUTTER_VULKAN_VULKAN_NATIVE_SURFACE_LINUX_H_
