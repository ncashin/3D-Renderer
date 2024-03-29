#include "swapchain.h"

namespace render{
Swapchain::Swapchain(Window* window){
    vk_surface_ = window->CreateVulkanSurface(render::context.vk_instance);
    
    uint32_t available_format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(render::context.vk_physical_device, vk_surface_,
                                         &available_format_count, nullptr);
    VkSurfaceFormatKHR* available_surface_formats = new VkSurfaceFormatKHR[available_format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(render::context.vk_physical_device, vk_surface_,
                                         &available_format_count, available_surface_formats);
    VkSurfaceFormatKHR chosen_surface_format = available_surface_formats[0];
    for (int i = 0; i < available_format_count; i++) {
        if (available_surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen_surface_format = available_surface_formats[i];
            break;
        }
    }
    delete[] available_surface_formats;
    
    uint32_t available_present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(render::context.vk_physical_device, vk_surface_,
                                              &available_present_mode_count, nullptr);
    VkPresentModeKHR* available_present_modes = new VkPresentModeKHR[available_present_mode_count];
    vkGetPhysicalDeviceSurfacePresentModesKHR(render::context.vk_physical_device, vk_surface_,
                                              &available_present_mode_count, available_present_modes);
    
    VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (int i = 0; i < available_present_mode_count; i++) {
        if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosen_present_mode = available_present_modes[i];
            break;
        }
    }
    delete[] available_present_modes;
    
    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(render::context.vk_physical_device, vk_surface_,
                                              &surface_capabilities);
    extent_ = surface_capabilities.currentExtent;
    
    uint32_t image_count = surface_capabilities.minImageCount + 1;;
    if (surface_capabilities.maxImageCount > 0 &&
        image_count > surface_capabilities.maxImageCount){
        image_count = surface_capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    
    create_info.surface = vk_surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = chosen_surface_format.format;
    create_info.imageColorSpace = chosen_surface_format.colorSpace;
    create_info.imageExtent = extent_;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    if (render::context.graphics_queue.vk_family_index != render::context.present_queue.vk_family_index){
        uint32_t family_indices[] = {
            render::context.graphics_queue.vk_family_index,
            render::context.present_queue.vk_family_index
        };
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }
    create_info.preTransform = surface_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = chosen_present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    
    VkResult vk_result = vkCreateSwapchainKHR(render::context.vk_device, &create_info, nullptr, &vk_swapchain_);
    
    vkGetSwapchainImagesKHR(render::context.vk_device, vk_swapchain_, &image_count, nullptr);
    images_.resize(image_count);
    vkGetSwapchainImagesKHR(render::context.vk_device, vk_swapchain_, &image_count, images_.data());
    
    surface_format_ = chosen_surface_format;
}
Swapchain::~Swapchain(){
    for(VkImageView image_view : image_views){
        vkDestroyImageView(render::context.vk_device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(render::context.vk_device, vk_swapchain_, nullptr);
    vkDestroySurfaceKHR(render::context.vk_instance, vk_surface_, nullptr);
}

uint32_t Swapchain::AcquireImage(VkSemaphore semaphore, VkFence fence){
    usage_mutex.lock();
    uint32_t image_index;
    vkAcquireNextImageKHR(render::context.vk_device, vk_swapchain_, UINT64_MAX,
                          semaphore, fence, &image_index);
    usage_mutex.unlock();
    return   image_index;
}

void Swapchain::CreateImageViews(){
    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format   = surface_format_.format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount   = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;
        
    image_views.resize(images_.size());
    for(int i = 0; i < image_views.size(); i++){
        image_view_create_info.image = images_[i];
        vkCreateImageView(render::context.vk_device, &image_view_create_info, nullptr, &image_views[i]);
    }
}
VkImageView* Swapchain::GetImageViews(){
    if(image_views.size() == 0){
        CreateImageViews();
    }
    return image_views.data();
}
}

