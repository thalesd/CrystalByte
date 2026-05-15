#include "Application.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <random>

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <limits>
#include <string>
#include <fstream>
#include <stdexcept>
#include <array>
#include <cstring>
#include <cmath>
#include <thread>
#include <chrono>

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void VulkanApplication::run(const std::string& modelPath, const LaunchConfig& cfg) {
    config = cfg;

    // Auto-detect a texture image next to the model file
    std::string base = modelPath.substr(0, modelPath.find_last_of('.'));
    for (const char* ext : {".png", ".jpg", ".jpeg", ".PNG", ".JPG", ".JPEG"}) {
        std::ifstream probe(base + ext, std::ios::binary);
        if (probe.good()) { texturePath = base + ext; break; }
    }
    if (texturePath.empty())
        std::cout << "No texture found next to model — using white fallback\n";
    else
        std::cout << "Texture: " << texturePath << "\n";

    mesh         = Mesh::makeCone(32);
    asteroidMesh = Mesh::makeSphere(16, 32);
    spawnAsteroids();
    std::cout << "Player cone: "   << mesh.vertices.size()         << " vertices\n";
    std::cout << "Asteroid sphere: " << asteroidMesh.vertices.size() << " vertices\n";
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------

void VulkanApplication::initWindow() {
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(config.width, config.height, "CrystalByte", nullptr, nullptr);
    if (!window)
        throw std::runtime_error("Failed to create GLFW window");

    glfwSetWindowUserPointer(window, this);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    setupCallbacks();
}

void VulkanApplication::setupCallbacks() {
    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y) {
        static_cast<VulkanApplication*>(glfwGetWindowUserPointer(w))->onMouseMove(x, y);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods) {
        static_cast<VulkanApplication*>(glfwGetWindowUserPointer(w))->onMouseButton(button, action, mods);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(w, GLFW_TRUE);
    });
}

void VulkanApplication::onMouseMove(double xpos, double ypos) {
    if (firstMouseSample) {
        prevMouseX       = xpos;
        prevMouseY       = ypos;
        firstMouseSample = false;
        return;
    }

    float dx = static_cast<float>(xpos - prevMouseX);
    float dy = static_cast<float>(ypos - prevMouseY);
    prevMouseX = xpos;
    prevMouseY = ypos;

    camera.applyMouseDelta(dx, dy);
}

void VulkanApplication::onMouseButton(int button, int action, int mods) {
    (void)mods;
    if (action == GLFW_PRESS) {
        firstMouseSample = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            pendingShoot = true;
    }
}

// ---------------------------------------------------------------------------
// Vulkan init sequence
// ---------------------------------------------------------------------------

void VulkanApplication::initVulkan() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createDescriptorSetLayout();
    createRenderPass();
    createGraphicsPipeline();
    createCrosshairPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandPool();
    createAsteroidBuffers();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSet();
    createCommandBuffers();
    createSyncObjects();
}

// ---------------------------------------------------------------------------
// Instance
// ---------------------------------------------------------------------------

void VulkanApplication::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "CrystalByte";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName        = "CrystalByte Engine";
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

    std::cout << "Vulkan instance created\n";
}

// ---------------------------------------------------------------------------
// Surface
// ---------------------------------------------------------------------------

void VulkanApplication::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface");
    std::cout << "Window surface created\n";
}

// ---------------------------------------------------------------------------
// Physical device
// ---------------------------------------------------------------------------

QueueFamilyIndices VulkanApplication::findQueueFamilies(VkPhysicalDevice dev) const {
    QueueFamilyIndices qfi;

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

bool VulkanApplication::checkDeviceExtensionSupport(VkPhysicalDevice dev) const {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &count, available.data());

    std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& ext : available)
        required.erase(ext.extensionName);
    return required.empty();
}

SwapChainSupportDetails VulkanApplication::querySwapChainSupport(VkPhysicalDevice dev) const {
    SwapChainSupportDetails details;
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
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &modeCount, details.presentModes.data());
    }
    return details;
}

bool VulkanApplication::isDeviceSuitable(VkPhysicalDevice dev) const {
    if (!findQueueFamilies(dev).isComplete())        return false;
    if (!checkDeviceExtensionSupport(dev))           return false;
    SwapChainSupportDetails sc = querySwapChainSupport(dev);
    return !sc.formats.empty() && !sc.presentModes.empty();
}

