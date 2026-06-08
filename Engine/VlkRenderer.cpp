#include "VlkRenderer.h"
#include "VlkValidator.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "Lib/stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Lib/stb_image.h"
#include "Game/Game.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <GLFW/glfw3.h>

// =========================================================================
// Initialization / Setup
// =========================================================================

void VlkRenderer::createInstance() {
    if (validator.enableValidationLayers && !validator.checkValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but not available");

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanApp";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName = "VlkEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (validator.enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validator.validationLayers.size());
        createInfo.ppEnabledLayerNames = validator.validationLayers.data();

        validator.populateDebugMesengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance!");

    std::cout << "[Renderer] Instance created.\n";
}

void VlkRenderer::createSurface(GLFWwindow *window) {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

void VlkRenderer::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto &device: devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) throw std::runtime_error("Failed to find a suitable GPU");
    std::cout << "[Renderer] Physical Device Selected.\n";
}

void VlkRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily: uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (validator.enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validator.validationLayers.size());
        createInfo.ppEnabledLayerNames = validator.validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

    std::cout << "[Renderer] Logical device created.\n";
}

void VlkRenderer::createVmaAllocator() {
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;

    if (vmaCreateAllocator(&allocatorInfo, &vmaAllocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator!");
    }

    std::cout << "[Renderer] VMA Allocator created!\n";
}

void VlkRenderer::createStagingBuffer() {
    stagingBufferSize = STAGING_BUFFER_SIZE;

    createBuffer(
        stagingBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        stagingBuffer, stagingAllocation
    );

    vmaMapMemory(vmaAllocator, stagingAllocation, &stagingMapped);
    std::cout << "[Renderer] Persistent staging buffer created (" << (stagingBufferSize / 1024 / 1024) << "MB)\n";
}

void VlkRenderer::createSwapChain(GLFWwindow *window) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swapchain!");

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    std::cout << "[Renderer] Swapchain created!\n";
}

void VlkRenderer::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView( swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    std::cout << "[Renderer] Image views created!\n";
}

void VlkRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {
        colorAttachment,
        depthAttachment
    };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass!");

    std::cout << "[Renderer] Render Pass created!\n";
}

void VlkRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding fontSamplerBinding{};
    fontSamplerBinding.binding = 2;
    fontSamplerBinding.descriptorCount = 1;
    fontSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    fontSamplerBinding.pImmutableSamplers = nullptr;
    fontSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding skyLayoutBinding{};
    skyLayoutBinding.binding = 3;
    skyLayoutBinding.descriptorCount = 1;
    skyLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyLayoutBinding.pImmutableSamplers = nullptr;
    skyLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding lightmapLayoutBinding{};
    lightmapLayoutBinding.binding = 4;
    lightmapLayoutBinding.descriptorCount = 1;
    lightmapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    lightmapLayoutBinding.pImmutableSamplers = nullptr;
    lightmapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding spriteLayoutBinding{};
    spriteLayoutBinding.binding = 5;
    spriteLayoutBinding.descriptorCount = 1;
    spriteLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    spriteLayoutBinding.pImmutableSamplers = nullptr;
    spriteLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 6> bindings = {
        uboLayoutBinding,
        samplerLayoutBinding,
        fontSamplerBinding,
        skyLayoutBinding,
        lightmapLayoutBinding,
        spriteLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");

    std::cout << "[Renderer] Descriptor Set Layout created!\n";
}

void VlkRenderer::createGraphicsPipeline() {
    auto vertShaderCode = readFile(ASSET_PATH "Shaders/vert.spv");
    auto fragShaderCode = readFile(ASSET_PATH "Shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable = VK_TRUE;

    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(UIPushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline!");

    std::cout << "[Renderer] Graphics Pipeline created!\n";

    // ── UI pipeline (depth test OFF) ─────────────────────────────────────────
    VkPipelineDepthStencilStateCreateInfo uiDepthStencil{};
    uiDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    uiDepthStencil.depthTestEnable = VK_FALSE;
    uiDepthStencil.depthWriteEnable = VK_FALSE;
    uiDepthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    uiDepthStencil.depthBoundsTestEnable = VK_FALSE;
    uiDepthStencil.stencilTestEnable = VK_FALSE;

    pipelineInfo.pDepthStencilState = &uiDepthStencil;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &uiPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create UI pipeline!");

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    std::cout << "[Renderer] UI Graphics Pipeline created!\n";
}

void VlkRenderer::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {

        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer!");
    }

    std::cout << "[Renderer] Framebuffers created!\n";
}

void VlkRenderer::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool.");

    std::cout << "[Renderer] Command Pool created!\n";
}

