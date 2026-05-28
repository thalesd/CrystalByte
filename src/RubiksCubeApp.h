#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <optional>
#include <string>
#include "RubiksCube.h"
#include "UpgradeStore.h"
#include "MainMenu.h"

struct QueueFamilyIndices2 {
    std::optional<uint32_t> graphicsFamily, presentFamily;
    bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails2 {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

struct CubePushConst { glm::mat4 model; glm::vec4 color; };
struct HudPC         { glm::vec4 rect;  glm::vec4 color; float fill; };

class RubiksCubeApp {
public:
    void run(const MainMenuResult& cfg);

private:
    MainMenuResult config;

    // Vulkan core
    GLFWwindow*      window         = nullptr;
    VkInstance       instance       = VK_NULL_HANDLE;
    VkSurfaceKHR     surface        = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice         device         = VK_NULL_HANDLE;
    VkQueue          graphicsQueue  = VK_NULL_HANDLE;
    VkQueue          presentQueue   = VK_NULL_HANDLE;

    // Swap chain
    VkSwapchainKHR           swapChain       = VK_NULL_HANDLE;
    std::vector<VkImage>     swapChainImages;
    VkFormat                 swapChainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D               swapChainExtent = {};
    std::vector<VkImageView> swapChainViews;

    // Pipelines
    VkDescriptorSetLayout cubeDescLayout = VK_NULL_HANDLE;
    VkRenderPass          renderPass     = VK_NULL_HANDLE;
    VkPipelineLayout      cubePipeLayout = VK_NULL_HANDLE;
    VkPipeline            cubePipeline   = VK_NULL_HANDLE;
    VkPipelineLayout      hudPipeLayout  = VK_NULL_HANDLE;
    VkPipeline            hudPipeline    = VK_NULL_HANDLE;

    // Depth
    VkImage        depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthMem   = VK_NULL_HANDLE;
    VkImageView    depthView  = VK_NULL_HANDLE;

    // Framebuffers + commands
    std::vector<VkFramebuffer>   framebuffers;
    VkCommandPool                cmdPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> cmdBufs;

    // Cube mesh GPU buffers
    VkBuffer       cubeVB      = VK_NULL_HANDLE;
    VkDeviceMemory cubeVBMem   = VK_NULL_HANDLE;
    VkBuffer       cubeIB      = VK_NULL_HANDLE;
    VkDeviceMemory cubeIBMem   = VK_NULL_HANDLE;
    uint32_t       cubeIndexCount = 0;

    // Uniform buffer
    VkBuffer       ubo       = VK_NULL_HANDLE;
    VkDeviceMemory uboMem    = VK_NULL_HANDLE;
    void*          uboMapped = nullptr;

    // Descriptors
    VkDescriptorPool descPool = VK_NULL_HANDLE;
    VkDescriptorSet  descSet  = VK_NULL_HANDLE;

    // Sync
    VkSemaphore imgAvail   = VK_NULL_HANDLE;
    VkSemaphore renderDone = VK_NULL_HANDLE;
    VkFence     fence      = VK_NULL_HANDLE;

    // Game state
    RubiksCube   cube;
    UpgradeState upgrades;
    std::deque<CubeMove>  moveQueue;
    std::vector<CubeMove> moveHistory;

    bool      animating    = false;
    float     animTimer    = 0.0f;
    float     animDuration = 0.18f;
    CubeMove  animMove     = CubeMove::U();

    float  orbitYaw   = 0.3f;
    float  orbitPitch = 0.4f;
    float  orbitDist  = 7.0f;
    bool   mouseDown  = false;
    double prevMX = 0, prevMY = 0;

    double lastTime   = 0.0;
    float  deltaTime  = 0.0f;
    bool   shouldQuit = false;

    float commitFlash = 0.0f;

    // Init helpers
    void initWindow();
    void initVulkan();
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createCubePipeline();
    void createHudPipeline();
    void createDepthResources();
    void createFramebuffers();
    void createCommandPool();
    void createCubeBuffers();
    void createUniformBuffer();
    void createDescriptorPool();
    void createDescriptorSet();
    void createCommandBuffers();
    void createSyncObjects();
    void mainLoop();
    void drawFrame();
    void updateUniformBuffer();
    void recordCommandBuffer(uint32_t imageIndex);
    void cleanup();
    void processInput();

    QueueFamilyIndices2      findQueueFamilies(VkPhysicalDevice dev) const;
    SwapChainSupportDetails2 querySwapChainSupport(VkPhysicalDevice dev) const;
    bool                     isDeviceSuitable(VkPhysicalDevice dev) const;
    VkSurfaceFormatKHR       chooseSwapFormat(const std::vector<VkSurfaceFormatKHR>& fmts) const;
    VkPresentModeKHR         chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) const;
    VkExtent2D               chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps) const;
    VkImageView              createImageView(VkImage img, VkFormat fmt, VkImageAspectFlags aspect);
    VkShaderModule           createShaderModule(const std::vector<char>& code);
    uint32_t                 findMemoryType(uint32_t filter, VkMemoryPropertyFlags props) const;
    void                     createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                          VkMemoryPropertyFlags props,
                                          VkBuffer& buf, VkDeviceMemory& mem);
    VkCommandBuffer          beginSingleTimeCommands();
    void                     endSingleTimeCommands(VkCommandBuffer cb);
    void                     copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    VkFormat                 findDepthFormat() const;
    static std::vector<char> readFile(const std::string& path);

    glm::vec4 stickerColor(int gx, int gy, int gz, int meshFace) const;
    glm::mat4 cubieMatrix(int gx, int gy, int gz) const;
    void      queueMove(CubeMove m);
    void      commitScore();

    static void mouseBtnCb(GLFWwindow* w, int btn, int action, int mods);
    static void mouseMoveCb(GLFWwindow* w, double x, double y);
    static void keyCb(GLFWwindow* w, int key, int sc, int action, int mods);
};
