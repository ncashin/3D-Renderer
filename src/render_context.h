#pragma once
#include <vector>
#include <tuple>

#include <stdexcept>
#include <algorithm>

#include "vulkan/vulkan.h"
#include "vkutil.h"

#include "window.h"

namespace ngfx{
static VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT        messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
uint32_t RatePhysicalDevice(VkPhysicalDevice vk_physical_device);

struct VulkanQueueIndices{
    bool graphics_queue_found;
    uint32_t graphics_family_index;
};
VulkanQueueIndices QueryQueueIndices(VkPhysicalDevice vk_physical_device);

struct DeviceQueue{
    uint32_t vk_family_index;
    VkQueue  vk_queue;
};
struct DeviceHeap{
    //DeviceHeap(VkDeviceSize minimum_allocation,
    //           VkDeviceSize maximum_allocated_size);
    
    VkDeviceSize allocated_size = 0;
    const VkDeviceSize minimum_allocation = 0;
    const VkDeviceSize maximum_allocated_size = 0;
};
struct ContextInfo{
    Window* window;
    bool enable_validation_layers;
    const char* applcation_name; 
    const char* engine_name;
};
namespace Context{
void Initalize(Window* window, const bool enable_validation_layers,
               const char* applcation_name, const char* engine_name);
void Terminate();

extern VkInstance vk_instance;
extern VkDebugUtilsMessengerEXT vk_debug_utils_messenger;

extern VkPhysicalDevice vk_physical_device;
extern VkDevice vk_device;
extern DeviceQueue graphics_queue;
extern DeviceQueue compute_queue;
extern DeviceQueue transfer_queue;
extern DeviceQueue present_queue;

extern std::vector<DeviceHeap> device_heaps;
}
}