void VulkanApplication::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) throw std::runtime_error("No Vulkan-capable GPU found");

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    for (const auto& dev : devices) {
        if (isDeviceSuitable(dev)) { physicalDevice = dev; break; }
    }
    if (physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("No suitable GPU found");

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    std::cout << "Selected GPU: " << props.deviceName << "\n";
}

// ---------------------------------------------------------------------------
// Logical device + queues
// ---------------------------------------------------------------------------

void VulkanApplication::createLogicalDevice() {
    QueueFamilyIndices qfi = findQueueFamilies(physicalDevice);
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
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device");

    vkGetDeviceQueue(device, qfi.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, qfi.presentFamily.value(),  0, &presentQueue);
    std::cout << "Logical device and queues created\n";
}

// ---------------------------------------------------------------------------
// Swap chain
// ---------------------------------------------------------------------------

VkSurfaceFormatKHR VulkanApplication::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& formats) const
{
    for (const auto& f : formats)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    return formats[0];
}

VkPresentModeKHR VulkanApplication::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& modes) const
{
    for (const auto& m : modes)
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps) const {
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return caps.currentExtent;
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    return {
        std::clamp(static_cast<uint32_t>(w), caps.minImageExtent.width,  caps.maxImageExtent.width),
        std::clamp(static_cast<uint32_t>(h), caps.minImageExtent.height, caps.maxImageExtent.height)
    };
}

void VulkanApplication::createSwapChain() {
    SwapChainSupportDetails sc = querySwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR fmt   = chooseSwapSurfaceFormat(sc.formats);
    VkPresentModeKHR   mode  = chooseSwapPresentMode(sc.presentModes);
    VkExtent2D         ext   = chooseSwapExtent(sc.capabilities);

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

    QueueFamilyIndices qfi    = findQueueFamilies(physicalDevice);
    uint32_t           fams[] = { qfi.graphicsFamily.value(), qfi.presentFamily.value() };

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

    swapChainImageFormat = fmt.format;
    swapChainExtent      = ext;

    std::cout << "Swap chain created: " << imageCount << " images @ "
              << ext.width << "x" << ext.height << "\n";
}

// ---------------------------------------------------------------------------
// Image views
// ---------------------------------------------------------------------------

VkImageView VulkanApplication::createImageViewHelper(
    VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo info{};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image                           = image;
    info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    info.format                          = format;
    info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.subresourceRange.aspectMask     = aspectFlags;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 1;

    VkImageView view;
    if (vkCreateImageView(device, &info, nullptr, &view) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image view");
    return view;
}

void VulkanApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++)
        swapChainImageViews[i] = createImageViewHelper(
            swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    std::cout << "Image views created: " << swapChainImageViews.size() << "\n";
}

// ---------------------------------------------------------------------------
// Descriptor set layout
// ---------------------------------------------------------------------------

void VulkanApplication::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding ubo{};
    ubo.binding            = 0;
    ubo.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo.descriptorCount    = 1;
    ubo.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    ubo.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding sampler{};
    sampler.binding            = 1;
    sampler.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler.descriptorCount    = 1;
    sampler.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    sampler.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo, sampler };
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings    = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout");
    std::cout << "Descriptor set layout created\n";
}

// ---------------------------------------------------------------------------
// Render pass
// ---------------------------------------------------------------------------

void VulkanApplication::createRenderPass() {
    VkAttachmentDescription color{};
    color.format         = swapChainImageFormat;
    color.samples        = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format         = findDepthFormat();
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
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

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
    std::cout << "Render pass created\n";
}

// ---------------------------------------------------------------------------
// Graphics pipeline
// ---------------------------------------------------------------------------

std::vector<char> VulkanApplication::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open shader: " + path);
    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buf(size);
    file.seekg(0);
    file.read(buf.data(), static_cast<std::streamsize>(size));
    return buf;
}

VkShaderModule VulkanApplication::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo info{};
    info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule mod;
    if (vkCreateShaderModule(device, &info, nullptr, &mod) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module");
    return mod;
}

void VulkanApplication::createGraphicsPipeline() {
    auto vertCode = readFile(std::string(SHADER_DIR) + "/triangle.vert.spv");
    auto fragCode = readFile(std::string(SHADER_DIR) + "/triangle.frag.spv");
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
    pushRange.size       = sizeof(PushConstants); // model (64 B) + baseColor (16 B) = 80 B

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 1;
    layoutInfo.pSetLayouts            = &descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pushRange;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout");

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
    pipeInfo.layout              = pipelineLayout;
    pipeInfo.renderPass          = renderPass;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline");

    vkDestroyShaderModule(device, fragMod, nullptr);
    vkDestroyShaderModule(device, vertMod, nullptr);
    std::cout << "Graphics pipeline created\n";
}

