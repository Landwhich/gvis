#include "renderer.hpp"
#include "shaders.hpp"

/*
 * Helpers:
 */
// vk::DependencyInfo get_transition_image_layout(
//     vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask,
//     vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask,
//     vk::ImageAspectFlags image_aspect_flags
// );

const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}}
};

// descriptor sets
void Renderer::createDescriptorSetLayout() {
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings{{{
        .binding = 0, 
        .descriptorType = vk::DescriptorType::eUniformBuffer, 
        .descriptorCount = 1, 
        .stageFlags = vk::ShaderStageFlagBits::eVertex
    },{
        .binding = 1, 
        .descriptorType = vk::DescriptorType::eCombinedImageSampler, 
        .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment
    }}};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()), 
        .pBindings = bindings.data()
    };
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
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
    // default values for depth and colour
    vk::ClearValue clearColor = vk::ClearColorValue(0.5f, 0.0f, 0.0f, 1.0f);
//    vk::ClearValue clearDepth = vk::ClearDepthStencilValue{1.0f, 0};
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
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView   = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = clearColor
    };

    // TODO: depth buffering
    // commandBuffers[frameIndex].pipelineBarrier2(get_transition_image_layout(
    //     *depthImage,
    //     vk::ImageLayout::eUndefined,
    //     vk::ImageLayout::eDepthAttachmentOptimal,
    //     vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    //     vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    //     vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
    //     vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
    //     vk::ImageAspectFlagBits::eDepth
    // ));
    // vk::RenderingAttachmentInfo depthAttachmentInfo = {
    //     .imageView   = depthImageView,
    //     .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
    //     .loadOp      = vk::AttachmentLoadOp::eClear,
    //     .storeOp     = vk::AttachmentStoreOp::eDontCare,
    //     .clearValue  = clearDepth
    // };

    vk::RenderingInfo renderingInfo = {
        .renderArea           = {.offset = {0, 0}, .extent = swapChainExtent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachmentInfo,
        // .pDepthAttachment     = &depthAttachmentInfo
    };
    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
    // commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexTypeValue<decltype(indices)::value_type>::value);
    commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
    commandBuffer.setViewport(0, vk::Viewport{
        0.0f, 0.0f, static_cast<float>(swapChainExtent.width), 
        static_cast<float>(swapChainExtent.height), 
        0.0f, 1.0f
    });
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, swapChainExtent});

    commandBuffer.draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);

    // TODO: clean up index + vertex buffers
    // TODO: setup descriptors properly    
    // Params for the draw function:
    // vertexCount, instanceCount, firstVertex, firstInstance
    // commandBuffer.bindDescriptorSets(
    // vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[frameIndex], nullptr);
    // commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
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
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool, 
        .level = vk::CommandBufferLevel::ePrimary, 
        .commandBufferCount = 1
    };

    vk::raii::CommandBuffer commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());

    commandCopyBuffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{
        .srcOffset = 0, 
        .dstOffset = 0, 
        .size = size
    }); 

    commandCopyBuffer.end();
    queue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer}, nullptr);
    queue.waitIdle();
}

