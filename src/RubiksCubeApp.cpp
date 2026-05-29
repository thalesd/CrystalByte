#include "RubiksCubeApp.h"
#include "Mesh.h"
#include "UpgradeStore.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Types.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <set>
#include <cstring>
#include <array>
#include <limits>
#include <iostream>

// ---------------------------------------------------------------------------
// Device extensions required
// ---------------------------------------------------------------------------

static const std::vector<const char*> kDeviceExts = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// ---------------------------------------------------------------------------
// run
// ---------------------------------------------------------------------------

void RubiksCubeApp::run(const MainMenuResult& cfg) {
    config = cfg;
    cube.scrambleWithRandomColors();
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

// ---------------------------------------------------------------------------
// initWindow
// ---------------------------------------------------------------------------

void RubiksCubeApp::initWindow() {
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(config.width, config.height, "CrystalByte — Rubik's Cube",
                              nullptr, nullptr);
    if (!window)
        throw std::runtime_error("Failed to create GLFW window");

    glfwSetWindowUserPointer(window, this);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glfwSetMouseButtonCallback(window, mouseBtnCb);
    glfwSetCursorPosCallback(window, mouseMoveCb);
    glfwSetKeyCallback(window, keyCb);
}

// ---------------------------------------------------------------------------
// initVulkan
// ---------------------------------------------------------------------------

void RubiksCubeApp::initVulkan() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createCubePipeline();
    createHudPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandPool();
    createCubeBuffers();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSet();
    createCommandBuffers();
    createSyncObjects();
}

// ---------------------------------------------------------------------------
// createInstance
// ---------------------------------------------------------------------------

void RubiksCubeApp::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "RubiksCube";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName        = "CrystalByte";
    appInfo.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    uint32_t     glfwExtCount = 0;
    const char** glfwExts     = glfwGetRequiredInstanceExtensions(&glfwExtCount);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = glfwExtCount;
    createInfo.ppEnabledExtensionNames = glfwExts;
    createInfo.enabledLayerCount       = 0;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Vulkan instance");

    std::cout << "RubiksCubeApp: Vulkan instance created\n";
}

// ---------------------------------------------------------------------------
// createSurface
// ---------------------------------------------------------------------------

void RubiksCubeApp::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface");
}

// ---------------------------------------------------------------------------
// Queue families
// ---------------------------------------------------------------------------

QueueFamilyIndices2 RubiksCubeApp::findQueueFamilies(VkPhysicalDevice dev) const {
    QueueFamilyIndices2 qfi;

    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());

    for (uint32_t i = 0; i < count; i++) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            qfi.graphicsFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
        if (presentSupport) qfi.presentFamily = i;

        if (qfi.isComplete()) break;
    }
    return qfi;
}

// ---------------------------------------------------------------------------
// Swap chain support
// ---------------------------------------------------------------------------

SwapChainSupportDetails2 RubiksCubeApp::querySwapChainSupport(VkPhysicalDevice dev) const {
    SwapChainSupportDetails2 details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);
    if (formatCount) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, details.formats.data());
    }

    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &modeCount, nullptr);
    if (modeCount) {
        details.presentModes.resize(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &modeCount,
                                                   details.presentModes.data());
    }
    return details;
}

bool RubiksCubeApp::isDeviceSuitable(VkPhysicalDevice dev) const {
    if (!findQueueFamilies(dev).isComplete()) return false;

    // Check device extension support
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &count, available.data());
    std::set<std::string> required(kDeviceExts.begin(), kDeviceExts.end());
    for (const auto& ext : available)
        required.erase(ext.extensionName);
    if (!required.empty()) return false;

    SwapChainSupportDetails2 sc = querySwapChainSupport(dev);
    return !sc.formats.empty() && !sc.presentModes.empty();
}

// ---------------------------------------------------------------------------
// pickPhysicalDevice
// ---------------------------------------------------------------------------

void RubiksCubeApp::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0)
        throw std::runtime_error("No Vulkan-capable GPU found");

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    for (const auto& dev : devices) {
        if (isDeviceSuitable(dev)) { physicalDevice = dev; break; }
    }
    if (physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("No suitable GPU found");
}

// ---------------------------------------------------------------------------
// createLogicalDevice
// ---------------------------------------------------------------------------

void RubiksCubeApp::createLogicalDevice() {
    QueueFamilyIndices2 qfi = findQueueFamilies(physicalDevice);
    std::set<uint32_t> uniqueFamilies = { qfi.graphicsFamily.value(), qfi.presentFamily.value() };

    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo qi{};
        qi.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = family;
        qi.queueCount       = 1;
        qi.pQueuePriorities = &priority;
        queueInfos.push_back(qi);
    }

    VkPhysicalDeviceFeatures features{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos       = queueInfos.data();
    createInfo.pEnabledFeatures        = &features;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(kDeviceExts.size());
    createInfo.ppEnabledExtensionNames = kDeviceExts.data();

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device");

    vkGetDeviceQueue(device, qfi.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, qfi.presentFamily.value(),  0, &presentQueue);
}