void VulkanApplication::createCrosshairPipeline() {
    auto vertCode = readFile(std::string(SHADER_DIR) + "/crosshair.vert.spv");
    auto fragCode = readFile(std::string(SHADER_DIR) + "/crosshair.frag.spv");
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
    ds.sType           = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blendAtt;

    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(float); // aspect ratio

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pcRange;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &crosshairPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create crosshair pipeline layout");

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
    pipeInfo.layout              = crosshairPipelineLayout;
    pipeInfo.renderPass          = renderPass;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &crosshairPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create crosshair pipeline");

    vkDestroyShaderModule(device, fragMod, nullptr);
    vkDestroyShaderModule(device, vertMod, nullptr);
    std::cout << "Crosshair pipeline created\n";
}

// ---------------------------------------------------------------------------
// Memory / buffer helpers
// ---------------------------------------------------------------------------

uint32_t VulkanApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) const {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        if ((typeFilter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    throw std::runtime_error("Failed to find suitable memory type");
}

void VulkanApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                     VkMemoryPropertyFlags props,
                                     VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo info{};
    info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size        = size;
    info.usage       = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &info, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, buffer, &req);

    VkMemoryAllocateInfo alloc{};
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = req.size;
    alloc.memoryTypeIndex = findMemoryType(req.memoryTypeBits, props);

    if (vkAllocateMemory(device, &alloc, nullptr, &memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory");

    vkBindBufferMemory(device, buffer, memory, 0);
}

VkCommandBuffer VulkanApplication::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo alloc{};
    alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandPool        = commandPool;
    alloc.commandBufferCount = 1;

    VkCommandBuffer cb;
    vkAllocateCommandBuffers(device, &alloc, &cb);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cb, &begin);
    return cb;
}

void VulkanApplication::endSingleTimeCommands(VkCommandBuffer cb) {
    vkEndCommandBuffer(cb);

    VkSubmitInfo submit{};
    submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers    = &cb;

    vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cb);
}

void VulkanApplication::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBuffer cb = beginSingleTimeCommands();
    VkBufferCopy region{};
    region.size = size;
    vkCmdCopyBuffer(cb, src, dst, 1, &region);
    endSingleTimeCommands(cb);
}

// ---------------------------------------------------------------------------
// Image helper
// ---------------------------------------------------------------------------

void VulkanApplication::createImage(uint32_t width, uint32_t height, VkFormat format,
                                    VkImageTiling tiling, VkImageUsageFlags usage,
                                    VkMemoryPropertyFlags props,
                                    VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType     = VK_IMAGE_TYPE_2D;
    info.extent        = { width, height, 1 };
    info.mipLevels     = 1;
    info.arrayLayers   = 1;
    info.format        = format;
    info.tiling        = tiling;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage         = usage;
    info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    info.samples       = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device, &info, nullptr, &image) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image");

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, image, &req);

    VkMemoryAllocateInfo alloc{};
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = req.size;
    alloc.memoryTypeIndex = findMemoryType(req.memoryTypeBits, props);

    if (vkAllocateMemory(device, &alloc, nullptr, &memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate image memory");

    vkBindImageMemory(device, image, memory, 0);
}

// ---------------------------------------------------------------------------
// Depth buffer
// ---------------------------------------------------------------------------

VkFormat VulkanApplication::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                                  VkImageTiling tiling,
                                                  VkFormatFeatureFlags features) const {
    for (VkFormat fmt : candidates) {
        VkFormatProperties p;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, fmt, &p);
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (p.optimalTilingFeatures & features) == features)
            return fmt;
        if (tiling == VK_IMAGE_TILING_LINEAR  && (p.linearTilingFeatures  & features) == features)
            return fmt;
    }
    throw std::runtime_error("Failed to find supported format");
}

