#include "flutter/testing/test_vulkan_context.h"

#include "flutter/vulkan/vulkan_device.h"
#include "flutter/vulkan/vulkan_native_surface_linux.h"
#include "flutter/vulkan/vulkan_surface.h"

#include "flutter/common/graphics/persistent_cache.h"

namespace flutter {

TestVulkanContext::TestVulkanContext()
    : proc_table_(fml::MakeRefCounted<vulkan::VulkanProcTable>()) {
#ifdef OS_LINUX
  vulkan_native_surface_ = std::make_unique<vulkan::VulkanNativeSurfaceLinux>();
#elif OS_ANDROID
  vulkan_native_surface_ =
      std::make_unique<vulkan::VulkanNativeSurfaceAndroid>();
#endif

  if (!vulkan_native_surface_->IsValid()) {
    FML_DLOG(INFO) << "Native surface is invalid.";
    return;
  }

  if (!proc_table_->HasAcquiredMandatoryProcAddresses()) {
    FML_DLOG(INFO) << "Vulkan proc table was not initialized.";
    return;
  }

  std::vector<std::string> extensions = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      vulkan_native_surface_->GetExtensionName()};

  vulkan_app_ = std::make_unique<vulkan::VulkanApplication>(
      *proc_table_, "FlutterTest", std::move(extensions),
      VK_MAKE_VERSION(1, 0, 0), VK_MAKE_VERSION(1, 1, 0), true);

  if (!vulkan_app_->IsValid()) {
    FML_DLOG(INFO) << "Vulkan app is invalid.";
    return;
  }

  GrVkBackendContext backend_context;

  if (!CreateSkiaBackendContext(&backend_context)) {
    FML_DLOG(ERROR) << "Could not create Skia backend context.";
    return;
  }

  GrContextOptions options;
  if (PersistentCache::cache_sksl()) {
    options.fShaderCacheStrategy = GrContextOptions::ShaderCacheStrategy::kSkSL;
  }
  PersistentCache::MarkStrategySet();
  options.fPersistentCache = PersistentCache::GetCacheForProcess();

  sk_sp<GrDirectContext> context =
      GrDirectContext::MakeVulkan(backend_context, options);

  if (context == nullptr) {
    FML_DLOG(ERROR) << "Failed to create GrDirectContext";
    return;
  }

  context->setResourceCacheLimits(vulkan::kGrCacheMaxCount,
                                  vulkan::kGrCacheMaxByteSize);

  skia_context_ = context;
}

void* TestVulkanContext::GetVulkanDevice() const {
  return vulkan_app_->AcquireFirstCompatibleLogicalDevice()->GetHandle();
}

void* TestVulkanContext::GetVulkanPresentCommandPool() const {
  return vulkan_app_->AcquireFirstCompatibleLogicalDevice()->GetCommandPool();
}

bool TestVulkanContext::CreateSkiaBackendContext(GrVkBackendContext* context) {
  auto getProc = proc_table_->CreateSkiaGetProc();
  auto logical_device = vulkan_app_->AcquireFirstCompatibleLogicalDevice();

  if (getProc == nullptr) {
    FML_DLOG(ERROR) << "GetProcAddress is null";
    return false;
  }

  uint32_t skia_features = 0;
  if (!logical_device->GetPhysicalDeviceFeaturesSkia(&skia_features)) {
    FML_DLOG(ERROR) << "Failed to get Physical Device features";
    return false;
  }

  context->fInstance = vulkan_app_->GetInstance();
  context->fPhysicalDevice = logical_device->GetPhysicalDeviceHandle();
  context->fDevice = logical_device->GetHandle();
  context->fQueue = logical_device->GetQueueHandle();
  context->fGraphicsQueueIndex = logical_device->GetGraphicsQueueIndex();
  context->fMinAPIVersion = vulkan_app_->GetAPIVersion();
  context->fMaxAPIVersion = vulkan_app_->GetAPIVersion();
  context->fFeatures = skia_features;
  context->fGetProc = std::move(getProc);
  context->fOwnsInstanceAndDevice = false;
  return true;
}

TestVulkanContext::~TestVulkanContext() {}

TestVulkanContext::TextureInfo TestVulkanContext::CreateVulkanTexture(
    const SkISize& size) {
  std::scoped_lock lock(textures_mutex);
  auto logical_device = vulkan_app_->AcquireFirstCompatibleLogicalDevice();

  VkImageCreateInfo new_image_create_info;
  new_image_create_info.sType =
      VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  new_image_create_info.imageType = VkImageType::VK_IMAGE_TYPE_2D;
  new_image_create_info.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
  new_image_create_info.extent.width = size.width();
  new_image_create_info.extent.height = size.height();
  new_image_create_info.extent.depth = 1;
  new_image_create_info.arrayLayers = 1;
  new_image_create_info.mipLevels = 1;
  new_image_create_info.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
  new_image_create_info.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
  new_image_create_info.usage =
      VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT |
      VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  new_image_create_info.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
  new_image_create_info.initialLayout =
      VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

  VkImage new_image;

  auto result = proc_table_->CreateImage(
      logical_device->GetHandle(), &new_image_create_info, nullptr, &new_image);

  VkMemoryRequirements mem_reqs;

  proc_table_->GetImageMemoryRequirements(logical_device->GetHandle(),
                                          new_image, &mem_reqs);
  VkMemoryAllocateInfo new_image_mem_alloc;
  new_image_mem_alloc.allocationSize = mem_reqs.size;
  new_image_mem_alloc.memoryTypeIndex =
      logical_device->GetPhysicalDeviceMemoryTypeIndex(
          mem_reqs.memoryTypeBits,
          VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkDeviceMemory new_image_device_memory;

  result = proc_table_->AllocateMemory(logical_device->GetHandle(),
                                       &new_image_mem_alloc, nullptr,
                                       &new_image_device_memory);

  proc_table_->BindImageMemory(logical_device->GetHandle(), new_image,
                               new_image_device_memory, 0);

  VkImageViewCreateInfo new_image_view_create_info;
  new_image_view_create_info.image = new_image;
  new_image_view_create_info.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
  new_image_view_create_info.format = new_image_create_info.format;
  new_image_view_create_info.subresourceRange = {
      VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  // VkImageView new_image_view;
  // result = proc_table_->CreateImageView(logical_device->GetHandle(),
  // &new_image_view_create_info, nullptr, &new_image_view);

  const int64_t texture_id = texture_id_ctr_++;
  textures_[texture_id] = new_image;

  return {
      .texture_id = texture_id,
      .texture = textures_[texture_id],
  };
}

bool TestVulkanContext::Present(int64_t texture_id) {
  std::scoped_lock lock(textures_mutex);
  if (textures_.find(texture_id) == textures_.end()) {
    return false;
  } else {
    return true;
  }
}

}  // namespace flutter