void VlkRenderer::createTransferResources() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &transferCommandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create transfer pool.");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = transferCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &transferCommandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate transfer command buffers.");

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(device, &fenceInfo, nullptr, &transferFence) != VK_SUCCESS)
        throw std::runtime_error("Failed to create transfer fence!");

    std::cout << "[Renderer] Transfer resources created!\n";
}

void VlkRenderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(
        swapChainExtent.width, swapChainExtent.height, depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, depthImage, depthImageAllocation
    );

    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    std::cout << "[Renderer] Depth resources created!\n";
}

void VlkRenderer::createTextureImage(const std::string& path) {
    TextureInfo texInfo = loadTextureImage(path);

    VkDeviceSize imageSize = texInfo.width * texInfo.height * 4;

    createImage(texInfo.width, texInfo.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, textureImage, textureImageAllocation);

    uploadTexture(
        texInfo.pixels, imageSize,
        textureImage, VK_FORMAT_R8G8B8A8_SRGB,
        texInfo.width, texInfo.height
    );

    std::cout << "[Renderer] Main texture atlas created!\n";
}

void VlkRenderer::createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VlkRenderer::createSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    auto info = defaultSamplerInfo();

    info.magFilter = VK_FILTER_NEAREST;
    info.minFilter = VK_FILTER_NEAREST;
    info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    textureSampler = makeSampler(info);
}

void VlkRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformAllocations.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            uniformBuffers[i],
            uniformAllocations[i]
        );

        vmaMapMemory(vmaAllocator, uniformAllocations[i], &uniformBuffersMapped[i]);
    }

    std::cout << "[Renderer] Uniform Buffers created!\n";
}

void VlkRenderer::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSize{};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 5);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
    poolInfo.pPoolSizes = poolSize.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");

    std::cout << "[Renderer] Descriptor pool created!\n";
}

void VlkRenderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        VkDescriptorImageInfo fontImageInfo{};
        fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        fontImageInfo.imageView   = fontImageView;
        fontImageInfo.sampler     = fontSampler;

        VkDescriptorImageInfo skyImageInfo{};
        skyImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        skyImageInfo.imageView   = skyImageView;
        skyImageInfo.sampler     = skySampler;

        VkDescriptorImageInfo lightmapImageInfo{};
        lightmapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        lightmapImageInfo.imageView   = lightmapImageView;
        lightmapImageInfo.sampler     = lightmapSampler;

        VkDescriptorImageInfo spriteImageInfo{};
        spriteImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        spriteImageInfo.imageView   = spriteImageView;
        spriteImageInfo.sampler     = spriteSampler;

        auto makeWrite = [&](uint32_t binding, VkDescriptorType type) {
            VkWriteDescriptorSet w{};
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = descriptorSets[i];
            w.dstBinding = binding;
            w.dstArrayElement = 0;
            w.descriptorType = type;
            w.descriptorCount = 1;
            return w;
        };

        auto w0 = makeWrite(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); w0.pBufferInfo = &bufferInfo;
        auto w1 = makeWrite(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); w1.pImageInfo = &imageInfo;
        auto w2 = makeWrite(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); w2.pImageInfo = &fontImageInfo;
        auto w3 = makeWrite(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); w3.pImageInfo = &skyImageInfo;
        auto w4 = makeWrite(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); w4.pImageInfo = &lightmapImageInfo;
        auto w5 = makeWrite(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); w5.pImageInfo = &spriteImageInfo;

        std::array<VkWriteDescriptorSet, 6> descriptorWrites = {w0, w1, w2, w3, w4, w5};

        vkUpdateDescriptorSets(
            device,
            descriptorWrites.size(),
            descriptorWrites.data(),
            0, nullptr
        );
    }

    std::cout << "[Renderer] Descriptor sets created!\n";
}

void VlkRenderer::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffer!");

    std::cout << "[Renderer] Command Buffers created!\n";
}

void VlkRenderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphores!");
            }
    }

    std::cout << "[Renderer] Sync Objects created!\n";
}

// =========================================================================
// Frame Rendering
// =========================================================================

