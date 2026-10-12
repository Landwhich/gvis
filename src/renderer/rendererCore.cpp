#include <iostream>

#include "renderer.hpp"

/* image views
 */

bool Renderer::createSurface(){
    VkSurfaceKHR _surface;
    if(this->platform->createWindowSurface(&this->instance, &_surface)) 
        return false;
    this->surface = vk::raii::SurfaceKHR(this->instance, _surface);
        return true;
}

void Renderer::createImageViews() {
    assert(swapChainImageViews.empty());

    swapChainImageViews.reserve(swapChainImages.size());
    for ( auto &image: swapChainImages ) {
        swapChainImageViews.emplace_back(createImageView(
            image, swapChainSurfaceFormat.format, 
            vk::ImageAspectFlagBits::eColor, 1
        ));
    }
}

vk::raii::ImageView Renderer::createImageView(
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
    return vk::raii::ImageView(vulkanDevice->getDevice(), viewInfo);
}

/* Swap chain setup 
 */
void Renderer::createSwapChain(){
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = vulkanDevice->getPhysicalDevice().getSurfaceCapabilitiesKHR(*surface);
    swapChainExtent                                = chooseSwapExtent(surfaceCapabilities);
    uint32_t minImageCount                         = chooseSwapMinImageCount(surfaceCapabilities);

    std::vector<vk::SurfaceFormatKHR> availableFormats = vulkanDevice->getPhysicalDevice().getSurfaceFormatsKHR(*surface);
    swapChainSurfaceFormat                             = chooseSwapSurfaceFormat(availableFormats);

    std::vector<vk::PresentModeKHR> availablePresentModes = vulkanDevice->getPhysicalDevice().getSurfacePresentModesKHR(*surface);
    vk::PresentModeKHR              presentMode           = chooseSwapPresentMode(availablePresentModes);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface          = *surface,
        .minImageCount    = minImageCount,
        .imageFormat      = swapChainSurfaceFormat.format,
        .imageColorSpace  = swapChainSurfaceFormat.colorSpace,
        .imageExtent      = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform     = surfaceCapabilities.currentTransform,
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode      = presentMode,
        .clipped          = true
    };

    swapChain       = vk::raii::SwapchainKHR(vulkanDevice->getDevice(), swapChainCreateInfo);
    swapChainImages = swapChain.getImages();
}

void Renderer::cleanupSwapChain(){
    swapChainImageViews.clear();
    swapChain = nullptr; 
}

void Renderer::recreateSwapChain(){

    int width = 0, height = 0;
    platform->getFramebufferSize(&width, &height);
    if ((!width || !height) && !platform->shouldClose()){
        platform->getFramebufferSize(&width, &height);
        platform->wait();
    }  
    if (platform->shouldClose())
        return;

    IdleDevice();
    // may be neat to implement pass recreation  
    // to change image format in swapChain
    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createDepthResources();
}

vk::Extent2D Renderer::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    int width, height;
    platform->getFramebufferSize(&width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

uint32_t Renderer::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities){
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
        minImageCount = surfaceCapabilities.maxImageCount;
    return minImageCount;
}

vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    const auto formatIt = std::ranges::find_if(
        availableFormats, [](const auto &format) { 
        return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; 
    });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR Renderer::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes) {
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { 
        return presentMode == vk::PresentModeKHR::eFifo; 
    }));
    return std::ranges::any_of(availablePresentModes,
        [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
        vk::PresentModeKHR::eMailbox
        : vk::PresentModeKHR::eFifo;
}

void Renderer::createVulkanInstance(){
    constexpr vk::ApplicationInfo appInfo {
        .pApplicationName       = "GVIS_ENGINE"
        , .applicationVersion   = VK_MAKE_VERSION( 1, 0, 0)
        , .pEngineName          = "No engine"  
        , .engineVersion        = VK_MAKE_VERSION( 1, 0, 0)
        , .apiVersion           = vk::ApiVersion14
    }; 

    std::vector<const char*> requiredLayers;      
    if (gv::VALIDATION_LAYERS_ENABLED){
        requiredLayers.assign(gv::VALIDATION_LAYERS.begin(), gv::VALIDATION_LAYERS.end());
    }

    auto layerProperties = context.enumerateInstanceLayerProperties();
    auto unsupportedLayerIt = std::ranges::find_if(
        requiredLayers,
        [&layerProperties](auto const& requiredLayer) {
            return std::ranges::none_of(
            layerProperties,
            [requiredLayer](auto const& layerProperty) {
                return strcmp(layerProperty.layerName, requiredLayer);  
            }
        );}
    );   

    if (unsupportedLayerIt != requiredLayers.end())
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));

    getRequiredInstanceExtensions();
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    auto unsupportedPropertyIt = std::ranges::find_if(
        requiredExtensions,
        [&extensionProperties](auto const &requiredExtension) {
            return std::ranges::none_of(extensionProperties,
            [requiredExtension](auto const &extensionProperty) { 
                return strcmp(extensionProperty.extensionName, requiredExtension) == 0; });
        }
    );

    if (unsupportedPropertyIt != requiredExtensions.end())
        throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));

#ifdef __APPLE__
    this->requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

    vk::InstanceCreateInfo createInfo{ 
        .pApplicationInfo           = &appInfo   
#ifdef __APPLE__
        , .flags                    = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR
#else
        , .flags                    = vk::InstanceCreateFlags{}
#endif
        , .enabledLayerCount        = static_cast<uint32_t>(requiredLayers.size())
        , .ppEnabledLayerNames      = requiredLayers.data()
        , .enabledExtensionCount    = static_cast<uint32_t>(requiredExtensions.size()) 
        , .ppEnabledExtensionNames  = requiredExtensions.data() 
    };

    instance = vk::raii::Instance(context, createInfo);
}

void Renderer::getRequiredInstanceExtensions(){
    // fetching glfw exts
    auto [platformExtensionCount, platformExtensions] = platform->getPlatformExtensions();
    this->requiredExtensions.assign(platformExtensions, platformExtensions + platformExtensionCount);
    // fetching validation layer exts
    if (gv::VALIDATION_LAYERS_ENABLED)
        this->requiredExtensions.push_back(vk::EXTDebugUtilsExtensionName);
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity, 
    vk::DebugUtilsMessageTypeFlagsEXT type, 
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, 
    void *
) {
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError 
        || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
    ) {
        std::cerr   << "validation layer: type " << to_string(type) 
                    << " msg: " << pCallbackData->pMessage << std::endl;
    }

    return vk::False;
}

void Renderer::setupDebugMessenger(){
    if (!gv::VALIDATION_LAYERS_ENABLED)
        return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );

    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | 
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType     = messageTypeFlags,
        .pfnUserCallback = &debugCallback
    };

    this->debugMessenger = this->instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}
