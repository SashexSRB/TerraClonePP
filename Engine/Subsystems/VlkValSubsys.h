#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VlkRenderer;

/**
 * Vulkan validation and debug utilities wrapper.
 *
 * Responsible for enabling validation layers and managing
 * the Vulkan debug messenger used for runtime error reporting.
 */
class VlkValSubsys {
public:
    VlkRenderer* r = nullptr;

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkDebugUtilsMessengerEXT debugMessenger;

    /**
     * Checks whether required Vulkan validation layers are available.
     */
    bool checkValidationLayerSupport();

    /**
     * Vulkan debug callback used by validation layers.
     *
     * This function is called internally by the Vulkan runtime.
     */
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData
    );

    /**
     * Creates and registers the Vulkan debug messenger.
     *
     * @param instance Vulkan instance to attach debugger to
     */
    void setupDebugMessenger(VkInstance instance);

    /**
     * Fills Vulkan debug messenger creation structure with default settings.
     *
     * @param createInfo Output structure to populate
     */
    void populateDebugMesengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT &createInfo
    );

    /**
     * Creates Vulkan debug messenger (extension wrapper).
     */
    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger
    );

    /**
     * Destroys Vulkan debug messenger.
     */
    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks *pAllocator
    );
};

/**
 * Global validator instance used by the application.
 */
//extern VlkValSubsys validator;
