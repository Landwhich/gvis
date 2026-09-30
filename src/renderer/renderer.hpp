#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include "vulkan_shared.h"
#include "platform.hpp"

namespace gv = GVIS;

class Renderer{
    Platform*                               platform = nullptr;

    //vulkan/vulkanDevice.cpp
    // * * * * * * * *
    vk::raii::Context                       context; 
    vk::raii::Instance                      instance = nullptr; 
    vk::raii::SurfaceKHR                    surface = nullptr; 
    vk::raii::PhysicalDevice                physicalDevice = nullptr;
    vk::raii::Device                        device = nullptr;
    vk::raii::Queue                         queue = nullptr;
    //TODO: duplicate queue arch to provide optimized queueing for both
    //graphics and transfer ops
    uint32_t                                queueIndex = ~0;
    std::vector<vk::raii::Semaphore>        presentCompleteSemaphores; 
    std::vector<vk::raii::Semaphore>        renderFinishedSemaphores; 
    std::vector<vk::raii::Fence>            inFlightFences; 
    //renderer/rendererCore.cpp
    // * * * * * * * *
    vk::Extent2D                            swapChainExtent;
    vk::SurfaceFormatKHR                    swapChainSurfaceFormat;
    vk::raii::SwapchainKHR                  swapChain = nullptr;
    std::vector<vk::Image>                  swapChainImages;
    std::vector<vk::raii::ImageView>        swapChainImageViews;
    //renderer/rendererPipeline.cpp
    // * * * * * * * *
    vk::raii::PipelineLayout                pipelineLayout = nullptr;
    vk::raii::Pipeline                      graphicsPipeline = nullptr; 
    //renderer/rendererResource.cpp
    // * * * * * * * *
    vk::raii::CommandPool                   commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer>    commandBuffers;
    vk::raii::DescriptorSetLayout           descriptorSetLayout = nullptr;
    vk::raii::DescriptorPool                descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet>    descriptorSets;
    vk::raii::Buffer                        vertexBuffer       = nullptr;
    vk::raii::DeviceMemory                  vertexBufferMemory = nullptr;
    vk::raii::Buffer                        indexBuffer        = nullptr;
    vk::raii::DeviceMemory                  indexBufferMemory  = nullptr;

    vk::raii::Image                         textureImage = nullptr;
    vk::raii::DeviceMemory                  textureImageMemory = nullptr;
    vk::raii::ImageView                     textureImageView = nullptr;
    vk::raii::Sampler                       textureSampler = nullptr;

    vk::raii::Image                         depthImage = nullptr;
    vk::raii::DeviceMemory                  depthImageMemory = nullptr;
    vk::raii::ImageView                     depthImageView = nullptr;

    std::vector<vk::raii::Buffer>           uniformBuffers;
    std::vector<vk::raii::DeviceMemory>     uniformBuffersMemory;
    std::vector<void *>                     uniformBuffersMapped;
    uint32_t                                frameIndex = 0;
    bool                                    framebufferResized = false;
    // for multi device instances, we want the option to 
    // use differing layers and extensions
    std::vector<const char*>                validationlayers{};
    std::vector<const char*>                requiredExtensions{}; 
    // NOTE: research decl order in class affecting delete order
    vk::raii::DebugUtilsMessengerEXT        debugMessenger = nullptr;
public:

    void DrawFrame();
    void Initialize(Platform* platform);
    void IdleDevice();
    void Cleanup();

private:

    // * * * * * * * * * * * * 
    //
    //vulkan/vulkanDevice.cpp
    //
    // * * * * * * * * * * * * 
    /*
     * Describe the instance with (name and version)
     * get extensions needed and ensure they are supported
     * + same for layers
     */
    void createVulkanInstance();

    void setupDebugMessenger();

    /*
     * Finds a device which supports:
     * - required queue families
     * - the needed device EXTs
     * - needed features
     * should expand this to optimize device on higher-end PCs
     */
    void pickPhysicalDevice();

    /*
     * Query window surface from platform object,
     * and create vulkan surface object from that
     */
    bool createSurface();

    /*
     * Create real vulkan device a logical interface to the physical device:
     * - check for swapchain support
     * - find supporting queue given what we need (ofs render doesn't need present func)
     * - query for features and support needed within the device
     * - query for extensions
     * - finally create device and queue 
     */
    void createLogicalDevice();

    // * * * * * * * * * * * * 
    //
    //renderer/rendererCore.cpp
    //
    // * * * * * * * * * * * * 
    /*
     * vulkan has no 'framebuffer' as it owns the buffer(s) directly  
     * we need to query for explicit surface support as well on top of generic support
     *      for surface formats, capabilities, and modes
     * - capabilities store thigs like amount of images that can be stored in swapchain, 
     *   width and height of images etc...
     * - formats store things like color space and pixel format
     * - modes dictate how the image will be displayed based on certain cond.s
     *   - vk::PresentModeKHR::eImmediate means display as soon as the next image is 
     *     available
     *   - vk::PresentModeKHR::eFifo (i.e. vsync) means images are in a queue and added
     *     to swapchain only when it is empty
     *   - vk::PresentModeKHR::eFifoRelaxed like vsync but plays catch up if last frame
     *     was delayed
     *   - vk::PresentModeKHR::eMailbox is like vsync, but when swapchain is full, old 
     *     frames get replaced (i.e. 'triple-buffering') 
     * - query window extent from the platform to match swapchain extent 
     */
    void createSwapChain();