VkFormat VulkanApplication::findDepthFormat() const {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void VulkanApplication::createDepthResources() {
    VkFormat fmt = findDepthFormat();
    createImage(swapChainExtent.width, swapChainExtent.height,
                fmt, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthImage, depthImageMemory);
    depthImageView = createImageViewHelper(depthImage, fmt, VK_IMAGE_ASPECT_DEPTH_BIT);
    std::cout << "Depth resources created\n";
}

// ---------------------------------------------------------------------------
// Framebuffers
// ---------------------------------------------------------------------------

void VulkanApplication::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> att = { swapChainImageViews[i], depthImageView };

        VkFramebufferCreateInfo info{};
        info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass      = renderPass;
        info.attachmentCount = static_cast<uint32_t>(att.size());
        info.pAttachments    = att.data();
        info.width           = swapChainExtent.width;
        info.height          = swapChainExtent.height;
        info.layers          = 1;

        if (vkCreateFramebuffer(device, &info, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer " + std::to_string(i));
    }
    std::cout << "Framebuffers created: " << swapChainFramebuffers.size() << "\n";
}

// ---------------------------------------------------------------------------
// Command pool
// ---------------------------------------------------------------------------

void VulkanApplication::createCommandPool() {
    QueueFamilyIndices qfi = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = qfi.graphicsFamily.value();

    if (vkCreateCommandPool(device, &info, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool");
    std::cout << "Command pool created\n";
}

// ---------------------------------------------------------------------------
// Texture
// ---------------------------------------------------------------------------

void VulkanApplication::transitionImageLayout(VkImage image, VkFormat /*format*/,
                                              VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer cb = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags srcStage, dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("Unsupported image layout transition");
    }

    vkCmdPipelineBarrier(cb, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(cb);
}

void VulkanApplication::copyBufferToImage(VkBuffer buffer, VkImage image,
                                          uint32_t width, uint32_t height) {
    VkCommandBuffer cb = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = { 0, 0, 0 };
    region.imageExtent                     = { width, height, 1 };

    vkCmdCopyBufferToImage(cb, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    endSingleTimeCommands(cb);
}

void VulkanApplication::createTextureImage() {
    int      texW = 0, texH = 0, texCh = 0;
    stbi_uc* pixels   = nullptr;
    bool     usedStbi = false;

    if (!texturePath.empty()) {
        pixels = stbi_load(texturePath.c_str(), &texW, &texH, &texCh, STBI_rgb_alpha);
        if (pixels)
            usedStbi = true;
        else
            std::cout << "Warning: stb_image could not load '" << texturePath << "' — using white fallback\n";
    }

    // 1×1 opaque white fallback when no texture is available
    uint32_t white = 0xFFFFFFFFu;
    if (!pixels) {
        texW = texH = 1;
        pixels = reinterpret_cast<stbi_uc*>(&white);
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texW * texH * 4);

    VkBuffer staging; VkDeviceMemory stagingMem;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging, stagingMem);

    void* data;
    vkMapMemory(device, stagingMem, 0, imageSize, 0, &data);
    std::memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingMem);

    if (usedStbi) stbi_image_free(pixels);

    createImage(static_cast<uint32_t>(texW), static_cast<uint32_t>(texH),
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                textureImage, textureImageMemory);

    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(staging, textureImage,
                      static_cast<uint32_t>(texW), static_cast<uint32_t>(texH));
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);
    std::cout << "Texture image created (" << texW << "x" << texH << ")\n";
}

void VulkanApplication::createTextureImageView() {
    textureImageView = createImageViewHelper(
        textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    std::cout << "Texture image view created\n";
}

void VulkanApplication::createTextureSampler() {
    VkSamplerCreateInfo info{};
    info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter               = VK_FILTER_LINEAR;
    info.minFilter               = VK_FILTER_LINEAR;
    info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable        = VK_FALSE;
    info.maxAnisotropy           = 1.0f;
    info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable           = VK_FALSE;
    info.compareOp               = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipLodBias              = 0.0f;
    info.minLod                  = 0.0f;
    info.maxLod                  = 0.0f;

    if (vkCreateSampler(device, &info, nullptr, &textureSampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler");
    std::cout << "Texture sampler created\n";
}

// ---------------------------------------------------------------------------
// Asteroids
// ---------------------------------------------------------------------------

void VulkanApplication::spawnAsteroids() {
    std::mt19937 rng(42); // fixed seed — consistent layout every run
    std::uniform_real_distribution<float> posXZ(-10.0f, 10.0f);
    std::uniform_real_distribution<float> posY(-2.5f,   2.5f);
    std::uniform_real_distribution<float> axis(-1.0f,   1.0f);
    std::uniform_real_distribution<float> speed(15.0f,  60.0f);
    std::uniform_real_distribution<float> angle(0.0f,  360.0f);

    asteroids.reserve(12);
    while (static_cast<int>(asteroids.size()) < 12) {
        glm::vec3 pos(posXZ(rng), posY(rng), posXZ(rng));
        if (glm::length(pos) < 2.5f) continue; // keep clear of the origin

        Asteroid a;
        a.position = pos;
        a.rotAxis  = glm::normalize(glm::vec3(axis(rng), axis(rng), axis(rng)));
        a.rotAngle = angle(rng);
        a.rotSpeed = speed(rng);
        asteroids.push_back(a);
    }
    std::cout << "Spawned " << asteroids.size() << " asteroids\n";
}

void VulkanApplication::updateAsteroids(float dt) {
    for (auto& a : asteroids)
        a.rotAngle += a.rotSpeed * dt;
}

glm::mat4 VulkanApplication::playerModelMatrix() const {
    glm::vec3 fwd    = aimDirection;
    glm::vec3 up_ref = (std::abs(glm::dot(fwd, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.99f)
                       ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 col0   = glm::normalize(glm::cross(fwd, up_ref));
    glm::vec3 col2   = glm::cross(col0, fwd);
    glm::mat4 rot(1.0f);
    rot[0] = glm::vec4(col0, 0.0f);
    rot[1] = glm::vec4(fwd,  0.0f);
    rot[2] = glm::vec4(col2, 0.0f);
    // Slim the body and elongate the axis so the tip extends clearly forward
    // and the silhouette reads as a rocket/missile from the TPS camera angle.
    glm::mat4 shape = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 2.0f, 0.5f));
    return glm::translate(glm::mat4(1.0f), playerPosition) * rot * shape;
}

// Step 1 of two-step aim: cast the camera center ray against every live asteroid
// (each is a sphere of radius 0.5 centered at asteroid.position after the 0.5 scale).
// Returns the nearest hit point, or a far default if nothing is hit.
glm::vec3 VulkanApplication::findCrosshairTarget() const {
    constexpr float kFar    = 500.0f;
    constexpr float kRadius = 0.5f;
    float nearestT = kFar;

    for (const auto& a : asteroids) {
        if (!a.alive) continue;
        glm::vec3 oc   = cameraPosition - a.position;
        float     proj = glm::dot(oc, aimDirection);
        float     disc = proj * proj - (glm::dot(oc, oc) - kRadius * kRadius);
        if (disc < 0.0f) continue;
        float t = -proj - std::sqrt(disc);
        if (t > 0.0f && t < nearestT)
            nearestT = t;
    }

    return cameraPosition + aimDirection * nearestT;
}

void VulkanApplication::checkCollisions() {
    static const glm::vec3 kLocalMin(-1.0f, -1.0f, -1.0f);
    static const glm::vec3 kLocalMax( 1.0f,  1.0f,  1.0f);

    AABB playerAABB = transformAABB(kLocalMin, kLocalMax, playerModelMatrix());

    for (size_t i = 0; i < asteroids.size(); ++i) {
        if (!asteroids[i].alive) continue;

        AABB ai = transformAABB(kLocalMin, kLocalMax, asteroids[i].modelMatrix());

        if (ai.intersects(playerAABB)) {
            asteroids[i].alive = false;
            continue;
        }

        for (size_t j = i + 1; j < asteroids.size(); ++j) {
            if (!asteroids[j].alive) continue;
            AABB aj = transformAABB(kLocalMin, kLocalMax, asteroids[j].modelMatrix());
            if (ai.intersects(aj)) {
                asteroids[i].alive = false;
                asteroids[j].alive = false;
            }
        }
    }

    // Bullets vs asteroids
    static const glm::vec3 kBulletHalf(0.12f);
    for (auto& bullet : bullets) {
        if (!bullet.alive) continue;
        AABB ba{ bullet.position - kBulletHalf, bullet.position + kBulletHalf };
        for (auto& asteroid : asteroids) {
            if (!asteroid.alive) continue;
            AABB aa = transformAABB(kLocalMin, kLocalMax, asteroid.modelMatrix());
            if (ba.intersects(aa)) {
                bullet.alive   = false;
                asteroid.alive = false;
                break;
            }
        }
    }
}

void VulkanApplication::updateBullets(float dt) {
    for (auto& b : bullets) {
        if (!b.alive) continue;
        b.position += b.direction * b.speed * dt;
        b.lifetime -= dt;
        if (b.lifetime <= 0.0f) b.alive = false;
    }
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
        [](const Bullet& b) { return !b.alive; }), bullets.end());
}

void VulkanApplication::processPlayerInput(float dt) {
    glm::vec3 fwd   = aimDirection;
    glm::vec3 right = glm::cross(fwd, glm::vec3(0.0f, 1.0f, 0.0f));
    float     rlen  = glm::length(right);
    if (rlen > 0.001f) right /= rlen;
    else right = glm::vec3(1.0f, 0.0f, 0.0f);

    float speed = camera.moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_W)            == GLFW_PRESS) playerPosition += fwd   * speed * dt;
    if (glfwGetKey(window, GLFW_KEY_S)            == GLFW_PRESS) playerPosition -= fwd   * speed * dt;
    if (glfwGetKey(window, GLFW_KEY_A)            == GLFW_PRESS) playerPosition -= right * speed * dt;
    if (glfwGetKey(window, GLFW_KEY_D)            == GLFW_PRESS) playerPosition += right * speed * dt;
    if (glfwGetKey(window, GLFW_KEY_SPACE)        == GLFW_PRESS) playerPosition.y += speed * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) playerPosition.y -= speed * dt;
}

