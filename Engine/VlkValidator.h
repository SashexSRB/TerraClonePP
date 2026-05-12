#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VlkValidator {
public:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkDebugUtilsMessengerEXT debugMessenger;

    bool checkValidationLayerSupport();

    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                  void *pUserData);

    void setupDebugMessenger(VkInstance instance);

    void populateDebugMesengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT &createInfo);

    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks *pAllocator);
};

extern VlkValidator validator;