    /*
     * Images are created in 'Renderer::createImageView()' 
     * reserves space in the swapchain imageViews vector and fills it with image 
     *      views (interfaces for images). 
     */
    void createImageViews();

    void cleanupSwapChain();

    void recreateSwapChain();

    // * * * * * * * * * * * * 
    //
    //renderer/rendererResources.cpp
    //
    // * * * * * * * * * * * * 
    /*
     *   
     */
    void createCommandPool();

    void createCommandBuffers();
    /*
     * create a staging buffer to copy vertex data over and then copy
     * to a vertex buffer optimized for the gpu
     */
    void createVertexBuffer();

    void createIndexBuffer();
    
    void createUniformBuffers();

    /*
     * Copies pixel data straight from buffer to vulkan image
     *
     * Textures should always get stored in GPU granted they aren't getting 
     * frequently updated (they shouldn't be), and we have the vram for them
     * To do this images are staged in host and then sent to GPU
     */ 
    void createTextureImage();

    void createTextureImageView();
    
    /*
     * Sampler enables filters and transforms for the image before
     * passing texels to shader
     * Is independant of images and image views and can be used across multiple
     */
    void createTextureSampler();
    
    void createUniformBuffers();

    void createDepthResources();

    /*
     * uses chrono to track exact time and update irrespective of frame rate
     * TODO: investigate push constants
     */
    void updateUniformBuffer(uint32_t currentImage);

    /*
     * descriptor bindings are used to transmit information from the 
     * cpp code to the shader code. this layout creation is used to feed
     * information to the pipeline in creation for the vertex shader
     */
    void createDescriptorSetLayout();

    void createDescriptorPool();

    /*
     * descriptor sets are created after the established layout passed to
     * the pipeline and are needed to bind UBO  
     */
    void createDescriptorSets();

    /*
     * transition target images' layouts for each set of incoming data, 
     * then assemble attachments and their details to be sent to render info   
     * then bind the pipeline to send attachment info
     *
     * finally reset the target image to empty and release command buffer to
     * be redrawn to
     */
    void recordCommandBuffer(uint32_t imageIndex);

    void createSyncObjects();

    // * * * * * * * * * * * * 
    //
    //renderer/rendererPipeline.cpp
    //
    // * * * * * * * * * * * * 
    /*
     * Vulkan's pipeline is fixed and immutable and so needs to be rebuilt
     * if it needs to be updated, this also means all state must be explicitly
     * set. This is excepted by select few dynamic states. 
     * 
     * Dynamic Rendering (1.3) means we don't need to create explicit render passes
     * and can instead set everything from the pipeline
     */
    void createGraphicsPipeline();
       
    void framebufferResizeCallback(int width, int height); 
    //vulkan/vulkandevice.cpp
    // * * * * * * * * * * * * 
    void getRequiredInstanceExtensions();
    bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice);

    //renderer/pipelineResources.cpp
    // * * * * * * * * * * * * 
    void transition_image_layout(
        vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, 
        vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, 
        vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask,
        vk::ImageAspectFlags image_aspect_flags
    );
    // queries for first supported format in a list of candidates
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, 
            vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::Format findDepthFormat();
    // shaders could be used for this but better to use images in vulkan as they 
    // contain simpler 2D texels
    std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(
        uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, 
        vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties 
    );


    void endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer);
    vk::raii::CommandBuffer beginSingleTimeCommands();
    // find the correct type of GPU memory based on reqs  
    uint32_t findMemoryType(
        uint32_t typeFilter, vk::MemoryPropertyFlags properties
    );
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(
        vk::DeviceSize size, 
        vk::BufferUsageFlags usage, 
        vk::MemoryPropertyFlags properties
    );
    void copyBuffer(
            vk::raii::Buffer &srcBuffer, 
            vk::raii::Buffer &dstBuffer, 
            vk::DeviceSize size
    );

    //renderer/rendererCore.cpp
    // * * * * * * * * * * * * 
    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities);
    uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities);
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes);
     // Uses a lot of the same logic as createImageViews, worth having in
     // its own function handy for textures and other images
    vk::raii::ImageView createImageView(vk::Image const &image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);

};

// * * * * * * * * * * * * 
//
// GRAPHICS PIPELINE
//
// * * * * * * * * * * * * 
//
// Input Assembler      // creates model using data from vertex buffer and often an index 
//      |               //      buffer
//      V
// Vertex Shader        // transforms model space to screen for every vertex
//      |
//      V
// Tesselation Shader   // allows subdivison of geometry based on certain rules to
//      |               //      increase mesh quality
//      V
// Geometry Shader      // like tesselation but for primitives: (e.g. triangles, lines
//      |               //      squares, etc...) not great performance and rarely used
//      V
// Rasterization        // breaks primitives into fragments (i.e. pixels) and discards 
//      |               //      pixels that fall outside screen space or fail depth test
//      V
// Fragment Shader      // for all remaining fragements, this shader determines: which 
//      |               //      framebuffer the fragement will go to, color value, and
//      |               //      depth values, based on data ( normals, coords, ...)
//      V
// Color Blending       // blending changes fragments based on previous objects like 
//                      // like how gradients and alpha work 
// * * * * * * * * * * * * 



