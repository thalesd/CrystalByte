#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <optional>
#include <string>

#include "Types.h"
#include "Camera.h"
#include "Mesh.h"
#include "LaunchSettings.h"

// ---------------------------------------------------------------------------
// Internal Vulkan helper types
// ---------------------------------------------------------------------------

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

// ---------------------------------------------------------------------------
// Application
// ---------------------------------------------------------------------------

class VulkanApplication {
public:
    void run(const std::string& modelPath, const LaunchConfig& cfg);

private:
    LaunchConfig config;

    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // --- Window ---
    GLFWwindow* window = nullptr;

    // --- Core Vulkan ---
    VkInstance       instance       = VK_NULL_HANDLE;
    VkSurfaceKHR     surface        = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice         device         = VK_NULL_HANDLE;
    VkQueue          graphicsQueue  = VK_NULL_HANDLE;
    VkQueue          presentQueue   = VK_NULL_HANDLE;

    // --- Swap chain ---
    VkSwapchainKHR           swapChain            = VK_NULL_HANDLE;
    std::vector<VkImage>     swapChainImages;
    VkFormat                 swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D               swapChainExtent      = {};
    std::vector<VkImageView> swapChainImageViews;

    // --- Pipeline ---
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkRenderPass          renderPass          = VK_NULL_HANDLE;
    VkPipelineLayout      pipelineLayout      = VK_NULL_HANDLE;
    VkPipeline            graphicsPipeline    = VK_NULL_HANDLE;

    // --- Depth buffer ---
    VkImage        depthImage       = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView    depthImageView   = VK_NULL_HANDLE;

    // --- Framebuffers + commands ---
    std::vector<VkFramebuffer>   swapChainFramebuffers;
    VkCommandPool                commandPool   = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    // --- GPU buffers ---
    VkBuffer       vertexBuffer        = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory  = VK_NULL_HANDLE;
    VkBuffer       indexBuffer         = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory   = VK_NULL_HANDLE;
    VkBuffer       uniformBuffer       = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
    void*          uniformBufferMapped = nullptr;

    // --- Descriptors ---
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet  descriptorSet  = VK_NULL_HANDLE;

    // --- Sync ---
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence     inFlightFence           = VK_NULL_HANDLE;

    // --- Mesh ---
    Mesh mesh;

    // --- Texture ---
    std::string    texturePath;
    VkImage        textureImage       = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    VkImageView    textureImageView   = VK_NULL_HANDLE;
    VkSampler      textureSampler     = VK_NULL_HANDLE;

    // --- Object rotation ---
    float objectRotX = 0.0f;
    float objectRotY = 0.0f;

    // --- Camera + timing ---
    Camera camera;
    double lastFrameTime = 0.0;
    float  deltaTime     = 0.0f;

    // --- Mouse drag state ---
    bool   lmbHeld         = false;
    bool   rmbHeld         = false;
    bool   firstMouseSample = true;
    double prevMouseX      = 0.0;
    double prevMouseY      = 0.0;

    // -----------------------------------------------------------------------
    // Methods
    // -----------------------------------------------------------------------

    // Window
    void initWindow();
    void setupCallbacks();
    void onMouseMove(double xpos, double ypos);
    void onMouseButton(int button, int action, int mods);

    // Vulkan init
    void initVulkan();
    void createInstance();
    void createSurface();

    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createDescriptorSetLayout();
    void createRenderPass();
    void createGraphicsPipeline();
    void createDepthResources();
    void createFramebuffers();
    void createCommandPool();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffer();
    void createDescriptorPool();
    void createDescriptorSet();
    void createCommandBuffers();
    void createSyncObjects();

    // Vulkan helpers
    QueueFamilyIndices      findQueueFamilies(VkPhysicalDevice dev) const;
    bool                    checkDeviceExtensionSupport(VkPhysicalDevice dev) const;
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev) const;
    bool                    isDeviceSuitable(VkPhysicalDevice dev) const;
    VkSurfaceFormatKHR      chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
    VkPresentModeKHR        chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) const;
    VkExtent2D              chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps) const;
    VkImageView             createImageViewHelper(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkShaderModule          createShaderModule(const std::vector<char>& code);
    uint32_t                findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    void                    createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                         VkMemoryPropertyFlags properties,
                                         VkBuffer& buffer, VkDeviceMemory& memory);
    VkCommandBuffer         beginSingleTimeCommands();
    void                    endSingleTimeCommands(VkCommandBuffer cb);
    void                    copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    void                    createImage(uint32_t width, uint32_t height, VkFormat format,
                                        VkImageTiling tiling, VkImageUsageFlags usage,
                                        VkMemoryPropertyFlags properties,
                                        VkImage& image, VkDeviceMemory& memory);
    VkFormat                findSupportedFormat(const std::vector<VkFormat>& candidates,
                                                VkImageTiling tiling,
                                                VkFormatFeatureFlags features) const;
    VkFormat                findDepthFormat() const;
    static std::vector<char> readFile(const std::string& path);

    // Per-frame
    void updateUniformBuffer();
    void drawFrame();
    void mainLoop();
    void cleanup();
};
