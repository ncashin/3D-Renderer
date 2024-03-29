#include "render/context.h"

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include "vk_mem_alloc.h"

namespace render{
static VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                           VkDebugUtilsMessageTypeFlagsEXT        messageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                           void* pUserData) {
    printf("Vulkan Validation Layer -> %s\n", pCallbackData->pMessage);
    
    return VK_FALSE;
}
uint32_t RatePhysicalDevice(VkPhysicalDevice vk_physical_device){
    return 0;
}
VulkanQueueIndices QueryQueueIndices(VkPhysicalDevice vk_physical_device){
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, nullptr);
    VkQueueFamilyProperties* queue_family_properties = new VkQueueFamilyProperties[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, queue_family_properties);
    
    VulkanQueueIndices queue_indices{};
    for(int i = 0; i < queue_family_count; i++){
        if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            queue_indices.graphics_queue_found = true;
            queue_indices.graphics_family_index = i;
        }
    }
    delete[] queue_family_properties;
    return queue_indices;
}


void Context::Initalize(ContextInfo info){
    std::vector<const char*> extension_names;
    extension_names.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extension_names.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    
    unsigned int window_extension_count = 0;
    info.window->GetInstanceExtensions(&window_extension_count, nullptr);
    const char** window_extension_names = new const char*[window_extension_count];
    info.window->GetInstanceExtensions(&window_extension_count, window_extension_names);
    for(unsigned int i = 0; i < window_extension_count; i++){
        extension_names.emplace_back(window_extension_names[i]);
    }
    delete[] window_extension_names;
    if(info.enable_validation_layers){
        extension_names.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    std::vector<const char*> layer_names;
    if(info.enable_validation_layers){
        layer_names.emplace_back("VK_LAYER_KHRONOS_validation");
    }
    
    VkApplicationInfo application_info{};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = nullptr;
    
    application_info.pApplicationName   = info.applcation_name;
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.pEngineName   = info.engine_name;
    application_info.engineVersion = VK_MAKE_VERSION(1, 2, 0);
    application_info.apiVersion = VK_API_VERSION_1_2;
    
    
    auto unsupported_extensions = vkutil::ValidateInstanceExtensionSupport(extension_names);
    if(unsupported_extensions.size() != 0){
        for(const char* name : unsupported_extensions){
            printf("INSTANCE EXTENSION: %s | NOT SUPPORTED!\n", name);
        }
    }
    
    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = nullptr;
    instance_create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    
    instance_create_info.pApplicationInfo = &application_info;
    
    instance_create_info.enabledExtensionCount   = (uint32_t)extension_names.size();
    instance_create_info.ppEnabledExtensionNames = extension_names.data();
    instance_create_info.enabledLayerCount   = (uint32_t)layer_names.size();
    instance_create_info.ppEnabledLayerNames = layer_names.data();
    
    VkResult result = vkCreateInstance(&instance_create_info, nullptr, &vk_instance);
    if(result != VK_SUCCESS){
        throw std::runtime_error("FAILED TO CREATE VULKAN INSTANCE!");
    }
    
    if(info.enable_validation_layers){
        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};
        debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_messenger_create_info.pNext = nullptr;
        debug_messenger_create_info.flags = 0;
        
        debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_messenger_create_info.pfnUserCallback = &DefaultDebugCallback;
        debug_messenger_create_info.pUserData = nullptr;
        
        vkutil::CreateDebugUtilsMessengerEXT(vk_instance, &debug_messenger_create_info, nullptr, &vk_debug_utils_messenger);
    }
    
    
    std::vector<const char*> device_extension_names{};
    device_extension_names.emplace_back("VK_KHR_portability_subset");
    device_extension_names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    
    uint32_t physical_device_count;
    vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, nullptr);
    VkPhysicalDevice* physical_devices = new VkPhysicalDevice[physical_device_count];
    vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices);
    std::vector<std::tuple<uint32_t, VkPhysicalDevice>> rated_physical_devices;
    for(unsigned int i = 0; i < physical_device_count; i++){
        rated_physical_devices.emplace_back(std::make_tuple(RatePhysicalDevice(physical_devices[i]),
                                                            physical_devices[i]));
    }
    delete[] physical_devices;
    // Sort By First Element of Tuple / Device Rating
    std::sort(rated_physical_devices.begin(), rated_physical_devices.end());
    
    VulkanQueueIndices queue_indices{};
    for(std::tuple<uint32_t, VkPhysicalDevice> tuple : rated_physical_devices){
        vk_physical_device = std::get<1>(tuple);
        
        queue_indices = QueryQueueIndices(vk_physical_device);
        float priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> device_queue_create_info{};
        if(queue_indices.graphics_queue_found){
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.pNext = nullptr;
            queue_create_info.flags = 0;
            
            queue_create_info.queueCount = 1;
            queue_create_info.queueFamilyIndex  = queue_indices.graphics_family_index;
            queue_create_info.pQueuePriorities = &priority;
            
            device_queue_create_info.emplace_back(queue_create_info);
        }
        
        VkPhysicalDeviceFeatures device_features{};
        
        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pNext = nullptr;
        device_create_info.flags = 0;
        
        device_create_info.queueCreateInfoCount = (uint32_t)device_queue_create_info.size();
        device_create_info.pQueueCreateInfos    = device_queue_create_info.data();
        
        device_create_info.enabledExtensionCount   = (uint32_t)device_extension_names.size();
        device_create_info.ppEnabledExtensionNames = device_extension_names.data();
        device_create_info.enabledLayerCount   = (uint32_t)layer_names.size();
        device_create_info.ppEnabledLayerNames = layer_names.data();
        device_create_info.pEnabledFeatures = &device_features;
        
        result = vkCreateDevice(vk_physical_device, &device_create_info, nullptr, &vk_device);
        if(result == VK_SUCCESS){
            break;
        }
    }
    if(vk_device == VK_NULL_HANDLE){
        
    }
    graphics_queue.vk_family_index = queue_indices.graphics_family_index;
    vkGetDeviceQueue(vk_device, graphics_queue.vk_family_index, 0, &graphics_queue.vk_queue);
    
    
    VmaVulkanFunctions vma_vulkan_functions = {};
    vma_vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vma_vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
    
    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_create_info.physicalDevice = vk_physical_device;
    allocator_create_info.device = vk_device;
    allocator_create_info.instance = vk_instance;
    allocator_create_info.pVulkanFunctions = &vma_vulkan_functions;
    vmaCreateAllocator(&allocator_create_info, &allocator);
}
void Context::Terminate(){
    vmaDestroyAllocator(allocator);
    
    vkDestroyDevice(vk_device, nullptr);
    
    vkutil::DestroyDebugUtilsMessengerEXT(vk_instance, vk_debug_utils_messenger, nullptr);
    vkDestroyInstance(vk_instance, nullptr);
}
Context context;
}
