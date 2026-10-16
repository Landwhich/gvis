#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <vulkan_shared.h>

namespace gv = GVIS;

class Swapchain{
    vk::raii::CommandPool                   commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer>    commandBuffers;

    std::vector<uint32_t>                   indices;
    vk::raii::Buffer                        vertexBuffer       = nullptr;
    vk::raii::DeviceMemory                  vertexBufferMemory = nullptr;
    vk::raii::Buffer                        indexBuffer        = nullptr;
    vk::raii::DeviceMemory                  indexBufferMemory  = nullptr;

    vk::raii::Image                         textureImage = nullptr;
    vk::raii::DeviceMemory                  textureImageMemory = nullptr;

    vk::raii::Image                         depthImage = nullptr;
    vk::raii::DeviceMemory                  depthImageMemory = nullptr;
    vk::raii::ImageView                     depthImageView = nullptr;
    vk::raii::ImageView                     textureImageView = nullptr;   
    vk::raii::Sampler                       textureSampler = nullptr;

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
     * Query window surface from platform object,
     * and create vulkan surface object from that
     */
    bool createSurface();

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

    /*
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
       
    void framebufferResizeCallback(int width, int height); 
    //vulkan/vulkandevice.cpp
    // * * * * * * * * * * * * 
    void getRequiredInstanceExtensions();

    //renderer/pipelineResources.cpp
    // * * * * * * * * * * * * 
    void transition_image_layout(
        vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, 
        vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, 
        vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask,
        vk::ImageAspectFlags image_aspect_flags
    );
    // queries for first supported format in a list of candidates
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
    vk::raii::ImageView createImageView(
            vk::Image const &image, vk::Format format, vk::ImageAspectFlags aspectFlags, 
            vk::raii::Device& device, uint32_t mipLevels);

};
