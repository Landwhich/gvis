#include "renderer.hpp"

void Renderer::Initialize(Platform* platform){
    this->platform = platform;
    // Renderer registers a lambda, no GLFW/user-pointer knowledge needed
    platform->setResizeCallback([this](int w, int h) { framebufferResizeCallback(w, h); });
    this->createVulkanInstance();
    this->setupDebugMessenger();
    if(!this->createSurface())
        throw std::runtime_error("failed to create vulkan surface"); 
    this->pickPhysicalDevice();
    this->createLogicalDevice();
    this->createSwapChain();
    this->createImageViews();
    this->createDescriptorSetLayout();
    this->createGraphicsPipeline();
    this->createCommandPool();
    this->createVertexBuffer();
    this->createIndexBuffer();
    this->createCommandBuffers();
    this->createSyncObjects();
}

void Renderer::IdleDevice(){
    device.waitIdle();
}

void Renderer::Cleanup(){
    cleanupSwapChain();
}

void Renderer::DrawFrame(){
    auto fenceResult = device.waitForFences(*this->inFlightFences[frameIndex], vk::True, UINT64_MAX);
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
    device.resetFences(*this->inFlightFences[frameIndex]);

    commandBuffers[frameIndex].reset();
    recordCommandBuffer(imageIndex);

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
    queue.submit(submitInfo, *inFlightFences[frameIndex]);

    const vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &*swapChain,
        .pImageIndices      = &imageIndex
    };

    auto presentResult = queue.presentKHR(presentInfo);

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
