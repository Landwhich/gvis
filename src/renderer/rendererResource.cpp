#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "renderer.hpp"
#include "shaders.hpp"

/*
 * Helpers:
 */

void transitionImageLayout( vk::raii::CommandBuffer &commandBuffer, const vk::raii::Image &image, 
                            vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels );

void copyBufferToImage( vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer, 
                        vk::raii::Image &image, uint32_t width, uint32_t height); 

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};


// descriptor sets
void Renderer::createDescriptorSetLayout() {
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings{{{
        .binding = 0, 
        .descriptorType = vk::DescriptorType::eUniformBuffer, 
        .descriptorCount = 1, 
        .stageFlags = vk::ShaderStageFlagBits::eVertex
    } ,{
        .binding = 1, 
        .descriptorType = vk::DescriptorType::eCombinedImageSampler, 
        .descriptorCount = 1, 
        // stage flag for tex could also be in vert shader (heightmaps)
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    }}};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()), 
        .pBindings = bindings.data()
    };
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void Renderer::createDescriptorPool() {
    std::array<vk::DescriptorPoolSize, 2> poolSize{{{
        .type = vk::DescriptorType::eUniformBuffer, 
        .descriptorCount = gv::MAX_FRAMES_IN_FLIGHT
    }, {
        .type = vk::DescriptorType::eCombinedImageSampler, 
        .descriptorCount = gv::MAX_FRAMES_IN_FLIGHT
    }}};
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = gv::MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
        .pPoolSizes    = poolSize.data()
    };
    descriptorPool = vk::raii::DescriptorPool(this->device, poolInfo);
}

void Renderer::createDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(gv::MAX_FRAMES_IN_FLIGHT, *this->descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool     = this->descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts        = layouts.data()
    };

    this->descriptorSets = this->device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < gv::MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{ 
            .buffer = this->uniformBuffers[i], 
            .offset = 0, 
            .range = sizeof(UniformBufferObject) 
        };
        vk::DescriptorImageInfo imageInfo{
            .sampler = this->textureSampler, 
            .imageView = this->textureImageView, 
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };
        std::array<vk::WriteDescriptorSet, 2> descriptorWrites{{
        {
            .dstSet          = this->descriptorSets[i],
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo     = &bufferInfo
        }, {
            .dstSet          = this->descriptorSets[i],
            .dstBinding      = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo      = &imageInfo
        }}};
        this->device.updateDescriptorSets(descriptorWrites, {}); 
    }
}