// ---------------------------------------------------------------------------
// Swap chain helpers
// ---------------------------------------------------------------------------

VkSurfaceFormatKHR RubiksCubeApp::chooseSwapFormat(
    const std::vector<VkSurfaceFormatKHR>& fmts) const
{
    for (const auto& f : fmts)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    return fmts[0];
}

VkPresentModeKHR RubiksCubeApp::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& modes) const
{
    for (const auto& m : modes)
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D RubiksCubeApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps) const {
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return caps.currentExtent;
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    return {
        std::clamp(static_cast<uint32_t>(w), caps.minImageExtent.width,  caps.maxImageExtent.width),
        std::clamp(static_cast<uint32_t>(h), caps.minImageExtent.height, caps.maxImageExtent.height)
    };
}

// ---------------------------------------------------------------------------
// createSwapChain
// ---------------------------------------------------------------------------

void RubiksCubeApp::createSwapChain() {
    SwapChainSupportDetails2 sc = querySwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR fmt  = chooseSwapFormat(sc.formats);
    VkPresentModeKHR   mode = chooseSwapPresentMode(sc.presentModes);
    VkExtent2D         ext  = chooseSwapExtent(sc.capabilities);

    uint32_t imageCount = sc.capabilities.minImageCount + 1;
    if (sc.capabilities.maxImageCount > 0)
        imageCount = std::min(imageCount, sc.capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = fmt.format;
    createInfo.imageColorSpace  = fmt.colorSpace;
    createInfo.imageExtent      = ext;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices2 qfi   = findQueueFamilies(physicalDevice);
    uint32_t            fams[] = { qfi.graphicsFamily.value(), qfi.presentFamily.value() };

    if (qfi.graphicsFamily != qfi.presentFamily) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = fams;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform   = sc.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = mode;
    createInfo.clipped        = VK_TRUE;
    createInfo.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain");

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainFormat = fmt.format;
    swapChainExtent = ext;
}

// ---------------------------------------------------------------------------
// Image views
// ---------------------------------------------------------------------------

VkImageView RubiksCubeApp::createImageView(VkImage img, VkFormat fmt, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo info{};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image                           = img;
    info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    info.format                          = fmt;
    info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.subresourceRange.aspectMask     = aspect;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 1;

    VkImageView view;
    if (vkCreateImageView(device, &info, nullptr, &view) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image view");
    return view;
}

void RubiksCubeApp::createImageViews() {
    swapChainViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++)
        swapChainViews[i] = createImageView(swapChainImages[i],
                                            swapChainFormat,
                                            VK_IMAGE_ASPECT_COLOR_BIT);
}

// ---------------------------------------------------------------------------
// Render pass
// ---------------------------------------------------------------------------

void RubiksCubeApp::createRenderPass() {
    VkAttachmentDescription color{};
    color.format         = swapChainFormat;
    color.samples        = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format         = VK_FORMAT_D32_SFLOAT;
    depth.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { color, depth };

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    rpInfo.pAttachments    = attachments.data();
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies   = &dep;

    if (vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass");
}

// ---------------------------------------------------------------------------
// Descriptor set layout
// ---------------------------------------------------------------------------

void RubiksCubeApp::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding            = 0;
    uboBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount    = 1;
    uboBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    uboBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings    = &uboBinding;

    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &cubeDescLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout");
}

// ---------------------------------------------------------------------------
// File / shader helpers
// ---------------------------------------------------------------------------

std::vector<char> RubiksCubeApp::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open shader: " + path);
    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buf(size);
    file.seekg(0);
    file.read(buf.data(), static_cast<std::streamsize>(size));
    return buf;
}

VkShaderModule RubiksCubeApp::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo info{};
    info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule mod;
    if (vkCreateShaderModule(device, &info, nullptr, &mod) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module");
    return mod;
}

// ---------------------------------------------------------------------------
// Cube pipeline
// ---------------------------------------------------------------------------