void VlkRenderer::drawFrame(GLFWwindow *window, bool &framebufferResized, const CameraParams &cam) {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device, swapChain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE, &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain(window);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    updateUniformBuffer(currentFrame, cam);

    lastCam = cam;

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);

    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer!");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        framebufferResized) {
        framebufferResized = false;
        recreateSwapChain(window);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VlkRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer!");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}}; // black — sky covers this
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    // ── Sky (UI pipeline, depth OFF, drawn first) ─────────────────────────
    bindPipeline(commandBuffer, uiPipeline, viewport, scissor);
    drawSky(commandBuffer);

    // ── World & Player pipeline (depth ON) ─────────────────────────────────────────
    bindPipeline(commandBuffer, graphicsPipeline, viewport, scissor);
    {
        UIPushConstants worldPush{};
        worldPush.useLighting    = 1;
        worldPush.lightmapOrigin = lastLightmapOrigin;
        worldPush.lightmapSize   = lastLightmapSize;

        vkCmdPushConstants(
            commandBuffer, pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(UIPushConstants), &worldPush
        );

        for (const auto& key : chunkKeys) draw(key, commandBuffer);
    }

    {
        UIPushConstants playerPush{};
        playerPush.useSprite     = 1;
        playerPush.useLighting   = 1;
        playerPush.lightmapOrigin = lastLightmapOrigin;
        playerPush.lightmapSize   = lastLightmapSize;

        vkCmdPushConstants(commandBuffer, pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(UIPushConstants), &playerPush);

        draw("player", commandBuffer);
    }

    // ── UI pipeline (depth OFF) ───────────────────────────────────────────
    bindPipeline(commandBuffer, uiPipeline, viewport, scissor);
    drawUI("inventory", commandBuffer);
    drawSprite("inventory_sprites", commandBuffer);
    drawText(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void VlkRenderer::updateUniformBuffer(uint32_t currentImage, const CameraParams &cam) {
    UniformBufferObject ubo{};

    ubo.proj = glm::mat4(1.0f);
    ubo.proj[0][0] = 2.0f / cam.visibleWidth;
    ubo.proj[1][1] = 2.0f / cam.visibleHeight;
    ubo.proj[3][0] = -1.0f;
    ubo.proj[3][1] = -1.0f;

    ubo.view = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(
            -(cam.position.x - cam.visibleWidth / 2.0f),
            -(cam.position.y - cam.visibleHeight / 2.0f),
            0.0f
        )
    );

    ubo.model = glm::mat4(1.0f);

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void VlkRenderer::recreateSwapChain(GLFWwindow *window) {
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain(window);
    createImageViews();
    createDepthResources();
    createFramebuffers();
}

void VlkRenderer::cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(vmaAllocator, depthImage, depthImageAllocation);

    for (auto framebuffer: swapChainFramebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    for (auto imageView: swapChainImageViews)
        vkDestroyImageView(device, imageView, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

// =========================================================================
// Mesh Management
// =========================================================================

void VlkRenderer::updateVertexBuffer(const std::string &name, const std::vector<Vertex> &vertices) {
    auto& mesh = meshes[name];

    uploadToGpuBuffer(
        mesh.vertexBuffer,
        mesh.vertexAllocation,
        vertices,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );

}

void VlkRenderer::updateVertexBuffer(int64_t key, const std::vector<Vertex> &vertices) {
    auto& mesh = chunkMeshes[key];

    uploadToGpuBuffer(
        mesh.vertexBuffer,
        mesh.vertexAllocation,
        vertices,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );
}

void VlkRenderer::updateIndexBuffer(const std::string &name, const std::vector<uint32_t> &indices) {
    auto& mesh = meshes[name];

    uploadToGpuBuffer(
        mesh.indexBuffer,
        mesh.indexAllocation,
        indices,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );

    mesh.indexCount = indices.size();
}

void VlkRenderer::updateIndexBuffer(int64_t key, const std::vector<uint32_t> &indices) {
    auto& mesh = chunkMeshes[key];

    uploadToGpuBuffer(
        mesh.indexBuffer,
        mesh.indexAllocation,
        indices,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );

    mesh.indexCount = indices.size();
}

void VlkRenderer::destroyMesh(const std::string &name) {
    destroyMeshImpl(meshes, name);
}

void VlkRenderer::destroyMesh(int64_t key) {
    destroyMeshImpl(chunkMeshes, key);
}

void VlkRenderer::setChunkKeys(const std::vector<int64_t>& keys) {
    chunkKeys = keys;
}

// =========================================================================
// Drawing
// =========================================================================

void VlkRenderer::draw(const std::string &name, VkCommandBuffer commandBuffer) {
    auto it = meshes.find(name);
    if (it != meshes.end()) drawMesh(it->second, commandBuffer);
}

// Overload
void VlkRenderer::draw(int64_t key, VkCommandBuffer commandBuffer) {
    auto it = chunkMeshes.find(key);
    if (it != chunkMeshes.end()) drawMesh(it->second, commandBuffer);
}

void VlkRenderer::drawUI(const std::string &name, VkCommandBuffer commandBuffer) {
    drawWithPush(name, commandBuffer, makeOrthoPush());
}

void VlkRenderer::drawText(VkCommandBuffer commandBuffer) {
    auto push = makeOrthoPush();
    push.useFont = 1;
    drawWithPush("__text__", commandBuffer, push);
}

void VlkRenderer::drawSky(VkCommandBuffer commandBuffer) {
    auto push = makeOrthoPush();
    push.useSky = 1;
    push.skyUVOffset = skyUVOffset;
    push.skyUVScale = {skyUVScaleX, skyUVScaleY};
    drawWithPush("__sky__", commandBuffer, push);
}

void VlkRenderer::drawSprite(const std::string &name, VkCommandBuffer commandBuffer) {
    auto push = makeOrthoPush();
    push.useSprite = 1;
    drawWithPush(name, commandBuffer, push);
}

// =========================================================================
// Text / Font
// =========================================================================

void VlkRenderer::createFontTexture(const std::string& fontPath, int fontSize) {
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Failed to open font");
    size_t size = file.tellg();
    file.seekg(0);
    std::vector<unsigned char> fontBuffer(size);
    file.read((char*)fontBuffer.data(), size);
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, fontBuffer.data(),
        stbtt_GetFontOffsetForIndex(fontBuffer.data(), 0)))
        throw std::runtime_error("Font init failed");

    const int W = 512;
    const int H = 512;
    std::vector<unsigned char> atlas(W * H, 0);
    float scale = stbtt_ScaleForPixelHeight(&font, (float)fontSize);
    int penX = 1, penY = 1, rowH = 0;

    for (int c = 32; c < 128; c++) {
        int gw, gh, xoff, yoff;
        unsigned char* sdf = stbtt_GetGlyphSDF(
            &font, scale, stbtt_FindGlyphIndex(&font, c),
            4, 128, 32.0f, &gw, &gh, &xoff, &yoff
        );
        if (!sdf) continue;
        if (penX + gw >= W) { penX = 1; penY += rowH + 1; rowH = 0; }
        if (penY + gh >= H) throw std::runtime_error("Font atlas too small");

        for (int y = 0; y < gh; y++)
            for (int x = 0; x < gw; x++)
                atlas[(penY + y) * W + (penX + x)] = sdf[y * gw + x];

        stbtt_FreeSDF(sdf, nullptr);

        SDFGlyph& g = sdfFont.glyphs[c - 32];
        g.u0 = (float)penX / W;
        g.v0 = (float)penY / H;
        g.u1 = (float)(penX + gw) / W;
        g.v1 = (float)(penY + gh) / H;
        g.xoff = (float)xoff;
        g.yoff = (float)yoff;

        int advanceWidth, lsb;
        stbtt_GetCodepointHMetrics(&font, c, &advanceWidth, &lsb);
        g.xadvance = advanceWidth * scale;
        g.width = gw;
        g.height = gh;
        penX += gw + 1;
        rowH = std::max(rowH, gh);
    }

    sdfFont.atlasWidth  = W;
    sdfFont.atlasHeight = H;

    VkDeviceSize imageSize = W * H;

    createImage(W, H, VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, fontImage, fontImageAllocation);

    uploadTexture(atlas.data(), imageSize, fontImage, VK_FORMAT_R8_UNORM, W, H);

    fontImageView = createImageView(fontImage, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    auto info = defaultSamplerInfo();

    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    fontSampler = makeSampler(info);

    std::cout << "[Renderer] SDF Font texture created!\n";
}

void VlkRenderer::buildTextMesh(const std::vector<TextDrawCall> &calls) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& call : calls) {
        float x = call.x;
        float y = call.y;

        for (char c : call.text) {
            if (c < 32 || c >= 128) continue;

            const SDFGlyph& g = sdfFont.glyphs[c - 32];

            float gx = x + g.xoff;
            float gy = y + g.yoff;  // yoff is negative = above baseline, pulls text up
            float w = g.width;
            float h = g.height;

            uint32_t base = (uint32_t)vertices.size();

            vertices.push_back({ {gx,     gy},     0.0f, call.color, {g.u0, g.v0} });
            vertices.push_back({ {gx + w, gy},     0.0f, call.color, {g.u1, g.v0} });
            vertices.push_back({ {gx + w, gy + h}, 0.0f, call.color, {g.u1, g.v1} });
            vertices.push_back({ {gx,     gy + h}, 0.0f, call.color, {g.u0, g.v1} });

            indices.insert(indices.end(), {
                base, base + 1, base + 2,
                base + 2, base + 3, base
            });

            x += g.xadvance;
        }
    }

    if (vertices.empty()) {
        destroyMesh("__text__");
        return;
    }

    updateVertexBuffer("__text__", vertices);
    updateIndexBuffer("__text__", indices);
}

