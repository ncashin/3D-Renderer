#include <vector>
#include <tuple>

#include <stdexcept>

#include <algorithm>

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

#include "vulkan/vulkan.hpp"

class Window{
public:
    Window(const char* name, int width, int height, int x = 0, int y = 0){
        window_ = SDL_CreateWindow(name, x, y, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    }
    ~Window(){
        SDL_DestroyWindow(window_);
    }
    
    void GetInstanceExtensions(unsigned int* pCount, const char** pNames){
        SDL_Vulkan_GetInstanceExtensions(window_, pCount, pNames);
    }
    VkSurfaceKHR CreateVulkanSurface(VkInstance vk_instance){
        VkSurfaceKHR vk_surface;
        SDL_Vulkan_CreateSurface(window_, vk_instance, &vk_surface);
        return vk_surface;
    }
    
private:
    SDL_Window* window_;
};

namespace vkutil{
std::vector<const char*> ValidateInstanceExtensionSupport(std::vector<const char*> extension_names){
    uint32_t extension_property_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_property_count, nullptr);
    VkExtensionProperties* extension_properties = new VkExtensionProperties[extension_property_count];
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_property_count, extension_properties);
    for(unsigned int i = 0; i < extension_property_count; i++){
        for(auto iterator = extension_names.begin(); iterator != extension_names.end(); iterator++){
            if(strcmp(extension_properties[i].extensionName, *iterator)){
                extension_names.erase(iterator);
                break;
            }
        }
    }
    delete[] extension_properties;
    return extension_names;
}
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
uint32_t RatePhysicalDevice(VkPhysicalDevice vk_physical_device){
    return 0;
}
}
static VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT        messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    printf("Vulkan Validation Layer -> %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

struct VulkanQueueIndices{
    bool graphics_queue_found;
    uint32_t graphics_family_index;
};
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

struct VulkanQueue{
    uint32_t vk_family_index;
    VkQueue  vk_queue;
};
struct DeviceHeap{
    DeviceHeap(VkDeviceSize minimum_allocation,
               VkDeviceSize maximum_allocated_size)
    : minimum_allocation(minimum_allocation), maximum_allocated_size(maximum_allocated_size) {};
    
    VkDeviceSize allocated_size;
    const VkDeviceSize minimum_allocation;
    const VkDeviceSize maximum_allocated_size;
};
class RenderContext{
public:
    RenderContext(Window* window, const bool enable_validation_layers,
                  const char* applcation_name, const char* engine_name){
        std::vector<const char*> extension_names;
        extension_names.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        
        unsigned int window_extension_count = 0;
        window->GetInstanceExtensions(&window_extension_count, nullptr);
        const char** window_extension_names = new const char*[window_extension_count];
        window->GetInstanceExtensions(&window_extension_count, window_extension_names);
        for(unsigned int i = 0; i < window_extension_count; i++){
            extension_names.emplace_back(window_extension_names[i]);
        }
        delete[] window_extension_names;
        if(enable_validation_layers){
            extension_names.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        
        std::vector<const char*> layer_names;
        if(enable_validation_layers){
            layer_names.emplace_back("VK_LAYER_KHRONOS_validation");
        }
        
        VkApplicationInfo application_info{};
        application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        application_info.pNext = nullptr;
        
        application_info.pApplicationName   = applcation_name;
        application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        application_info.pEngineName   = engine_name;
        application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        application_info.apiVersion = VK_API_VERSION_1_0;

        
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
        
        if(enable_validation_layers){
            VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};
            debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debug_messenger_create_info.pNext = nullptr;
            debug_messenger_create_info.flags = 0;
            
            debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            debug_messenger_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debug_messenger_create_info.pfnUserCallback = &DefaultDebugCallback;
            debug_messenger_create_info.pUserData = nullptr;

            vkutil::CreateDebugUtilsMessengerEXT(vk_instance, &debug_messenger_create_info, nullptr, &vk_debug_utils_messenger_);
        }
        
        
        std::vector<const char*> device_extension_names{};
        device_extension_names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        
        uint32_t physical_device_count;
        vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, nullptr);
        VkPhysicalDevice* physical_devices = new VkPhysicalDevice[physical_device_count];
        vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices);
        std::vector<std::tuple<uint32_t, VkPhysicalDevice>> rated_physical_devices;
        for(unsigned int i = 0; i < physical_device_count; i++){
            rated_physical_devices.emplace_back(std::make_tuple(vkutil::RatePhysicalDevice(physical_devices[i]),
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
        
        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &memory_properties);
        for(uint8_t i = 0; i < memory_properties.memoryHeapCount; i++){
            device_heaps.emplace_back(DeviceHeap{
                static_cast<VkDeviceSize>(memory_properties.memoryHeaps[i].size * 0.16),
                static_cast<VkDeviceSize>(memory_properties.memoryHeaps[i].size * 0.80),
            });
        }
    }
    ~RenderContext(){
        vkDestroyDevice(vk_device, nullptr);
        
        vkutil::DestroyDebugUtilsMessengerEXT(vk_instance, nullptr, nullptr);
        vkDestroyInstance(vk_instance, nullptr);
    }
    
    VkInstance vk_instance;
    VkDebugUtilsMessengerEXT vk_debug_utils_messenger_;
    
    VkPhysicalDevice vk_physical_device;
    VkDevice vk_device;
    VulkanQueue graphics_queue;
    VulkanQueue compute_queue;
    VulkanQueue transfer_queue;
    VulkanQueue present_queue;
    
    std::vector<DeviceHeap> device_heaps;
};
RenderContext* render_context = nullptr;

struct Swapchain{
    Swapchain(Window* window){
        vk_surface_ = window->CreateVulkanSurface(render_context->vk_instance);
        
        uint32_t available_format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(render_context->vk_physical_device, vk_surface_,
                                             &available_format_count, nullptr);
        VkSurfaceFormatKHR* available_surface_formats = new VkSurfaceFormatKHR[available_format_count];
        vkGetPhysicalDeviceSurfaceFormatsKHR(render_context->vk_physical_device, vk_surface_,
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
        vkGetPhysicalDeviceSurfacePresentModesKHR(render_context->vk_physical_device, vk_surface_,
                                                  &available_present_mode_count, nullptr);
        VkPresentModeKHR* available_present_modes = new VkPresentModeKHR[available_present_mode_count];
        vkGetPhysicalDeviceSurfacePresentModesKHR(render_context->vk_physical_device, vk_surface_,
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
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(render_context->vk_physical_device, vk_surface_,
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
        create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        
        if (render_context->graphics_queue.vk_family_index != render_context->present_queue.vk_family_index){
            uint32_t family_indices[] = {
                render_context->graphics_queue.vk_family_index,
                render_context->present_queue.vk_family_index
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
        
        VkResult vk_result = vkCreateSwapchainKHR(render_context->vk_device, &create_info, nullptr, &vk_swapchain_);
        
        vkGetSwapchainImagesKHR(render_context->vk_device, vk_swapchain_, &image_count, nullptr);
        images_.resize(image_count);
        vkGetSwapchainImagesKHR(render_context->vk_device, vk_swapchain_, &image_count, images_.data());
        
        surface_format_ = chosen_surface_format;
    }
    ~Swapchain(){
        for(VkImageView image_view : image_views){
            vkDestroyImageView(render_context->vk_device, image_view, nullptr);
        }
        vkDestroySwapchainKHR(render_context->vk_device, vk_swapchain_, nullptr);
        vkDestroySurfaceKHR(render_context->vk_instance, vk_surface_, nullptr);
    }
    
    void CreateImageViews(){
        VkImageViewCreateInfo image_view_create_info{};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
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
            vkCreateImageView(render_context->vk_device, &image_view_create_info, nullptr, &image_views[i]);
        }
    }
    VkImageView* GetImageViews(){
        if(image_views.size() == 0){
            CreateImageViews();
        }
        return image_views.data();
    }
    
    VkSurfaceKHR vk_surface_;
    VkSurfaceFormatKHR surface_format_;
    VkExtent2D extent_;
    VkSwapchainKHR vk_swapchain_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views;
};
Swapchain* swapchain;

struct Region{
    size_t offset;
    size_t size;
};
class RegionList{
public:
    RegionList(){};
    RegionList(size_t offset, size_t size) : list_({{offset, size}}) {}
    bool GetRegion(size_t size, size_t alignment, Region* acquired_region){
        size_t padding = 0;
        for(Region& memory : list_){
            if(alignment != 0 && memory.offset % alignment){
                padding = alignment - (memory.offset % alignment);
            }
            if(memory.size < size + padding){
                continue;
            }
            *acquired_region = Region{ memory.offset, size + padding };
            memory.size   -= size + padding;
            memory.offset += size + padding;
            return true;
        }
        return false;
    }
    void FreeRegion(Region free_memory){
        size_t index = list_.size() / 2;
        while(index / 2 != 0){
            if     (free_memory.offset < list_[index].offset){ index -= index / 2;}
            else if(free_memory.offset > list_[index].offset){ index += index / 2; }
        }
        if(free_memory.offset > list_[index].offset){ index++; }
        
        if(free_memory.offset + free_memory.size == list_[index].offset){
            list_[index].offset = free_memory.offset;
            list_[index].size += free_memory.size;

            if(index - 1 != UINT32_MAX && list_[index - 1].offset + list_[index - 1].size == free_memory.offset){
                list_[index - 1].size += list_[index].size;
                list_.erase(list_.begin() + index);
            } return;
        }
        else if(list_[index].offset + list_[index].size == free_memory.offset){
            list_[index].size += free_memory.size; return;
        }
        list_.insert(list_.begin() + index, free_memory);
    }
private:
    std::vector<Region> list_;
};

enum class MemoryType{
    DeviceLocal,
    Coherent,
};
struct DeviceMemory{
    DeviceMemory(){};
    RegionList region_list;
    uint8_t vk_memory_type_index;
    VkDeviceMemory vk_memory;
    char* mapped_pointer;
};
struct MemoryAllocation{
    uint8_t resource_index;
    size_t offset;
    size_t size;
};
class Allocator{
public:
    const MemoryAllocation Malloc(MemoryType desired_memory_type,
                                  size_t size, size_t alignment, uint32_t memory_type_bits){
        VkMemoryPropertyFlags required_properties{};
        switch(desired_memory_type){
            case MemoryType::DeviceLocal: required_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;  break;
            case MemoryType::Coherent:    required_properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; break;
        }
        
        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(render_context->vk_physical_device, &memory_properties);
        for(uint8_t i = 0; i < device_allocations_.size(); i++){
            DeviceMemory& memory = device_allocations_[i];
            if(!(memory_type_bits & (1 << memory.vk_memory_type_index)) ||
                (memory_properties.memoryTypes[memory.vk_memory_type_index].propertyFlags
                & required_properties) != required_properties){
                continue;
            }
            Region memory_region{};
            if(memory.region_list.GetRegion(size, alignment, &memory_region)){
                return MemoryAllocation{ i, memory_region.offset, memory_region.size };
            }
        }

        for(uint8_t i = 0; i < memory_properties.memoryTypeCount; i++){
            if(!(memory_type_bits & (1 << i)) ||
                (memory_properties.memoryTypes[i].propertyFlags & required_properties) != required_properties){
                continue;
            }
            uint8_t heap_index = memory_properties.memoryTypes[i].heapIndex;
            DeviceHeap& heap = render_context->device_heaps[heap_index];
            if(heap.allocated_size + size > heap.maximum_allocated_size){
                return;
            }
            VkDeviceSize new_allocation_size = size;
            if(new_allocation_size < heap.minimum_allocation &&
               heap.allocated_size + heap.minimum_allocation < heap.maximum_allocated_size){
                new_allocation_size = heap.minimum_allocation;
            }
            
            VkMemoryAllocateInfo allocate_info{};
            allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocate_info.pNext = nullptr;
            allocate_info.allocationSize = new_allocation_size;
            allocate_info.memoryTypeIndex = i;
            
            uint16_t index = device_allocations_.size();
            DeviceMemory new_memory{};
            new_memory.region_list = { 0, new_allocation_size };
            new_memory.vk_memory_type_index = i;
            vkAllocateMemory(render_context->vk_device, &allocate_info, nullptr, &new_memory.vk_memory);
            if(desired_memory_type == MemoryType::Coherent){
                vkMapMemory(render_context->vk_device,
                            new_memory.vk_memory, 0, VK_WHOLE_SIZE, 0, (void**)&new_memory.mapped_pointer);
            }
            uint8_t new_memory_index = (uint8_t)device_allocations_.size();
            device_allocations_.emplace_back(new_memory);
            return MemoryAllocation{ new_memory_index, 0, size };
        }
        return MemoryAllocation{};
    }
    void Free(const MemoryAllocation allocation){
        device_allocations_[allocation.resource_index].region_list.FreeRegion({
            allocation.offset, allocation.size,
        });
    }
    
    std::vector<DeviceMemory> device_allocations_;
};
Allocator* allocator = nullptr;

class RenderBuffer{
public:
    RenderBuffer(Swapchain* swapchain) : swapchain_attachment(swapchain) {
        VkAttachmentDescription swapchain_attachment_description{};
        swapchain_attachment_description.flags = 0;
        swapchain_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        swapchain_attachment_description.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        swapchain_attachment_description.format = swapchain->surface_format_.format;
        swapchain_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        swapchain_attachment_description.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        swapchain_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        swapchain_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        swapchain_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
        VkAttachmentReference swapchain_attachment_reference{};
        swapchain_attachment_reference.attachment = 0;
        swapchain_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription forward_subpass_description{};
        forward_subpass_description.flags = 0;
        forward_subpass_description.colorAttachmentCount = 1;
        forward_subpass_description.pColorAttachments = &swapchain_attachment_reference;
        forward_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        
        VkSubpassDependency swapchain_present_dependency{};
        swapchain_present_dependency.dependencyFlags = 0;
        swapchain_present_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        swapchain_present_dependency.dstSubpass = 0;
        
        swapchain_present_dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        swapchain_present_dependency.srcAccessMask = 0;
        
        swapchain_present_dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        swapchain_present_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        
        VkRenderPassCreateInfo render_pass_create_info{};
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_create_info.pNext = nullptr;
        render_pass_create_info.flags = 0;
        render_pass_create_info.attachmentCount = 1;
        render_pass_create_info.pAttachments    = &swapchain_attachment_description;
        render_pass_create_info.subpassCount = 1;
        render_pass_create_info.pSubpasses   = &forward_subpass_description;
        render_pass_create_info.dependencyCount = 1;
        render_pass_create_info.pDependencies   = &swapchain_present_dependency;
        
        vkCreateRenderPass(render_context->vk_device, &render_pass_create_info, nullptr, &vk_render_pass);
        
        std::vector<VkImageView> attachment_views{};
        attachment_views.emplace_back(VkImageView{});
        
        VkFramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = nullptr;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.width  = swapchain->extent_.width;
        framebuffer_create_info.height = swapchain->extent_.height;
        framebuffer_create_info.layers = 1;
        framebuffer_create_info.renderPass = vk_render_pass;
        framebuffer_create_info.pAttachments = attachment_views.data();

        vk_framebuffers.resize(swapchain->images_.size());
        uint32_t swapchain_index = (uint32_t)attachment_views.size() - 1;
        VkImageView* swapchain_image_views = swapchain->GetImageViews();
        for(int i = 0; i < swapchain->images_.size(); i++){
            attachment_views[swapchain_index] = swapchain_image_views[i];
            vkCreateFramebuffer(render_context->vk_device, &framebuffer_create_info, nullptr, &vk_framebuffers[i]);
        }
    }
    ~RenderBuffer(){
        for(VkFramebuffer framebuffer : vk_framebuffers){
            vkDestroyFramebuffer(render_context->vk_device, framebuffer, nullptr);
        }
        vkDestroyRenderPass(render_context->vk_device, vk_render_pass, nullptr);
    }
    
    VkRenderPass vk_render_pass;
    std::vector<VkFramebuffer> vk_framebuffers;
    Swapchain* swapchain_attachment;
};



int main(int argc, char** argv){
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    auto window = new Window("engine", display_mode.w, display_mode.h);
    render_context = new RenderContext(window, true, "engine", "engine");
    allocator      = new Allocator();
    auto swapchain = new Swapchain(window);
    auto render_buffer = new RenderBuffer(swapchain);
    
    bool running = true;
    while(running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_EventType::SDL_QUIT:
                    running = false;
                    break;
            }
        }
    }
    
    vkDeviceWaitIdle(render_context->vk_device);
    delete render_buffer;
    delete swapchain;
    delete allocator;
    delete render_context;
    delete window;
}
