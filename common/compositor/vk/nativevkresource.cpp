// Copyright (c) 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "nativevkresource.h"

#include "hwctrace.h"
#include "overlaybuffer.h"
#include "overlaylayer.h"

namespace hwcomposer {

bool NativeVKResource::PrepareResources(
    const std::vector<OverlayLayer>& layers) {
  VkResult res;

  Reset();

  VkImageSubresourceRange clear_range = {};
  clear_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  clear_range.levelCount = 1;
  clear_range.layerCount = 1;

  for (auto& layer : layers) {
    VkImage image = layer.GetBuffer()->ImportImage(dev_);
    if (image == VK_NULL_HANDLE) {
      ETRACE("Failed to make import image\n");
      return false;
    }
    src_images_.emplace_back(image);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = clear_range;
    src_barrier_before_clear_.emplace_back(barrier);

    VkImageViewCreateInfo view_create = {};
    view_create.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create.image = image;
    view_create.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create.format = GbmToVkFormat(layer.GetBuffer()->GetFormat());
    view_create.components = {};
    view_create.components.r = VK_COMPONENT_SWIZZLE_R;
    view_create.components.g = VK_COMPONENT_SWIZZLE_G;
    view_create.components.b = VK_COMPONENT_SWIZZLE_B;
    view_create.components.a = VK_COMPONENT_SWIZZLE_A;
    view_create.subresourceRange = {};
    view_create.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create.subresourceRange.levelCount = 1;
    view_create.subresourceRange.layerCount = 1;

    VkImageView image_view;
    res = vkCreateImageView(dev_, &view_create, NULL, &image_view);
    if (res != VK_SUCCESS) {
      ETRACE("vkCreateImageView failed (%d)\n", res);
      return false;
    }
    src_image_views_.emplace_back(image_view);

    struct vk_resource resource;
    resource.image = image;
    resource.image_view = image_view;
    layer_textures_.emplace_back(resource);
  }
  return true;
}

NativeVKResource::~NativeVKResource() {
  Reset();
}

void NativeVKResource::Reset() {
  layer_textures_.clear();
  src_images_.clear();
  src_image_views_.clear();
  src_barrier_before_clear_.clear();
}

GpuResourceHandle NativeVKResource::GetResourceHandle(
    uint32_t layer_index) const {
  if (layer_textures_.size() < layer_index) {
    struct vk_resource res = {};
    return res;
  }

  return layer_textures_.at(layer_index);
}

}  // namespace hwcomposer