void RubiksCubeApp::createCubePipeline() {
    auto vertCode = readFile(std::string(SHADER_DIR) + "/cube.vert.spv");
    auto fragCode = readFile(std::string(SHADER_DIR) + "/cube.frag.spv");
    VkShaderModule vertMod = createShaderModule(vertCode);
    VkShaderModule fragMod = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertMod;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragMod;
    stages[1].pName  = "main";

    auto binding = Vertex::getBindingDescription();
    auto attribs = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertInput{};
    vertInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInput.vertexBindingDescriptionCount   = 1;
    vertInput.pVertexBindingDescriptions      = &binding;
    vertInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
    vertInput.pVertexAttributeDescriptions    = attribs.data();

    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.width    = static_cast<float>(swapChainExtent.width);
    viewport.height   = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth   = 1.0f;
    raster.cullMode    = VK_CULL_MODE_BACK_BIT;
    raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable  = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp   = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blendAtt;

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset     = 0;
    pushRange.size       = sizeof(CubePushConst); // mat4(64) + vec4(16) = 80

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 1;
    layoutInfo.pSetLayouts            = &cubeDescLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pushRange;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &cubePipeLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create cube pipeline layout");

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.stageCount          = 2;
    pipeInfo.pStages             = stages;
    pipeInfo.pVertexInputState   = &vertInput;
    pipeInfo.pInputAssemblyState = &assembly;
    pipeInfo.pViewportState      = &viewportState;
    pipeInfo.pRasterizationState = &raster;
    pipeInfo.pMultisampleState   = &ms;
    pipeInfo.pDepthStencilState  = &ds;
    pipeInfo.pColorBlendState    = &blend;
    pipeInfo.layout              = cubePipeLayout;
    pipeInfo.renderPass          = renderPass;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &cubePipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create cube pipeline");

    vkDestroyShaderModule(device, fragMod, nullptr);
    vkDestroyShaderModule(device, vertMod, nullptr);
}

// ---------------------------------------------------------------------------
// HUD pipeline
// ---------------------------------------------------------------------------

void RubiksCubeApp::createHudPipeline() {
    auto vertCode = readFile(std::string(SHADER_DIR) + "/hud.vert.spv");
    auto fragCode = readFile(std::string(SHADER_DIR) + "/hud.frag.spv");
    VkShaderModule vertMod = createShaderModule(vertCode);
    VkShaderModule fragMod = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertMod;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragMod;
    stages[1].pName  = "main";

    // No vertex input — vertices generated in shader via gl_VertexIndex
    VkPipelineVertexInputStateCreateInfo vertInput{};
    vertInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.width    = static_cast<float>(swapChainExtent.width);
    viewport.height   = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth   = 1.0f;
    raster.cullMode    = VK_CULL_MODE_NONE;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable  = VK_FALSE;
    ds.depthWriteEnable = VK_FALSE;

    // Alpha blend: srcAlpha*srcColor + (1-srcAlpha)*dstColor
    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.blendEnable         = VK_TRUE;
    blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAtt.colorBlendOp        = VK_BLEND_OP_ADD;
    blendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAtt.alphaBlendOp        = VK_BLEND_OP_ADD;
    blendAtt.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blendAtt;

    // rect(vec4) + color(vec4) + fill(float) = 36 bytes
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(HudPC); // 4+4+4+4+4 = 36 bytes

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 0;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pcRange;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &hudPipeLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create HUD pipeline layout");

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.stageCount          = 2;
    pipeInfo.pStages             = stages;
    pipeInfo.pVertexInputState   = &vertInput;
    pipeInfo.pInputAssemblyState = &assembly;
    pipeInfo.pViewportState      = &viewportState;
    pipeInfo.pRasterizationState = &raster;
    pipeInfo.pMultisampleState   = &ms;
    pipeInfo.pDepthStencilState  = &ds;
    pipeInfo.pColorBlendState    = &blend;
    pipeInfo.layout              = hudPipeLayout;
    pipeInfo.renderPass          = renderPass;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &hudPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create HUD pipeline");

    vkDestroyShaderModule(device, fragMod, nullptr);
    vkDestroyShaderModule(device, vertMod, nullptr);
}

// ---------------------------------------------------------------------------
// Depth resources
// ---------------------------------------------------------------------------

VkFormat RubiksCubeApp::findDepthFormat() const {
    return VK_FORMAT_D32_SFLOAT;
}