void Renderer::createUniformBuffers() {
    for (size_t i = 0; i < gv::MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        // we don't use a staging buffer here due to repeated rewrites every frame
        // no worth the overhead
        auto [buffer, bufferMem]  = createBuffer(
            bufferSize, 
            vk::BufferUsageFlagBits::eUniformBuffer, 
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        this->uniformBuffers.emplace_back(std::move(buffer));
        this->uniformBuffersMemory.emplace_back(std::move(bufferMem));
        this->uniformBuffersMapped.emplace_back( this->uniformBuffersMemory.back().mapMemory(0, bufferSize));
    }
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
    // static means it keeps initial start timestamp
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time       = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = rotate(
        rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f))
        ,   time * glm::radians(30.0f)
        ,   glm::vec3(1.0f, 0.0f, 0.0f)
    );
    ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(
        glm::radians(45.0f), 
        static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 
        0.1f, 
        10.0f
    );
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Renderer::createIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        bufferSize, 
        vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void *data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, indices.data(), (size_t) bufferSize);
    stagingBufferMemory.unmapMemory();

    std::tie(indexBuffer, indexBufferMemory) = createBuffer(
        bufferSize, 
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void Renderer::createVertexBuffer(){
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        bufferSize, 
        vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    // make it visible to the CPU
    void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    std::tie(vertexBuffer, vertexBufferMemory) = createBuffer(
        bufferSize, 
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}

void Renderer::createTextureImage(){
    int             texWidth, texHeight, texChannels;
    stbi_uc*        pixels = stbi_load("textures/rova.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels)
        throw std::runtime_error(std::string("could not load texture: [stbi] ") + stbi_failure_reason());

    vk::DeviceSize  imageSize = texWidth * texHeight * 4;
    
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        imageSize
        ,   vk::BufferUsageFlagBits::eTransferSrc
        ,   vk::MemoryPropertyFlagBits::eHostVisible 
        | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void* data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, imageSize);
    stagingBufferMemory.unmapMemory();

    stbi_image_free(pixels);

    std::tie(textureImage, textureImageMemory) = this->createImage(
        texWidth,
        texHeight,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();
    transitionImageLayout(
        commandBuffer
        ,   textureImage
        ,   vk::ImageLayout::eUndefined
        ,   vk::ImageLayout::eTransferDstOptimal
        ,   1 // mip level placeholder
    );
    copyBufferToImage(
        commandBuffer, stagingBuffer, textureImage, 
        static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)
    );
    transitionImageLayout(
        commandBuffer
        ,   textureImage
        ,   vk::ImageLayout::eTransferDstOptimal
        ,   vk::ImageLayout::eShaderReadOnlyOptimal
        ,   1 // mip level placeholder
    );
    endSingleTimeCommands(std::move(commandBuffer));
}

void Renderer::createTextureImageView(){
    textureImageView = createImageView(*textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor , 1);
}

void Renderer::createTextureSampler() {
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
    textureSampler = vk::raii::Sampler(device, samplerInfo);
}

void Renderer::createDepthResources(){
    vk::Format depthFormat = findDepthFormat();

    std::tie(this->depthImage, this->depthImageMemory) = createImage(
        this->swapChainExtent.width, 
        this->swapChainExtent.height, 
        // this->msaaSamples,
        depthFormat, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eDepthStencilAttachment, 
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    this->depthImageView = createImageView(this->depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1); 
}

void Renderer::createCommandPool(){
vk::CommandPoolCreateInfo poolInfo{
// hint flag may affect memory layout
.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
.queueFamilyIndex = queueIndex
};

commandPool = vk::raii::CommandPool(device, poolInfo);
}

void Renderer::createCommandBuffers(){
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool, 
        // level is either primary or second access framebuffers
        // second level are usefull for segmented reoccuring sequences
        // and cannot be sent to the queue directly
        .level = vk::CommandBufferLevel::ePrimary, 
        .commandBufferCount = gv::MAX_FRAMES_IN_FLIGHT
    };

    commandBuffers = vk::raii::CommandBuffers( device, allocInfo );
}

void Renderer::recordCommandBuffer(uint32_t imageIndex){
    auto &commandBuffer = commandBuffers[frameIndex];
    // the vk::CommandBufferBeginInfo struct will be left blank until needed
    commandBuffer.begin({});

    // Before starting rendering, transition the swapchain image to vk::ImageLayout::eColorAttachmentOptimal
    // transitioning the image is required to make sure it's laid out properly for differing draw ops
    transition_image_layout(
        swapChainImages[imageIndex],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,               // layout format
        // srcAccessMask (no need to wait for previous operations)
        {},                                                                
        vk::AccessFlagBits2::eColorAttachmentWrite,             // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // dstStage
        vk::ImageAspectFlagBits::eColor
    );
    transition_image_layout(
        *depthImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth
    );

    // default values for depth and colour
    vk::ClearValue clearColor = vk::ClearColorValue(0.1f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue{1.0f, 0};

    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView   = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = clearColor
    };
    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView   = depthImageView,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eDontCare,
        .clearValue  = clearDepth
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea           = {.offset = {0, 0}, .extent = swapChainExtent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachmentInfo,
        .pDepthAttachment     = &depthAttachmentInfo
    };
    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
    commandBuffer.setViewport(0, vk::Viewport{
        0.0f,  
        static_cast<float>(swapChainExtent.height), 
        static_cast<float>(swapChainExtent.width), 
        // height * -1 to compensate for y flip
        -static_cast<float>(swapChainExtent.height), 
        0.0f, 1.0f
    });
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, swapChainExtent});
    commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
    commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
    commandBuffers[frameIndex].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[frameIndex], nullptr
    );
    commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    commandBuffer.endRendering();

    // After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
    transition_image_layout(
        swapChainImages[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
        {},                                                     // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe,              // dstStage
        vk::ImageAspectFlagBits::eColor
    );
    commandBuffer.end();
}

