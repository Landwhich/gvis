#include <chrono>

#include "renderer.hpp"
#include "shaders.hpp"
#include "vulkan_shared.h"

namespace gv = GVIS;

/*
 * Helpers:
 */

void copyBufferToImage( vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer, 
                        vk::raii::Image &image, uint32_t width, uint32_t height); 

void Renderer::createUniformBuffers() {
    for (size_t i = 0; i < gv::MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        // we don't use a staging buffer here due to repeated rewrites every frame
        // no worth the overhead
        auto [buffer, bufferMem] = gv::vkutil::CreateBuffer(
            vulkanDevice->getDevice(),
            vulkanDevice->getPhysicalDevice(),
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
        ,   time * glm::radians(00.0f)
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

    auto [stagingBuffer, stagingBufferMemory] = gv::vkutil::CreateBuffer(
        vulkanDevice->getDevice(),
        vulkanDevice->getPhysicalDevice(),
        bufferSize, 
        vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void *data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, indices.data(), (size_t) bufferSize);
    stagingBufferMemory.unmapMemory();

    std::tie(indexBuffer, indexBufferMemory) = gv::vkutil::CreateBuffer(
        vulkanDevice->getDevice(),
        vulkanDevice->getPhysicalDevice(),
        bufferSize, 
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void Renderer::createVertexBuffer(){
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    auto [stagingBuffer, stagingBufferMemory] = gv::vkutil::CreateBuffer(
        vulkanDevice->getDevice(),
        vulkanDevice->getPhysicalDevice(),
        bufferSize, 
        vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    // make it visible to the CPU
    void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    std::tie(vertexBuffer, vertexBufferMemory) = gv::vkutil::CreateBuffer(
        vulkanDevice->getDevice(),
        vulkanDevice->getPhysicalDevice(),
        bufferSize, 
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}

void Renderer::createDepthResources(){
    vk::Format depthFormat = findDepthFormat();

    std::tie(this->depthImage, this->depthImageMemory) = gv::vkutil::CreateImage(
        vulkanDevice->getDevice(),
        vulkanDevice->getPhysicalDevice(),
        this->swapChainExtent.width, 
        this->swapChainExtent.height, 
        // this->msaaSamples,
        depthFormat, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eDepthStencilAttachment, 
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    this->depthImageView = gv::vkutil::CreateImageView(
            vulkanDevice->getDevice(), this->depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1); 
}

void Renderer::createCommandPool(){
    vk::CommandPoolCreateInfo poolInfo{
        // hint flag may affect memory layout
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = vulkanDevice->getQueueIndex()
    };

commandPool = vk::raii::CommandPool(vulkanDevice->getDevice(), poolInfo);
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

    commandBuffers = vk::raii::CommandBuffers(vulkanDevice->getDevice(), allocInfo );
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

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline->getPipeline());
    commandBuffer.setViewport(0, vk::Viewport{
        0.0f, 0.0,  
        static_cast<float>(swapChainExtent.width), 
        static_cast<float>(swapChainExtent.height), 
        // height * -1 to compensate for y flip
        // -static_cast<float>(swapChainExtent.height), 
        0.0f, 1.0f
    });
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, swapChainExtent});
    commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
    commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexTypeValue<decltype(indices)::value_type>::value);
    commandBuffers[frameIndex].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, graphicsPipeline->getLayout(), 0, 
            *graphicsPipeline->getDescriptorSets()[frameIndex], nullptr );
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
        this->renderFinishedSemaphores.emplace_back(vulkanDevice->getDevice(), vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < gv::MAX_FRAMES_IN_FLIGHT; i++) {
        this->presentCompleteSemaphores.emplace_back(vulkanDevice->getDevice(), vk::SemaphoreCreateInfo());
        this->inFlightFences.emplace_back(vulkanDevice->getDevice(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
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

void Renderer::copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size) {
    vk::raii::CommandBuffer commandCopyBuffer = gv::vkutil::BeginSingleTimeCommands(
            this->commandPool ,vulkanDevice->getDevice());
    commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{
        .srcOffset = 0, 
        .dstOffset = 0, 
        .size = size
    }); 
    gv::vkutil::EndSingleTimeCommands(std::move(commandCopyBuffer), vulkanDevice->getQueue());
}


vk::Format Renderer::findDepthFormat() {
    return vulkanDevice->findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}
        ,   vk::ImageTiling::eOptimal
        ,   vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}