void VulkanApplication::createAsteroidBuffers() {
    // Vertex buffer
    {
        VkDeviceSize size = sizeof(Vertex) * asteroidMesh.vertices.size();
        VkBuffer staging; VkDeviceMemory stagingMem;
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     staging, stagingMem);
        void* data;
        vkMapMemory(device, stagingMem, 0, size, 0, &data);
        std::memcpy(data, asteroidMesh.vertices.data(), static_cast<size_t>(size));
        vkUnmapMemory(device, stagingMem);
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     asteroidVertexBuffer, asteroidVertexBufferMemory);
        copyBuffer(staging, asteroidVertexBuffer, size);
        vkDestroyBuffer(device, staging, nullptr);
        vkFreeMemory(device, stagingMem, nullptr);
    }
    // Index buffer
    {
        VkDeviceSize size = sizeof(uint32_t) * asteroidMesh.indices.size();
        VkBuffer staging; VkDeviceMemory stagingMem;
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     staging, stagingMem);
        void* data;
        vkMapMemory(device, stagingMem, 0, size, 0, &data);
        std::memcpy(data, asteroidMesh.indices.data(), static_cast<size_t>(size));
        vkUnmapMemory(device, stagingMem);
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     asteroidIndexBuffer, asteroidIndexBufferMemory);
        copyBuffer(staging, asteroidIndexBuffer, size);
        vkDestroyBuffer(device, staging, nullptr);
        vkFreeMemory(device, stagingMem, nullptr);
    }
    std::cout << "Asteroid GPU buffers created\n";
}

