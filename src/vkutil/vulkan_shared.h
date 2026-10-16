#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <vector>
#include <array>

namespace GVIS {

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
const std::string  MODEL_PATH           = "resources/models/viking_room.obj";
const std::string  TEXTURE_PATH         = "resources/textures/viking_room.png";

static constexpr std::array<const char*, 1> VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
}; 
static std::vector<const char*> REQUIRED_DEVICE_EXT = {
        vk::KHRSwapchainExtensionName
};

#ifdef NDEBUG
static constexpr bool VALIDATION_LAYERS_ENABLED = false;
#else
static constexpr bool VALIDATION_LAYERS_ENABLED = true;
#endif

struct DeviceContext {
    const vk::raii::Device&             device;
    const vk::raii::PhysicalDevice&     physicalDevice;
    const vk::raii::Queue&              queue;
};

struct GraphicsContext {
    const vk::raii::CommandPool&        commandPool;
};

namespace vkutil {

void EndSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer, 
        const vk::raii::Queue& queue);
vk::raii::CommandBuffer BeginSingleTimeCommands(
        const vk::raii::CommandPool& commandPool, const vk::raii::Device& device);

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(
    // the following are apart of gvis graphics context
    const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice,
    vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties
);

// shaders could be used for this but better to use images in vulkan as they 
// contain simpler 2D texels
std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(
    const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice,
    uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, 
    vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties 
);

void CopyBufferToImage(vk::raii::CommandBuffer &commandBuffer, 
        const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, 
        uint32_t height);

 // Uses a lot of the same logic as createImageViews, worth having in
 // its own function handy for textures and other images
vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const &image, 
        vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);

void TransitionImageLayout(
        vk::raii::CommandBuffer &commandBuffer, const vk::raii::Image &image, 
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);

uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, 
        const vk::raii::PhysicalDevice& physicalDevice);

}} // GVIS::vkutil