void VlkRenderer::setTextDrawCalls(const std::vector<TextDrawCall> &calls) {
    textDrawCalls = calls;
}

// =========================================================================
// Sky
// =========================================================================

void VlkRenderer::createSkyTexture(const std::string &path) {
    TextureInfo texInfo = loadTextureImage(path);

    VkDeviceSize imageSize = texInfo.width * texInfo.height * 4;

    createImage(texInfo.width, texInfo.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, skyImage, skyImageAllocation);

    uploadTexture(
        texInfo.pixels, imageSize,
        skyImage, VK_FORMAT_R8G8B8A8_SRGB,
        texInfo.width, texInfo.height
    );

    skyImageView = createImageView(skyImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

    auto info = defaultSamplerInfo();

    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    skySampler = makeSampler(info);

    std::cout << "[Renderer] Sky texture created!\n";
}

void VlkRenderer::updateSkyMesh(const CameraParams &cam, float parallaxFactor) {
    float w = static_cast<float>(swapChainExtent.width);
    float h = static_cast<float>(swapChainExtent.height);

    // Only create the static quad once
    if (meshes.find("__sky__") == meshes.end()) {
        std::vector<Vertex> verts = {
            {{0, 0}, 0.99f, {1,1,1}, {0, 0}},
            {{w, 0}, 0.99f, {1,1,1}, {1, 0}},
            {{w, h}, 0.99f, {1,1,1}, {1, 1}},
            {{0, h}, 0.99f, {1,1,1}, {0, 1}},
        };
        std::vector<uint32_t> idxs = {0, 1, 2, 2, 3, 0};
        updateVertexBuffer("__sky__", verts);
        updateIndexBuffer("__sky__", idxs);
    }

    // Store parallax offset for use in drawSky
    skyUVOffset.x = fmod(cam.position.x * parallaxFactor / 1024.0f, 1.0f);
    skyUVOffset.y = 0.0f;
    skyUVScaleX = static_cast<float>(swapChainExtent.width)  / 1024.0f;
    skyUVScaleY = static_cast<float>(swapChainExtent.height) / 512.0f;
}

// =========================================================================
// Lightmap
// =========================================================================

void VlkRenderer::createLightmapTexture(int width, int height) {
    destroyLightmap();

    lightmapTexWidth = width;
    lightmapTexHeight = height;

    createImage(
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        lightmapImage, lightmapAllocation
    );

    // Put the image into SHADER_READ_ONLY_OPTIMAL immediately so the descriptor
    // write in createDescriptorSets references a valid layout
    transitionImageLayout(
        lightmapImage,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    lightmapImageView = createImageView(
        lightmapImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT
    );

    auto info = defaultSamplerInfo();
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    lightmapSampler = makeSampler(info);

    std::cout << "[Renderer] Lightmap texture created (" << width << "x" << height << ")\n";
}

void VlkRenderer::updateLightmap(const uint8_t *pixels, int width, int height) {
    if (width != lightmapTexWidth || height != lightmapTexHeight)
        createLightmapTexture(width, height);

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

    // Use persistent staging buffer instead of temp one
    ensureStagingBuffer(stagingOffset + imageSize);
    memcpy(static_cast<char*>(stagingMapped) + stagingOffset, pixels, imageSize);

    // Record barrier + copy into the open transfer command buffer
    VkImageMemoryBarrier toTransfer{};
    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = lightmapImage;
    toTransfer.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toTransfer.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(transferCommandBuffer,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toTransfer);

    VkBufferImageCopy region{};
    region.bufferOffset = stagingOffset;
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    vkCmdCopyBufferToImage(transferCommandBuffer, stagingBuffer, lightmapImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    stagingOffset += imageSize;

    VkImageMemoryBarrier toRead{};
    toRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.image = lightmapImage;
    toRead.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(transferCommandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toRead);

    // Update descriptors immediately
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo info{};
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = lightmapImageView;
        info.sampler = lightmapSampler;

        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = descriptorSets[i];
        w.dstBinding = 4;
        w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w.descriptorCount = 1;
        w.pImageInfo = &info;

        vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
    }
}

void VlkRenderer::destroyLightmap() {
    if (lightmapSampler != VK_NULL_HANDLE)
        vkDestroySampler(device, lightmapSampler, nullptr);
    if (lightmapImageView != VK_NULL_HANDLE)
        vkDestroyImageView(device, lightmapImageView, nullptr);
    if (lightmapImage != VK_NULL_HANDLE)
        vmaDestroyImage(vmaAllocator, lightmapImage, lightmapAllocation);

    lightmapSampler = VK_NULL_HANDLE;
    lightmapImageView = VK_NULL_HANDLE;
    lightmapImage = VK_NULL_HANDLE;
    lightmapTexWidth = 0;
    lightmapTexHeight = 0;
}

// =========================================================================
// Sprite Atlas
// =========================================================================

void VlkRenderer::createSpriteAtlas(const std::vector<uint8_t> &pixels, int width, int height) {
    destroySpriteAtlas();

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

    createImage(
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        spriteImage, spriteAllocation
    );

    uploadTexture(
        pixels.data(), imageSize,
        spriteImage, VK_FORMAT_R8G8B8A8_UNORM,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    );

    spriteImageView = createImageView(
        spriteImage,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    auto info = defaultSamplerInfo();
    info.magFilter = VK_FILTER_NEAREST;
    info.minFilter = VK_FILTER_NEAREST;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    spriteSampler = makeSampler(info);

    if (!descriptorSets.empty()) {
        updateSpriteAtlasDescriptors();
    }

    std::cout << "[Renderer] Sprite atlas uploaded: (" << width << "x" << height << ")\n";
}

void VlkRenderer::destroySpriteAtlas() {
    if (spriteSampler != VK_NULL_HANDLE)
        vkDestroySampler(device, spriteSampler, nullptr);
    if (spriteImageView != VK_NULL_HANDLE)
        vkDestroyImageView(device, spriteImageView, nullptr);
    if (spriteImage != VK_NULL_HANDLE)
        vmaDestroyImage(vmaAllocator, spriteImage, spriteAllocation);

    spriteSampler = VK_NULL_HANDLE;
    spriteImageView = VK_NULL_HANDLE;
    spriteImage = VK_NULL_HANDLE;
}

void VlkRenderer::updateSpriteAtlasDescriptors() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo imgInfo{};
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.imageView   = spriteImageView;
        imgInfo.sampler     = spriteSampler;

        VkWriteDescriptorSet w{};
        w.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet            = descriptorSets[i];
        w.dstBinding        = 5;
        w.descriptorType    = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w.descriptorCount   = 1;
        w.pImageInfo        = &imgInfo;
        vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
    }
}

// =========================================================================
// Buffers / Images / Memory
// =========================================================================

void VlkRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                               VmaMemoryUsage memUsage, VkBuffer &buffer,
                               VmaAllocation &allocation) {

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memUsage;

    if (vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }
}

void VlkRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset) {
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void VlkRenderer::destroyStagingBuffer() {
    if (stagingBuffer != VK_NULL_HANDLE) {
        vmaUnmapMemory(vmaAllocator, stagingAllocation);
        vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingAllocation);

        stagingBuffer = VK_NULL_HANDLE;
        stagingMapped = nullptr;
    }
}

void VlkRenderer::ensureStagingBuffer(VkDeviceSize size) {
    if (size <= stagingBufferSize) return;

    // Grow the staging buffer
    destroyStagingBuffer();
    stagingBufferSize = size * 2; // double to avoid frequent regrowth
    createBuffer(
        stagingBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        stagingBuffer, stagingAllocation
    );

    vmaMapMemory(vmaAllocator, stagingAllocation, &stagingMapped);
    std::cout << "[Staging] Grew to " << (stagingBufferSize / 1024 / 1024) << "MB\n";
}

void VlkRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                              VmaMemoryUsage memUsage, VkImage &image, VmaAllocation& imageAllocation) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memUsage;

    if (vmaCreateImage(vmaAllocator, &imageInfo, &allocInfo, &image, &imageAllocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image!");
}

void VlkRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void VlkRenderer::transitionImageLayout(VkImage image, VkFormat format,
                                        VkImageLayout oldLayout,
                                        VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // Used by createLightmapTexture to initialize a fresh image
        // directly into the read layout before any data is uploaded
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }else {
        throw std::invalid_argument("Unsupported layer transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0, 0,
        nullptr, 0,
        nullptr, 1,
        &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

VkImageView VlkRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture image view!");

    return imageView;
}

void VlkRenderer::beginTransferBatch() {
    if (transferOpen) return;

    // Wait for previous frame's transfers to complete
    vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &transferFence);
    vkResetCommandBuffer(transferCommandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

    stagingOffset = 0;
    transferOpen = true;
}

void VlkRenderer::endTransferBatch() {
    if (!transferOpen) return;

    vkEndCommandBuffer(transferCommandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCommandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, transferFence);
    // No vkQueueWaitIdle - GPU continues, we check fence next frame
    transferOpen = false;
}

void VlkRenderer::waitTransferComplete() {
    if (!transferOpen) {
        vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
    } else {
        endTransferBatch();
        vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
    }
}

// =========================================================================
// Device / Swapchain Queries
// =========================================================================

int VlkRenderer::rateDeviceSuitability(VkPhysicalDevice device) {
    int score = 0;

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;

    score += deviceProperties.limits.maxImageDimension2D;

    if (!deviceFeatures.geometryShader) return 0;

    return score;
}

bool VlkRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension: availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool VlkRenderer::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() &&
                            !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool VlkRenderer::hasStencilComponent(VkFormat format) {
    return false;
}

VlkRenderer::QueueFamilyIndices VlkRenderer::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily: queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) indices.presentFamily = i;

        if (indices.isComplete()) break;

        i++;
    }

    return indices;
}