// ---------------------------------------------------------------------------
// Geometry buffers
// ---------------------------------------------------------------------------

void VulkanApplication::createVertexBuffer() {
    VkDeviceSize size = sizeof(Vertex) * mesh.vertices.size();

    VkBuffer staging; VkDeviceMemory stagingMem;
    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging, stagingMem);

    void* data;
    vkMapMemory(device, stagingMem, 0, size, 0, &data);
    std::memcpy(data, mesh.vertices.data(), static_cast<size_t>(size));
    vkUnmapMemory(device, stagingMem);

    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    copyBuffer(staging, vertexBuffer, size);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);
    std::cout << "Vertex buffer created\n";
}

void VulkanApplication::createIndexBuffer() {
    VkDeviceSize size = sizeof(uint32_t) * mesh.indices.size();

    VkBuffer staging; VkDeviceMemory stagingMem;
    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging, stagingMem);

    void* data;
    vkMapMemory(device, stagingMem, 0, size, 0, &data);
    std::memcpy(data, mesh.indices.data(), static_cast<size_t>(size));
    vkUnmapMemory(device, stagingMem);

    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    copyBuffer(staging, indexBuffer, size);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);
    std::cout << "Index buffer created\n";
}

// ---------------------------------------------------------------------------
// Uniform buffer
// ---------------------------------------------------------------------------

void VulkanApplication::createUniformBuffer() {
    VkDeviceSize size = sizeof(UniformBufferObject);
    createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffer, uniformBufferMemory);
    vkMapMemory(device, uniformBufferMemory, 0, size, 0, &uniformBufferMapped);
    std::cout << "Uniform buffer created\n";
}

// ---------------------------------------------------------------------------
// Descriptors
// ---------------------------------------------------------------------------

