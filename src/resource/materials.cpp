#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "materials.hpp"

bool Texture::doLoad(const std::pair<gv::DeviceContext, gv::GraphicsContext>& resourceContext) {
    createTextureImage(
            resourceContext.first.device
        ,   resourceContext.first.physicalDevice
        ,   resourceContext.first.queue
        ,   resourceContext.second.commandPool
    );
    createTextureImageView(
            resourceContext.first.device
    );
    createTextureSampler(
            resourceContext.first.device
        ,   resourceContext.first.physicalDevice
    );

    return true;
}

void Texture::doUnload(){

}

void Texture::createTextureImage(
        const vk::raii::Device& device, 
        const vk::raii::PhysicalDevice& physicalDevice,
        const vk::raii::Queue& queue,
        const vk::raii::CommandPool& commandPool
){
    int             texWidth, texHeight, texChannels;
    stbi_uc*        pixels = stbi_load("resources/textures/viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels)
        throw std::runtime_error(std::string("could not load texture: [stbi] ") + stbi_failure_reason());

    vk::DeviceSize  imageSize = texWidth * texHeight * 4;
    
    auto [stagingBuffer, stagingBufferMemory] = gv::vkutil::CreateBuffer(
            device
        ,   physicalDevice
        ,   imageSize
        ,   vk::BufferUsageFlagBits::eTransferSrc
        ,   vk::MemoryPropertyFlagBits::eHostVisible 
        | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void* data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, imageSize);
    stagingBufferMemory.unmapMemory();

    stbi_image_free(pixels);

    std::tie(textureImage, textureImageMemory) = GVIS::vkutil::CreateImage(
            device
        ,   physicalDevice
        ,   texWidth
        ,   texHeight
        ,   vk::Format::eR8G8B8A8Srgb
        ,   vk::ImageTiling::eOptimal
        ,   vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
        ,   vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    vk::raii::CommandBuffer commandBuffer = gv::vkutil::BeginSingleTimeCommands(commandPool, device);
    gv::vkutil::TransitionImageLayout(
        commandBuffer
        ,   textureImage
        ,   vk::ImageLayout::eUndefined
        ,   vk::ImageLayout::eTransferDstOptimal
        ,   1 // mip level placeholder
    );
    gv::vkutil::CopyBufferToImage(
        commandBuffer, stagingBuffer, textureImage, 
        static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)
    );
    gv::vkutil::TransitionImageLayout(
        commandBuffer
        ,   textureImage
        ,   vk::ImageLayout::eTransferDstOptimal
        ,   vk::ImageLayout::eShaderReadOnlyOptimal
        ,   1 // mip level placeholder
    );
    gv::vkutil::EndSingleTimeCommands(std::move(commandBuffer), queue);
}

void Texture::createTextureImageView(const vk::raii::Device& device){
    // image views from renderer
    textureImageView = gv::vkutil::CreateImageView(
            device, *textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor , 1);
}

void Texture::createTextureSampler(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice) {
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    vk::SamplerCreateInfo samplerInfo{
        // filtering options for over and under samples
        .magFilter               = vk::Filter::eLinear,
        .minFilter               = vk::Filter::eLinear,
        .mipmapMode              = vk::SamplerMipmapMode::eLinear,
        .mipLodBias              = 0.0f,
        .minLod                  = 0.0f,
        .maxLod                  = vk::LodClampNone,
        // address modes for repeating textures per axis
        .addressModeU            = vk::SamplerAddressMode::eRepeat,
        .addressModeV            = vk::SamplerAddressMode::eRepeat,
        .addressModeW            = vk::SamplerAddressMode::eRepeat,
        .anisotropyEnable        = vk::True,
        .maxAnisotropy           = properties.limits.maxSamplerAnisotropy,
        .borderColor             = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False,
        .compareEnable           = vk::False,
        .compareOp               = vk::CompareOp::eAlways
    };
    // samplerInfo.minLod = static_cast<float>(mipLevels / 2);
    this->textureSampler = vk::raii::Sampler(device, samplerInfo);
}

