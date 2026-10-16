#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include "resource_manager.hpp"
#include "swapchain.hpp"
#include "vulkan_shared.h"

namespace gv = GVIS;

struct Texture : public Resource{

    explicit Texture(const std::string& id) : Resource(id) {};
    ~Texture() override { Unload(); } 

    bool doLoad(const std::pair<gv::DeviceContext, 
                                gv::GraphicsContext>& resourceContext) override;
    void doUnload() override;

  private:
    std::string                 name; 

    vk::raii::Image             textureImage = nullptr;
    vk::raii::DeviceMemory      textureImageMemory = nullptr;
//    vk::DeviceSize              offset;
    vk::raii::ImageView         textureImageView = nullptr;   
    vk::raii::Sampler           textureSampler = nullptr;

//    int width = 0;              // Image width in pixels
//    int height = 0;             // Image height in pixels
//    int channels = 0;           // Number of color channels (RGB=3, RGBA=4, etc.)   
     
    
    /*
     * Textures should always get stored in GPU granted they aren't getting 
     * frequently updated (they shouldn't be), and we have the vram for them
     * To do this images are staged in host and then sent to GPU
     */ 
    void createTextureImage(
        const vk::raii::Device& device, 
        const vk::raii::PhysicalDevice& physicalDevice,
        const vk::raii::Queue& queue,
        const vk::raii::CommandPool& commandPool);
    
    void createTextureImageView(
        const vk::raii::Device& device);
    
    /*
     * Sampler enables filters and transforms for the image before
     * passing texels to shader
     * Is independant of images and image views and can be used across multiple
     */
    void createTextureSampler(
        const vk::raii::Device& device, 
        const vk::raii::PhysicalDevice& physicalDevice);
};