void VulkanApplication::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> pools{};
    pools[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pools[0].descriptorCount = 1;
    pools[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pools[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = static_cast<uint32_t>(pools.size());
    info.pPoolSizes    = pools.data();
    info.maxSets       = 1;

    if (vkCreateDescriptorPool(device, &info, nullptr, &descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool");
    std::cout << "Descriptor pool created\n";
}

void VulkanApplication::createDescriptorSet() {
    VkDescriptorSetAllocateInfo alloc{};
    alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc.descriptorPool     = descriptorPool;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts        = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &alloc, &descriptorSet) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor set");

    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = uniformBuffer;
    bufInfo.offset = 0;
    bufInfo.range  = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgInfo.imageView   = textureImageView;
    imgInfo.sampler     = textureSampler;

    std::array<VkWriteDescriptorSet, 2> writes{};
    writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet          = descriptorSet;
    writes[0].dstBinding      = 0;
    writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo     = &bufInfo;

    writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet          = descriptorSet;
    writes[1].dstBinding      = 1;
    writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo      = &imgInfo;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    std::cout << "Descriptor set created\n";
}

// ---------------------------------------------------------------------------
// Command buffers
// ---------------------------------------------------------------------------

void VulkanApplication::createCommandBuffers() {
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo alloc{};
    alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool        = commandPool;
    alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &alloc, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers");

    std::cout << "Command buffers allocated: " << commandBuffers.size() << "\n";
}

void VulkanApplication::recordCommandBuffer(uint32_t imageIndex) {
    VkCommandBuffer cb = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cb, &begin) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer");

    VkClearValue clears[2];
    std::memset(clears, 0, sizeof(clears));
    clears[0].color.float32[0] = 0.05f;
    clears[0].color.float32[1] = 0.05f;
    clears[0].color.float32[2] = 0.10f;
    clears[0].color.float32[3] = 1.0f;
    clears[1].depthStencil.depth   = 1.0f;
    clears[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo rp{};
    rp.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass        = renderPass;
    rp.framebuffer       = swapChainFramebuffers[imageIndex];
    rp.renderArea.extent = swapChainExtent;
    rp.clearValueCount   = 2;
    rp.pClearValues      = clears;

    vkCmdBeginRenderPass(cb, &rp, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    const VkDeviceSize zero = 0;

    constexpr VkShaderStageFlags kPushStages =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // --- Player (cone, blue) ---
    PushConstants playerPC{};
    playerPC.model     = playerModelMatrix();
    playerPC.baseColor = glm::vec4(0.20f, 0.45f, 1.00f, 1.0f);
    vkCmdPushConstants(cb, pipelineLayout, kPushStages, 0, sizeof(PushConstants), &playerPC);
    vkCmdBindVertexBuffers(cb, 0, 1, &vertexBuffer, &zero);
    vkCmdBindIndexBuffer(cb, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cb, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);

    // --- Asteroids (spheres, orange) ---
    PushConstants asteroidPC{};
    asteroidPC.baseColor = glm::vec4(1.00f, 0.50f, 0.08f, 1.0f);
    vkCmdBindVertexBuffers(cb, 0, 1, &asteroidVertexBuffer, &zero);
    vkCmdBindIndexBuffer(cb, asteroidIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    for (const auto& asteroid : asteroids) {
        if (!asteroid.alive) continue;
        asteroidPC.model = asteroid.modelMatrix();
        vkCmdPushConstants(cb, pipelineLayout, kPushStages, 0, sizeof(PushConstants), &asteroidPC);
        vkCmdDrawIndexed(cb, static_cast<uint32_t>(asteroidMesh.indices.size()), 1, 0, 0, 0);
    }

    // --- Bullets (yellow spheres, reuse asteroid mesh) ---
    PushConstants bulletPC{};
    bulletPC.baseColor = glm::vec4(1.0f, 0.95f, 0.1f, 1.0f);
    for (const auto& bullet : bullets) {
        if (!bullet.alive) continue;
        bulletPC.model = glm::scale(glm::translate(glm::mat4(1.0f), bullet.position),
                                    glm::vec3(0.12f));
        vkCmdPushConstants(cb, pipelineLayout, kPushStages, 0, sizeof(PushConstants), &bulletPC);
        vkCmdDrawIndexed(cb, static_cast<uint32_t>(asteroidMesh.indices.size()), 1, 0, 0, 0);
    }

    // --- Crosshair (2D overlay, depth-test disabled) ---
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, crosshairPipeline);
    float aspect = static_cast<float>(swapChainExtent.width) /
                   static_cast<float>(swapChainExtent.height);
    vkCmdPushConstants(cb, crosshairPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(float), &aspect);
    vkCmdDraw(cb, 12, 1, 0, 0);

    vkCmdEndRenderPass(cb);
    if (vkEndCommandBuffer(cb) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer");
}

// ---------------------------------------------------------------------------
// Sync objects
// ---------------------------------------------------------------------------

void VulkanApplication::createSyncObjects() {
    VkSemaphoreCreateInfo sem{};
    sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence{};
    fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &sem, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &sem, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fence, nullptr, &inFlightFence) != VK_SUCCESS)
        throw std::runtime_error("Failed to create sync objects");
    std::cout << "Sync objects created\n";
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void VulkanApplication::updateUniformBuffer() {
    glm::vec3 fwd        = camera.forward();
    glm::vec3 lookTarget = playerPosition + fwd * 18.0f;
    // cameraPosition and aimDirection are already current (set in mainLoop before drawFrame).

    UniformBufferObject ubo{};
    ubo.view = glm::lookAt(cameraPosition, lookTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(60.0f),
                                static_cast<float>(swapChainExtent.width) / swapChainExtent.height,
                                0.1f, 100.0f);
    ubo.proj[1][1] *= -1.0f;

    std::memcpy(uniformBufferMapped, &ubo, sizeof(ubo));
}

// ---------------------------------------------------------------------------
// Per-frame render
// ---------------------------------------------------------------------------

void VulkanApplication::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                          imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    updateUniformBuffer();

    vkResetCommandBuffer(commandBuffers[imageIndex], 0);
    recordCommandBuffer(imageIndex);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &imageAvailableSemaphore;
    submit.pWaitDstStageMask    = &waitStage;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &commandBuffers[imageIndex];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &renderFinishedSemaphore;

    if (vkQueueSubmit(graphicsQueue, 1, &submit, inFlightFence) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command");

    VkPresentInfoKHR present{};
    present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores    = &renderFinishedSemaphore;
    present.swapchainCount     = 1;
    present.pSwapchains        = &swapChain;
    present.pImageIndices      = &imageIndex;

    vkQueuePresentKHR(presentQueue, &present);
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

void VulkanApplication::mainLoop() {
    const double targetFrameTime =
        (config.targetFPS > 0) ? (1.0 / config.targetFPS) : 0.0;

    lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double frameStart = glfwGetTime();
        deltaTime         = static_cast<float>(frameStart - lastFrameTime);
        lastFrameTime     = frameStart;

        glfwPollEvents();

        // Recompute camera state before any game logic so all systems use the same frame's value.
        {
            glm::vec3 fwd   = camera.forward();
            glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0.0f, 1.0f, 0.0f)));
            cameraPosition = playerPosition - fwd * 7.0f + right * 5.0f + glm::vec3(0.0f, 4.0f, 0.0f);
            glm::vec3 lookTarget = playerPosition + fwd * 18.0f;
            aimDirection = glm::normalize(lookTarget - cameraPosition);
        }

        processPlayerInput(deltaTime);
        if (pendingShoot) {
            Bullet b;
            // Step 2: spawn at cone tip, aim at the world point found by the raycast.
            b.position  = playerPosition + aimDirection * 2.0f;
            b.direction = glm::normalize(findCrosshairTarget() - b.position);
            bullets.push_back(b);
            pendingShoot = false;
        }
        updateBullets(deltaTime);
        updateAsteroids(deltaTime);
        checkCollisions();
        drawFrame();

        if (targetFrameTime > 0.0) {
            double elapsed   = glfwGetTime() - frameStart;
            double remaining = targetFrameTime - elapsed;
            // Sleep most of the wait, then busy-spin the last 2 ms for precision
            if (remaining > 0.002)
                std::this_thread::sleep_for(
                    std::chrono::duration<double>(remaining - 0.002));
            while (glfwGetTime() - frameStart < targetFrameTime) {}
        }
    }

    vkDeviceWaitIdle(device);
}

// ---------------------------------------------------------------------------
// Cleanup — reverse creation order
// ---------------------------------------------------------------------------

void VulkanApplication::cleanup() {
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    for (auto fb : swapChainFramebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    vkDestroyPipeline(device, crosshairPipeline, nullptr);
    vkDestroyPipelineLayout(device, crosshairPipelineLayout, nullptr);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    vkUnmapMemory(device, uniformBufferMemory);
    vkDestroyBuffer(device, uniformBuffer, nullptr);
    vkFreeMemory(device, uniformBufferMemory, nullptr);
    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);
    vkDestroyBuffer(device, asteroidIndexBuffer, nullptr);
    vkFreeMemory(device, asteroidIndexBufferMemory, nullptr);
    vkDestroyBuffer(device, asteroidVertexBuffer, nullptr);
    vkFreeMemory(device, asteroidVertexBufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    for (auto v : swapChainImageViews) vkDestroyImageView(device, v, nullptr);
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "Cleanup completed\n";
}