void Renderer::createSyncObjects(){
    assert(this->presentCompleteSemaphores.empty() && this->renderFinishedSemaphores.empty() && this->inFlightFences.empty());
    for (size_t i = 0; i < this->swapChainImages.size(); i++) {
        this->renderFinishedSemaphores.emplace_back(this->device, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < gv::MAX_FRAMES_IN_FLIGHT; i++) {
        this->presentCompleteSemaphores.emplace_back(this->device, vk::SemaphoreCreateInfo());
        this->inFlightFences.emplace_back(this->device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }    
}

void Renderer::transition_image_layout(
    vk::Image               image,
    vk::ImageLayout         old_layout,
    vk::ImageLayout         new_layout,
    vk::AccessFlags2        src_access_mask,
    vk::AccessFlags2        dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    vk::ImageAspectFlags    image_aspect_flags
) {
    vk::ImageMemoryBarrier2 barrier = {
        // .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask           = src_stage_mask,
        .srcAccessMask          = src_access_mask,
        .dstStageMask           = dst_stage_mask,
        .dstAccessMask          = dst_access_mask,
        .oldLayout              = old_layout,
        .newLayout              = new_layout,
        .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
        .image                  = image,
        .subresourceRange       = {
            .aspectMask     = image_aspect_flags,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };

    vk::DependencyInfo dependency_info = {
        .dependencyFlags         = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier
    };

    commandBuffers[frameIndex].pipelineBarrier2(dependency_info);
}

void transitionImageLayout(
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


std::pair<vk::raii::Image, vk::raii::DeviceMemory> Renderer::createImage(
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

    vk::raii::Image image = vk::raii::Image(this->device, imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = this->findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(this->device, allocInfo);
    image.bindMemory(imageMemory, 0);

    return {std::move(image), std::move(imageMemory)};
}

void copyBufferToImage(
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

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Renderer::createBuffer(
    vk::DeviceSize size,                //buffer size 
    vk::BufferUsageFlags usage,         //how the buffer will be used 
    vk::MemoryPropertyFlags properties  
) {
    vk::BufferCreateInfo   bufferInfo{
        .size = size, 
        .usage = usage, 
        .sharingMode = vk::SharingMode::eExclusive
    };
    vk::raii::Buffer       buffer          = vk::raii::Buffer(device, bufferInfo);
    // buffer created but needs to be assigned memory
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size, 
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
    // 0 is the offset within the memory chunk
    // mem is used only for this call so we don't want to offset it internally
    buffer.bindMemory(*bufferMemory, 0);
    return {std::move(buffer), std::move(bufferMemory)};
    // ^-- return the &&val 
    // (raii handles cannot be copied)
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++){
        // make sure index matches filter and req'd properties
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void Renderer::copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size) {
    vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
    commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{
        .srcOffset = 0, 
        .dstOffset = 0, 
        .size = size
    }); 
    endSingleTimeCommands(std::move(commandCopyBuffer));
}

vk::raii::CommandBuffer Renderer::beginSingleTimeCommands() {
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = this->commandPool
        ,   .level = vk::CommandBufferLevel::ePrimary
        ,   .commandBufferCount = 1
    };
    vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(this->device, allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void Renderer::endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    this->queue.submit(submitInfo, nullptr);
    this->queue.waitIdle();
}

vk::Format Renderer::findDepthFormat() {
        return findSupportedFormat(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}
            ,   vk::ImageTiling::eOptimal
            ,   vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
}

vk::Format Renderer::findSupportedFormat(
    const std::vector<vk::Format>& candidates, 
    vk::ImageTiling tiling, 
    vk::FormatFeatureFlags features
) {
    for (const auto format : candidates) {
        vk::FormatProperties props = this->physicalDevice.getFormatProperties(format);
        
        // candidacy will depend largely on tiling mode and usage
        if (((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features)) ||
        ((tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features))) {
            return format;
        }
    }

throw std::runtime_error("failed to find supported format!");
}


