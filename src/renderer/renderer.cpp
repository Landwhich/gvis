#include <iostream>

#include "renderer.hpp"

Renderer::Renderer(Platform* platform):platform(platform){

}

void Renderer::Initialize(){
    std::cout << "Initializing Renderer:\n";
    // Renderer registers a lambda, no GLFW/user-pointer knowledge needed
    //
    platform->setResizeCallback([this](int w, int h) { framebufferResizeCallback(w, h); });

    std::cout << " - Creating Instance...\n";
    this->createVulkanInstance();
    this->setupDebugMessenger();
    if(!this->createSurface())
        throw std::runtime_error("failed to create vulkan surface"); 

    // create Device  
    std::cout << " - Creating Device...\n";
    vulkanDevice = std::make_unique<VulkanDevice>(this->instance, this->surface, DeviceParams{
            .requiredExts = gv::REQUIRED_DEVICE_EXT,
            .optionalExts = {},
    }); 
    assert(vulkanDevice);
    assert(vulkanDevice->getDevice() != 0);
    assert(vulkanDevice->getPhysicalDevice() != 0);

    std::cout << " - Creating Swap Chain...\n";
    this->createSwapChain();
    this->createImageViews();

    this->createDescriptorSetLayout();

    std::cout << " - Creating Pipeline...\n";
    this->createGraphicsPipeline();
    this->createCommandPool();
}


void Renderer::Build(){
    std::cout << " - Creating Resources...\n";
    this->createDepthResources();
    this->createTextureImage();
    this->createTextureImageView();
    this->createTextureSampler();

    this->createVertexBuffer();
    this->createIndexBuffer();
    this->createUniformBuffers();
    this->createDescriptorPool();
    this->createDescriptorSets();
    this->createCommandBuffers();
    this->createSyncObjects();
}


void Renderer::RenderModel(Model* model){
    for (const auto& v : model->vertices)
        this->vertices.push_back(v);
    for (const auto& i : model->indices)
        this->indices.push_back(i);
}

void Renderer::IdleDevice(){
    vulkanDevice->idle();
}

void Renderer::Cleanup(){
    cleanupSwapChain();
}

void Renderer::DrawFrame(){
    auto fenceResult = vulkanDevice->getDevice().waitForFences(*this->inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
        throw std::runtime_error("failed to wait for fence!");

    auto [result, imageIndex] = swapChain.acquireNextImage(
        UINT64_MAX, *this->presentCompleteSemaphores[frameIndex], nullptr
    );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // avoid stalls from early returns above ^
    // Only reset the fence if we are submitting work
    vulkanDevice->getDevice().resetFences(*this->inFlightFences[frameIndex]);

    commandBuffers[frameIndex].reset();
    recordCommandBuffer(imageIndex);

    updateUniformBuffer(frameIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*commandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]
    };
    vulkanDevice->getQueue().submit(submitInfo, *inFlightFences[frameIndex]);

    const vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &*swapChain,
        .pImageIndices      = &imageIndex
    };

    auto presentResult = vulkanDevice->getQueue().presentKHR(presentInfo);

    if ((presentResult == vk::Result::eSuboptimalKHR) 
        || (presentResult == vk::Result::eErrorOutOfDateKHR) 
        || framebufferResized
    ) {
        framebufferResized = false;
        recreateSwapChain();
    } else {
    // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
        assert(presentResult == vk::Result::eSuccess);
    }

    frameIndex = (frameIndex + 1) % gv::MAX_FRAMES_IN_FLIGHT;
}