VlkRenderer::SwapChainSupportDetails VlkRenderer::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VlkRenderer::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat: availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;

    return availableFormats[0];
}

VkPresentModeKHR VlkRenderer::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode: availablePresentModes)
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VlkRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width  = std::clamp(
        actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );

    actualExtent.height = std::clamp(
        actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return actualExtent;
}

VkFormat VlkRenderer::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return format;
    }
    throw std::runtime_error("Failed to find supported depth format!");
}

VkFormat VlkRenderer::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

// =========================================================================
// Commands / Utility
// =========================================================================

VkShaderModule VlkRenderer::createShaderModule(const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module!");

    return shaderModule;
}

VkCommandBuffer VlkRenderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VlkRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

std::vector<const char *> VlkRenderer::getRequiredExtensions() {
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (validator.enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
};

std::vector<char> VlkRenderer::readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file!");

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

// =========================================================================
// Cleanup
// =========================================================================

void VlkRenderer::cleanupMeshes() {
    for (auto& [key, mesh] : meshes) {
        if (mesh.vertexBuffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(vmaAllocator, mesh.vertexBuffer, mesh.vertexAllocation);
        if (mesh.indexBuffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(vmaAllocator, mesh.indexBuffer, mesh.indexAllocation);
    }
    meshes.clear();

    for (auto& [key, mesh] : chunkMeshes) {
        if (mesh.vertexBuffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(vmaAllocator, mesh.vertexBuffer, mesh.vertexAllocation);
        if (mesh.indexBuffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(vmaAllocator, mesh.indexBuffer, mesh.indexAllocation);
    }
    chunkMeshes.clear();
}

// #########################################################################
// PRIVATE SECTION
// #########################################################################

// =========================================================================
// Sampler creating helpers
// =========================================================================
VkSampler VlkRenderer::makeSampler(const VkSamplerCreateInfo &info) {
    VkSampler sampler;

    if (vkCreateSampler(device, &info, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create sampler!");

    return sampler;
}

VkSamplerCreateInfo VlkRenderer::defaultSamplerInfo() {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;

    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    info.anisotropyEnable = VK_FALSE;
    info.maxAnisotropy = 1.0f;

    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;

    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;

    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    return info;
}

// =========================================================================
// Frame rendering helper
// =========================================================================

void VlkRenderer::bindPipeline(VkCommandBuffer cmd, VkPipeline pipeline, const VkViewport &viewport, const VkRect2D &scissor) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout, 0, 1,
        &descriptorSets[currentFrame],
        0, nullptr
    );
}

// =========================================================================
// Texture creation helper
// =========================================================================

VlkRenderer::TextureInfo VlkRenderer::loadTextureImage(const std::string &path) {
    int w, h, ch;

    stbi_uc *pixels = stbi_load(
        path.c_str(),
        &w, &h,
        &ch,
        STBI_rgb_alpha
    );

    if (!pixels)
        throw std::runtime_error("Failed to load sky texture: " + path);

    if (w % 8 != 0 || h % 8 != 0)
        throw std::runtime_error("Texture atlas dimensions must be multiplies of 8!");

    TextureInfo info{};
    info.width = w;
    info.height = h;
    info.channels = ch;
    info.pixels = pixels;

    return info;
}

void VlkRenderer::uploadTexture(const void* pixels, VkDeviceSize imageSize, VkImage image, VkFormat format, uint32_t width, uint32_t height) {
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        stagingBuffer,
        stagingAllocation
    );

    void* data;
    vmaMapMemory(vmaAllocator, stagingAllocation, &data);
    memcpy(data, pixels, imageSize);
    vmaUnmapMemory(vmaAllocator, stagingAllocation);

    transitionImageLayout(
        image, format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    copyBufferToImage(stagingBuffer, image, width, height);

    transitionImageLayout(
        image, format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingAllocation);
}

// =========================================================================
// Mesh management helpers
// =========================================================================

template<typename Key>
VlkRenderer::MeshBuffer& VlkRenderer::getMeshBuffer(std::unordered_map<Key, MeshBuffer>& map, const Key& key) {
    return map[key];
}

template<typename T>
void VlkRenderer::uploadToGpuBuffer(VkBuffer& buffer, VmaAllocation& allocation, const std::vector<T>& data, VkBufferUsageFlags useFlags) {
    VkDeviceSize bufferSize = sizeof(T) * data.size();
    if (buffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(vmaAllocator, buffer, allocation);

    ensureStagingBuffer(stagingOffset + bufferSize);
    memcpy(static_cast<char*>(stagingMapped) + stagingOffset, data.data(), bufferSize);

    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | useFlags,
        VMA_MEMORY_USAGE_GPU_ONLY,
        buffer, allocation
    );

    copyBuffer(stagingBuffer, buffer, bufferSize, stagingOffset);
    stagingOffset += bufferSize;
}

template<typename Key>
void VlkRenderer::destroyMeshImpl(std::unordered_map<Key, MeshBuffer>& map, const Key& key) {
    auto it = map.find(key);
    if (it == map.end()) return;

    MeshBuffer& mesh = it->second;
    if (mesh.vertexBuffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(vmaAllocator, mesh.vertexBuffer, mesh.vertexAllocation);
    if (mesh.indexBuffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(vmaAllocator, mesh.indexBuffer, mesh.indexAllocation);
    map.erase(it);
}

// =========================================================================
// Drawing helpers
// =========================================================================

VlkRenderer::UIPushConstants VlkRenderer::makeOrthoPush() {
    UIPushConstants push{};
    push.useUIProj = 1;
    push.proj = glm::mat4(1.0f);
    push.proj[0][0] = 2.0f / swapChainExtent.width;
    push.proj[1][1] = 2.0f / swapChainExtent.height;
    push.proj[3][0] = -1.0f;
    push.proj[3][1] = -1.0f;
    return push;
}

void VlkRenderer::drawMesh(const MeshBuffer &mesh, VkCommandBuffer commandBuffer) {
    if (mesh.indexCount == 0) return;
    VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
}

void VlkRenderer::drawWithPush(const std::string &name, VkCommandBuffer commandBuffer, const UIPushConstants &push) {
    auto it = meshes.find(name);
    if (it == meshes.end() || it->second.indexCount == 0) return;

    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(UIPushConstants),
        &push
    );

    drawMesh(it->second, commandBuffer);

    const UIPushConstants reset{};
    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(UIPushConstants),
        &reset
    );
}