void RubiksCubeApp::createDepthResources() {
    VkFormat fmt = findDepthFormat();

    VkImageCreateInfo imgInfo{};
    imgInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType     = VK_IMAGE_TYPE_2D;
    imgInfo.extent        = { swapChainExtent.width, swapChainExtent.height, 1 };
    imgInfo.mipLevels     = 1;
    imgInfo.arrayLayers   = 1;
    imgInfo.format        = fmt;
    imgInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imgInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device, &imgInfo, nullptr, &depthImage) != VK_SUCCESS)
        throw std::runtime_error("Failed to create depth image");

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, depthImage, &req);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = req.size;
    allocInfo.memoryTypeIndex = findMemoryType(req.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &depthMem) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate depth image memory");

    vkBindImageMemory(device, depthImage, depthMem, 0);

    depthView = createImageView(depthImage, fmt, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// ---------------------------------------------------------------------------
// Framebuffers
// ---------------------------------------------------------------------------

void RubiksCubeApp::createFramebuffers() {
    framebuffers.resize(swapChainViews.size());
    for (size_t i = 0; i < swapChainViews.size(); i++) {
        std::array<VkImageView, 2> att = { swapChainViews[i], depthView };

        VkFramebufferCreateInfo info{};
        info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass      = renderPass;
        info.attachmentCount = static_cast<uint32_t>(att.size());
        info.pAttachments    = att.data();
        info.width           = swapChainExtent.width;
        info.height          = swapChainExtent.height;
        info.layers          = 1;

        if (vkCreateFramebuffer(device, &info, nullptr, &framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer");
    }
}

// ---------------------------------------------------------------------------
// Command pool
// ---------------------------------------------------------------------------

void RubiksCubeApp::createCommandPool() {
    QueueFamilyIndices2 qfi = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = qfi.graphicsFamily.value();

    if (vkCreateCommandPool(device, &info, nullptr, &cmdPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool");
}

// ---------------------------------------------------------------------------
// Memory helpers
// ---------------------------------------------------------------------------

uint32_t RubiksCubeApp::findMemoryType(uint32_t filter, VkMemoryPropertyFlags props) const {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        if ((filter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    throw std::runtime_error("Failed to find suitable memory type");
}

void RubiksCubeApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                  VkMemoryPropertyFlags props,
                                  VkBuffer& buf, VkDeviceMemory& mem) {
    VkBufferCreateInfo info{};
    info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size        = size;
    info.usage       = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &info, nullptr, &buf) != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, buf, &req);

    VkMemoryAllocateInfo alloc{};
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = req.size;
    alloc.memoryTypeIndex = findMemoryType(req.memoryTypeBits, props);

    if (vkAllocateMemory(device, &alloc, nullptr, &mem) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory");

    vkBindBufferMemory(device, buf, mem, 0);
}

VkCommandBuffer RubiksCubeApp::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo alloc{};
    alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandPool        = cmdPool;
    alloc.commandBufferCount = 1;

    VkCommandBuffer cb;
    vkAllocateCommandBuffers(device, &alloc, &cb);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &begin);
    return cb;
}

void RubiksCubeApp::endSingleTimeCommands(VkCommandBuffer cb) {
    vkEndCommandBuffer(cb);

    VkSubmitInfo submit{};
    submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers    = &cb;

    vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, cmdPool, 1, &cb);
}

void RubiksCubeApp::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBuffer cb = beginSingleTimeCommands();
    VkBufferCopy region{};
    region.size = size;
    vkCmdCopyBuffer(cb, src, dst, 1, &region);
    endSingleTimeCommands(cb);
}

// ---------------------------------------------------------------------------
// Cube buffers
// ---------------------------------------------------------------------------

void RubiksCubeApp::createCubeBuffers() {
    Mesh m = Mesh::makeCube();
    cubeIndexCount = static_cast<uint32_t>(m.indices.size()); // 36

    // Vertex buffer
    {
        VkDeviceSize size = sizeof(Vertex) * m.vertices.size();
        VkBuffer staging; VkDeviceMemory stagingMem;
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     staging, stagingMem);
        void* data;
        vkMapMemory(device, stagingMem, 0, size, 0, &data);
        std::memcpy(data, m.vertices.data(), static_cast<size_t>(size));
        vkUnmapMemory(device, stagingMem);
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cubeVB, cubeVBMem);
        copyBuffer(staging, cubeVB, size);
        vkDestroyBuffer(device, staging, nullptr);
        vkFreeMemory(device, stagingMem, nullptr);
    }

    // Index buffer
    {
        VkDeviceSize size = sizeof(uint32_t) * m.indices.size();
        VkBuffer staging; VkDeviceMemory stagingMem;
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     staging, stagingMem);
        void* data;
        vkMapMemory(device, stagingMem, 0, size, 0, &data);
        std::memcpy(data, m.indices.data(), static_cast<size_t>(size));
        vkUnmapMemory(device, stagingMem);
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cubeIB, cubeIBMem);
        copyBuffer(staging, cubeIB, size);
        vkDestroyBuffer(device, staging, nullptr);
        vkFreeMemory(device, stagingMem, nullptr);
    }
}

// ---------------------------------------------------------------------------
// Uniform buffer
// ---------------------------------------------------------------------------

void RubiksCubeApp::createUniformBuffer() {
    VkDeviceSize size = sizeof(UniformBufferObject);
    createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 ubo, uboMem);
    vkMapMemory(device, uboMem, 0, size, 0, &uboMapped);
}

