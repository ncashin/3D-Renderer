#include "vkutil.h"

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
}
