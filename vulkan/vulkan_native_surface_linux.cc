// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vulkan_native_surface_linux.h"

#include <xcb/xcb.h>

#include "third_party/skia/include/gpu/vk/GrVkBackendContext.h"

namespace vulkan {

VulkanNativeSurfaceLinux::VulkanNativeSurfaceLinux() {
  xcb_connection_ = xcb_connect(NULL, NULL);

  const xcb_setup_t *xcb_setup = xcb_get_setup(xcb_connection_);
  xcb_screen_iterator_t xcb_screen_iter = xcb_setup_roots_iterator(xcb_setup);
  xcb_screen_t *main_screen = xcb_screen_iter.data;

  auto xcb_mask = XCB_CW_EVENT_MASK;
  xcb_event_mask_t xcb_mask_vals[] = {(xcb_event_mask_t)(XCB_EVENT_MASK_EXPOSURE)};

  xcb_window_t window = xcb_generate_id(xcb_connection_);
  xcb_create_window(xcb_connection_, XCB_COPY_FROM_PARENT, window,
                    main_screen->root, 0, 0, 1536, 768,
                    10, XCB_WINDOW_CLASS_INPUT_OUTPUT, main_screen->root_visual,
                    xcb_mask, xcb_mask_vals);
  xcb_map_window(xcb_connection_, window);

  xcb_event_handler_thread_ = std::thread([this] {
      while (!xcb_event_handler_end_signal_)
      {
        auto new_event = xcb_wait_for_event(xcb_connection_);

        if (new_event != nullptr)
        {
            switch (new_event->response_type)
            {
            case XCB_EXPOSE: {
                xcb_expose_event_t *expose_event = (xcb_expose_event_t *)new_event;
                
                native_window_size_ = SkISize::Make(
                    expose_event->width + expose_event->x,
                    expose_event->height + expose_event->y
                );
                break;
            }
            default:
                break;
            }
        } else {
            return;
        }
      }
  });
}

VulkanNativeSurfaceLinux::~VulkanNativeSurfaceLinux() {
  xcb_event_handler_end_signal_ = true;
  xcb_event_handler_thread_.join();

  xcb_disconnect(xcb_connection_);
}

const char* VulkanNativeSurfaceLinux::GetExtensionName() const {
  return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
}

uint32_t VulkanNativeSurfaceLinux::GetSkiaExtensionName() const {
  return kKHR_xcb_surface_GrVkExtensionFlag;
}

VkSurfaceKHR VulkanNativeSurfaceLinux::CreateSurfaceHandle(
    VulkanProcTable& vk,
    const VulkanHandle<VkInstance>& instance) const {
  if (!vk.IsValid() || !instance) {
    return VK_NULL_HANDLE;
  }

  const VkXcbSurfaceCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .window = native_window_,
  };

  VkSurfaceKHR surface = VK_NULL_HANDLE;

  if (VK_CALL_LOG_ERROR(vk.CreateXcbSurfaceKHR(
          instance, &create_info, nullptr, &surface)) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return surface;
}

bool VulkanNativeSurfaceLinux::IsValid() const {
  return create_surface_failed_;
}

SkISize VulkanNativeSurfaceLinux::GetSize() const {
  return native_window_size_;
}

}  // namespace vulkan