// ---------------------------------------------------------------------------
// Descriptors
// ---------------------------------------------------------------------------

void RubiksCubeApp::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 1;
    info.pPoolSizes    = &poolSize;
    info.maxSets       = 1;

    if (vkCreateDescriptorPool(device, &info, nullptr, &descPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool");
}

void RubiksCubeApp::createDescriptorSet() {
    VkDescriptorSetAllocateInfo alloc{};
    alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc.descriptorPool     = descPool;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts        = &cubeDescLayout;

    if (vkAllocateDescriptorSets(device, &alloc, &descSet) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor set");

    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = ubo;
    bufInfo.offset = 0;
    bufInfo.range  = sizeof(UniformBufferObject);

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = descSet;
    write.dstBinding      = 0;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo     = &bufInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

// ---------------------------------------------------------------------------
// Command buffers
// ---------------------------------------------------------------------------

void RubiksCubeApp::createCommandBuffers() {
    cmdBufs.resize(framebuffers.size());

    VkCommandBufferAllocateInfo alloc{};
    alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool        = cmdPool;
    alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = static_cast<uint32_t>(cmdBufs.size());

    if (vkAllocateCommandBuffers(device, &alloc, cmdBufs.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers");
}

// ---------------------------------------------------------------------------
// Sync objects
// ---------------------------------------------------------------------------

void RubiksCubeApp::createSyncObjects() {
    VkSemaphoreCreateInfo sem{};
    sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &sem, nullptr, &imgAvail)   != VK_SUCCESS ||
        vkCreateSemaphore(device, &sem, nullptr, &renderDone) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &fence)    != VK_SUCCESS)
        throw std::runtime_error("Failed to create sync objects");
}

// ---------------------------------------------------------------------------
// Update uniform buffer (camera orbit)
// ---------------------------------------------------------------------------

void RubiksCubeApp::updateUniformBuffer() {
    float cx = std::sin(orbitYaw)   * std::cos(orbitPitch);
    float cy = std::sin(orbitPitch);
    float cz = std::cos(orbitYaw)   * std::cos(orbitPitch);
    glm::vec3 camPos = glm::vec3(cx, cy, cz) * orbitDist;

    UniformBufferObject uboData{};
    uboData.view = glm::lookAt(camPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    uboData.proj = glm::perspective(glm::radians(45.0f),
        static_cast<float>(swapChainExtent.width) /
        static_cast<float>(swapChainExtent.height),
        0.1f, 100.0f);
    uboData.proj[1][1] *= -1.0f; // Vulkan Y flip

    std::memcpy(uboMapped, &uboData, sizeof(uboData));
}

// ---------------------------------------------------------------------------
// Sticker color
// ---------------------------------------------------------------------------

glm::vec4 RubiksCubeApp::stickerColor(int gx, int gy, int gz, int meshFace) const {
    // Dark gray for interior faces
    const glm::vec4 kInner(0.08f, 0.08f, 0.08f, 1.0f);

    // Returns the color of a specific face of a specific cubie.
    // meshFace: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z (from Mesh::makeCube)
    RColor rc;
    int row = 0, col = 0;
    bool outer = false;

    switch (meshFace) {
        case 0: // +X = RIGHT face, outer if gx==2
            if (gx != 2) return kInner;
            outer = true;
            rc  = RColor::BLUE; // will be overwritten
            row = 2 - gy;
            col = 2 - gz;
            rc  = cube.faces[F_RIGHT][row][col];
            break;
        case 1: // -X = LEFT face, outer if gx==0
            if (gx != 0) return kInner;
            outer = true;
            row = 2 - gy;
            col = gz;
            rc  = cube.faces[F_LEFT][row][col];
            break;
        case 2: // +Y = UP face, outer if gy==2
            if (gy != 2) return kInner;
            outer = true;
            row = gz;
            col = gx;
            rc  = cube.faces[F_UP][row][col];
            break;
        case 3: // -Y = DOWN face, outer if gy==0
            if (gy != 0) return kInner;
            outer = true;
            row = 2 - gz;
            col = gx;
            rc  = cube.faces[F_DOWN][row][col];
            break;
        case 4: // +Z = FRONT face, outer if gz==2
            if (gz != 2) return kInner;
            outer = true;
            row = 2 - gy;
            col = gx;
            rc  = cube.faces[F_FRONT][row][col];
            break;
        case 5: // -Z = BACK face, outer if gz==0
            if (gz != 0) return kInner;
            outer = true;
            row = 2 - gy;
            col = 2 - gx;
            rc  = cube.faces[F_BACK][row][col];
            break;
        default:
            return kInner;
    }

    (void)outer;

    switch (rc) {
        case RColor::WHITE:  return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        case RColor::YELLOW: return glm::vec4(1.0f, 0.9f, 0.0f, 1.0f);
        case RColor::RED:    return glm::vec4(0.9f, 0.1f, 0.1f, 1.0f);
        case RColor::ORANGE: return glm::vec4(1.0f, 0.45f, 0.0f, 1.0f);
        case RColor::BLUE:   return glm::vec4(0.05f, 0.25f, 0.95f, 1.0f);
        case RColor::GREEN:  return glm::vec4(0.05f, 0.65f, 0.15f, 1.0f);
        default:             return kInner;
    }
}

// ---------------------------------------------------------------------------
// Cubie transform matrix
// ---------------------------------------------------------------------------

glm::mat4 RubiksCubeApp::cubieMatrix(int gx, int gy, int gz) const {
    // Axes for animation rotation
    static const glm::vec3 axes[3] = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
    };

    glm::mat4 baseTranslate = glm::translate(glm::mat4(1.0f),
        glm::vec3(float(gx - 1), float(gy - 1), float(gz - 1)));
    glm::mat4 scaleM = glm::scale(glm::mat4(1.0f), glm::vec3(0.94f));

    if (animating) {
        bool inLayer = false;
        if      (animMove.axis == 0 && gx == animMove.layer) inLayer = true;
        else if (animMove.axis == 1 && gy == animMove.layer) inLayer = true;
        else if (animMove.axis == 2 && gz == animMove.layer) inLayer = true;

        if (inLayer) {
            float t = animTimer / animDuration;
            float angle = animMove.sign * glm::radians(90.0f) * t;
            glm::mat4 animRot = glm::rotate(glm::mat4(1.0f), angle, axes[animMove.axis]);
            // animRot is applied around origin, then translate, then scale
            return animRot * baseTranslate * scaleM;
        }
    }

    return baseTranslate * scaleM;
}

// ---------------------------------------------------------------------------
// Record command buffer
// ---------------------------------------------------------------------------

void RubiksCubeApp::recordCommandBuffer(uint32_t imageIndex) {
    VkCommandBuffer cb = cmdBufs[imageIndex];

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cb, &begin) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer");

    VkClearValue clears[2];
    std::memset(clears, 0, sizeof(clears));
    clears[0].color.float32[0] = 0.05f;
    clears[0].color.float32[1] = 0.05f;
    clears[0].color.float32[2] = 0.08f;
    clears[0].color.float32[3] = 1.0f;
    clears[1].depthStencil.depth   = 1.0f;
    clears[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo rp{};
    rp.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass        = renderPass;
    rp.framebuffer       = framebuffers[imageIndex];
    rp.renderArea.extent = swapChainExtent;
    rp.clearValueCount   = 2;
    rp.pClearValues      = clears;

    vkCmdBeginRenderPass(cb, &rp, VK_SUBPASS_CONTENTS_INLINE);

    // -----------------------------------------------------------------------
    // Draw the Rubik's cube (27 cubies x 6 faces)
    // -----------------------------------------------------------------------
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, cubePipeline);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            cubePipeLayout, 0, 1, &descSet, 0, nullptr);

    const VkDeviceSize zero = 0;
    vkCmdBindVertexBuffers(cb, 0, 1, &cubeVB, &zero);
    vkCmdBindIndexBuffer(cb, cubeIB, 0, VK_INDEX_TYPE_UINT32);

    constexpr VkShaderStageFlags kCubeStages =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    for (int gx = 0; gx < 3; ++gx) {
        for (int gy = 0; gy < 3; ++gy) {
            for (int gz = 0; gz < 3; ++gz) {
                glm::mat4 model = cubieMatrix(gx, gy, gz);
                for (int face = 0; face < 6; ++face) {
                    CubePushConst pc{};
                    pc.model = model;
                    pc.color = stickerColor(gx, gy, gz, face);
                    vkCmdPushConstants(cb, cubePipeLayout, kCubeStages,
                                       0, sizeof(CubePushConst), &pc);
                    // Each face uses indices [face*6 .. face*6+5]
                    vkCmdDrawIndexed(cb, 6, 1, face * 6, 0, 0);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Draw HUD (score bar)
    // -----------------------------------------------------------------------
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, hudPipeline);

    constexpr VkShaderStageFlags kHudStages =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    int score = computeWeightedScore();
    float fill = static_cast<float>(score) / 72.0f;

    // Score bar background (dark gray)
    {
        HudPC pc{};
        pc.rect  = glm::vec4(-0.9f, -0.98f, 1.8f, 0.08f);
        pc.color = glm::vec4(0.2f, 0.2f, 0.2f, 0.6f);
        pc.fill  = 1.0f;
        vkCmdPushConstants(cb, hudPipeLayout, kHudStages, 0, sizeof(HudPC), &pc);
        vkCmdDraw(cb, 6, 1, 0, 0);
    }

    // Score bar fill (green)
    {
        HudPC pc{};
        pc.rect  = glm::vec4(-0.9f, -0.98f, 1.8f, 0.08f);
        pc.color = glm::vec4(0.2f, 0.8f, 0.2f, 0.9f);
        pc.fill  = fill;
        vkCmdPushConstants(cb, hudPipeLayout, kHudStages, 0, sizeof(HudPC), &pc);
        vkCmdDraw(cb, 6, 1, 0, 0);
    }

    // Commit flash overlay at bottom center
    if (commitFlash > 0.0f) {
        HudPC pc{};
        pc.rect  = glm::vec4(-0.3f, 0.88f, 0.6f, 0.1f);
        pc.color = glm::vec4(1.0f, 1.0f, 0.0f, commitFlash);
        pc.fill  = 1.0f;
        vkCmdPushConstants(cb, hudPipeLayout, kHudStages, 0, sizeof(HudPC), &pc);
        vkCmdDraw(cb, 6, 1, 0, 0);
    }

    vkCmdEndRenderPass(cb);
    if (vkEndCommandBuffer(cb) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer");
}

// ---------------------------------------------------------------------------
// Draw frame
// ---------------------------------------------------------------------------

void RubiksCubeApp::drawFrame() {
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                             imgAvail, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        return;

    vkResetFences(device, 1, &fence);
    vkResetCommandBuffer(cmdBufs[imageIndex], 0);
    recordCommandBuffer(imageIndex);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &imgAvail;
    submit.pWaitDstStageMask    = &waitStage;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &cmdBufs[imageIndex];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &renderDone;

    if (vkQueueSubmit(graphicsQueue, 1, &submit, fence) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command");

    VkPresentInfoKHR present{};
    present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores    = &renderDone;
    present.swapchainCount     = 1;
    present.pSwapchains        = &swapChain;
    present.pImageIndices      = &imageIndex;

    vkQueuePresentKHR(presentQueue, &present);
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

void RubiksCubeApp::mainLoop() {
    lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window) && !shouldQuit) {
        double now = glfwGetTime();
        deltaTime  = static_cast<float>(now - lastTime);
        lastTime   = now;

        commitFlash = std::max(0.0f, commitFlash - deltaTime * 2.0f);

        glfwPollEvents();

        // Process animation
        animDuration = upgrades.fastMoves ? 0.09f : 0.18f;
        if (animating) {
            animTimer += deltaTime;
            if (animTimer >= animDuration) {
                animating = false;
                applyMoveToCube(cube, animMove);
                moveHistory.push_back(animMove);
                if (moveHistory.size() > 50)
                    moveHistory.erase(moveHistory.begin());
                // Start next move if queued
                if (!moveQueue.empty()) {
                    animMove = moveQueue.front();
                    moveQueue.pop_front();
                    animating  = true;
                    animTimer  = 0.0f;
                }
            }
        } else if (!moveQueue.empty()) {
            animMove = moveQueue.front();
            moveQueue.pop_front();
            animating = true;
            animTimer = 0.0f;
        }

        updateUniformBuffer();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

// ---------------------------------------------------------------------------
// commitScore
// ---------------------------------------------------------------------------

int RubiksCubeApp::computeWeightedScore() const {
    int score = 0;
    for (int f = 0; f < 6; ++f) {
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                int mult = upgrades.colorBonus[static_cast<int>(cube.faces[f][r][c])] ? 2 : 1;
                if (c < 2 && cube.faces[f][r][c] == cube.faces[f][r][c+1])
                    score += mult;
                if (r < 2 && cube.faces[f][r][c] == cube.faces[f][r+1][c])
                    score += mult;
            }
        }
    }
    return score;
}

void RubiksCubeApp::commitScore() {
    int score = computeWeightedScore();
    float mult = 1.0f + 0.5f * static_cast<float>(upgrades.multiplierLevel);
    int finalScore = static_cast<int>(static_cast<float>(score) * mult);
    upgrades.totalScore += finalScore;
    commitFlash = 1.0f;

    // Pause game, generate 3 upgrade options, show store
    vkDeviceWaitIdle(device);
    auto options = generateUpgradeOptions(upgrades);
    StoreAction action = showUpgradeStore(upgrades, finalScore, options);

    // Always rescramble with randomised colors after a commit
    cube.scrambleWithRandomColors();
    moveHistory.clear();
    moveQueue.clear();
    animating = false;

    if (action == StoreAction::QUIT)
        shouldQuit = true;
}

// ---------------------------------------------------------------------------
// queueMove
// ---------------------------------------------------------------------------

void RubiksCubeApp::queueMove(CubeMove m) {
    if (moveQueue.size() < 20)
        moveQueue.push_back(m);
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

void RubiksCubeApp::mouseBtnCb(GLFWwindow* w, int btn, int action, int /*mods*/) {
    auto* app = static_cast<RubiksCubeApp*>(glfwGetWindowUserPointer(w));
    if (btn == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            app->mouseDown = true;
            glfwGetCursorPos(w, &app->prevMX, &app->prevMY);
        } else if (action == GLFW_RELEASE) {
            app->mouseDown = false;
        }
    }
}

void RubiksCubeApp::mouseMoveCb(GLFWwindow* w, double x, double y) {
    auto* app = static_cast<RubiksCubeApp*>(glfwGetWindowUserPointer(w));
    if (app->mouseDown) {
        double dx = x - app->prevMX;
        double dy = y - app->prevMY;
        app->orbitYaw   += static_cast<float>(dx) * 0.01f;
        app->orbitPitch += static_cast<float>(dy) * 0.01f;
        // Clamp pitch to avoid gimbal singularities
        app->orbitPitch = std::clamp(app->orbitPitch, -1.4f, 1.4f);
        app->prevMX = x;
        app->prevMY = y;
    }
}

void RubiksCubeApp::keyCb(GLFWwindow* w, int key, int /*sc*/, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        auto* app  = static_cast<RubiksCubeApp*>(glfwGetWindowUserPointer(w));
        switch (key) {
            case GLFW_KEY_U: app->queueMove(shift ? CubeMove::UP() : CubeMove::U()); break;
            case GLFW_KEY_D: app->queueMove(shift ? CubeMove::DP() : CubeMove::D()); break;
            case GLFW_KEY_F: app->queueMove(shift ? CubeMove::FP() : CubeMove::F()); break;
            case GLFW_KEY_B: app->queueMove(shift ? CubeMove::BP() : CubeMove::B()); break;
            case GLFW_KEY_L: app->queueMove(shift ? CubeMove::LP() : CubeMove::L()); break;
            case GLFW_KEY_R: app->queueMove(shift ? CubeMove::RP() : CubeMove::R()); break;
            case GLFW_KEY_SPACE: app->commitScore(); break;
            case GLFW_KEY_Z:
                if (!shift && !app->moveHistory.empty() &&
                    app->upgrades.undosRemaining > 0) {
                    CubeMove rev = app->moveHistory.back();
                    app->moveHistory.pop_back();
                    rev.sign = -rev.sign;
                    app->queueMove(rev);
                    app->upgrades.undosRemaining--;
                }
                break;
            case GLFW_KEY_S:
                if (shift) { app->cube.scramble(); }
                break;
            case GLFW_KEY_ESCAPE:
                app->shouldQuit = true;
                break;
            default:
                break;
        }
    }
}

// ---------------------------------------------------------------------------
// processInput (polling — unused in favor of keyCb, kept for completeness)
// ---------------------------------------------------------------------------

void RubiksCubeApp::processInput() {
    // Input handled via GLFW callbacks (keyCb / mouseBtnCb / mouseMoveCb)
}

// ---------------------------------------------------------------------------
// cleanup
// ---------------------------------------------------------------------------

void RubiksCubeApp::cleanup() {
    vkDestroySemaphore(device, renderDone, nullptr);
    vkDestroySemaphore(device, imgAvail, nullptr);
    vkDestroyFence(device, fence, nullptr);

    vkDestroyCommandPool(device, cmdPool, nullptr);

    for (auto fb : framebuffers)
        vkDestroyFramebuffer(device, fb, nullptr);

    vkDestroyPipeline(device, hudPipeline, nullptr);
    vkDestroyPipelineLayout(device, hudPipeLayout, nullptr);
    vkDestroyPipeline(device, cubePipeline, nullptr);
    vkDestroyPipelineLayout(device, cubePipeLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    vkDestroyImageView(device, depthView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthMem, nullptr);

    vkUnmapMemory(device, uboMem);
    vkDestroyBuffer(device, ubo, nullptr);
    vkFreeMemory(device, uboMem, nullptr);

    vkDestroyBuffer(device, cubeIB, nullptr);
    vkFreeMemory(device, cubeIBMem, nullptr);
    vkDestroyBuffer(device, cubeVB, nullptr);
    vkFreeMemory(device, cubeVBMem, nullptr);

    vkDestroyDescriptorPool(device, descPool, nullptr);
    vkDestroyDescriptorSetLayout(device, cubeDescLayout, nullptr);

    for (auto v : swapChainViews)
        vkDestroyImageView(device, v, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}
