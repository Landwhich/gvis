#include "vulkan_shared.h"

vk::raii::ImageView GVIS::vkutil::CreateImageView(
    const vk::raii::Device& device,
    vk::Image const &image, 
    vk::Format format, 
    vk::ImageAspectFlags aspectFlags,
    uint32_t mipLevels
) {
    vk::ImageViewCreateInfo viewInfo{
        .image            = image,
        .viewType         = vk::ImageViewType::e2D,
        .format           = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,  
            .baseMipLevel = 0, 
            .levelCount = mipLevels, 
            .baseArrayLayer = 0, 
            .layerCount = 1
    }};
    return vk::raii::ImageView(device, viewInfo);
}

vk::raii::CommandBuffer GVIS::vkutil::BeginSingleTimeCommands(
        const vk::raii::CommandPool& commandPool, const vk::raii::Device& device
) {
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool
        ,   .level = vk::CommandBufferLevel::ePrimary
        ,   .commandBufferCount = 1
    };
    vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void GVIS::vkutil::EndSingleTimeCommands(
        vk::raii::CommandBuffer &&commandBuffer, const vk::raii::Queue& queue
) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();
}

void GVIS::vkutil::CopyBufferToImage(
    vk::raii::CommandBuffer &commandBuffer, 
    const vk::raii::Buffer &buffer, 
    vk::raii::Image &image, 
    uint32_t width, 
    uint32_t height
) {
    vk::BufferImageCopy region{
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = {
            .aspectMask = vk::ImageAspectFlagBits::eColor, 
            .mipLevel = 0, 
            .baseArrayLayer = 0, 
            .layerCount = 1
        },
        .imageOffset       = {0, 0, 0},
        .imageExtent       = {width, height, 1}};

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> GVIS::vkutil::CreateBuffer(
    const vk::raii::Device& device,
    const vk::raii::PhysicalDevice& physicalDevice,
    // the former are apart of gvis graphics context
    vk::DeviceSize size,                //buffer size 
    vk::BufferUsageFlags usage,         //how the buffer will be used 
    vk::MemoryPropertyFlags properties  
) {
    vk::BufferCreateInfo bufferInfo{
        .size = size, 
        .usage = usage, 
        .sharingMode = vk::SharingMode::eExclusive
    };
    vk::raii::Buffer buffer = vk::raii::Buffer(device, bufferInfo);
    // buffer created but needs to be assigned memory
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
    .allocationSize = memRequirements.size, 
    .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice)
    };
    vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
    // 0 is the offset within the memory chunk
    // mem is used only for this call so we don't want to offset it internally
    buffer.bindMemory(*bufferMemory, 0);
    return {std::move(buffer), std::move(bufferMemory)};
    // ^-- return the &&val 
    // (raii handles cannot be copied)
}
    
std::pair<vk::raii::Image, vk::raii::DeviceMemory> GVIS::vkutil::CreateImage(
    const vk::raii::Device& device,
    const vk::raii::PhysicalDevice& physicalDevice,
    uint32_t width, 
    uint32_t height, 
    vk::Format format, 
    vk::ImageTiling tiling, 
    vk::ImageUsageFlags usage, 
    vk::MemoryPropertyFlags properties 
) {
    vk::ImageCreateInfo imageInfo{
        .imageType   = vk::ImageType::e2D,
        .format      = format,
        // dictates number of layers, hence depth must be one
        .extent      = {width, height, 1},
        .mipLevels   = 1,
        .arrayLayers = 1,
        .samples     = vk::SampleCountFlagBits::e1,
        .tiling      = tiling,
        .usage       = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    vk::raii::Image image = vk::raii::Image(device, imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = GVIS::vkutil::findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice)
    };
    vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(device, allocInfo);
    image.bindMemory(imageMemory, 0);

    return {std::move(image), std::move(imageMemory)};
}

void GVIS::vkutil::TransitionImageLayout(
    vk::raii::CommandBuffer &commandBuffer, 
    const vk::raii::Image &image, 
    vk::ImageLayout oldLayout, 
    vk::ImageLayout newLayout,  
    uint32_t mipLevels
) {
    vk::ImageMemoryBarrier barrier {
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        // queue family index is for ownership transfer
        // ignored is not the default and is therefore required here
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image               = image,
        .subresourceRange    = {
            .aspectMask = vk::ImageAspectFlagBits::eColor, 
            .levelCount = mipLevels, 
            .layerCount = 1
        },
    };
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage      = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
}

uint32_t GVIS::vkutil::findMemoryType(uint32_t typeFilter, 
        vk::MemoryPropertyFlags properties,  
        const vk::raii::PhysicalDevice& physicalDevice
) {
    vk::PhysicalDeviceMemoryProperties memProperties 
        = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++){
    // make sure index matches filter and req'd properties
        if ((typeFilter & (1 << i)) 
            && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        return i;
    }

    throw std::runtime_error("failed to find suitable memory type!");
}
