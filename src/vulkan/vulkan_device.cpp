#include "renderer.hpp"

#include <vector>
#include <stdexcept>
//remove with debuging
#include <iostream>

bool Renderer::createSurface(){
    VkSurfaceKHR _surface;
    if(this->platform->createWindowSurface(&this->instance, &_surface)) 
        return false;
    this->surface = vk::raii::SurfaceKHR(this->instance, _surface);
    return true;
}

void Renderer::createLogicalDevice() {
    //  find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = this->physicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    queueIndex = ~0;

    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
        // found a queue family that supports both graphics and present
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) 
            && physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) 
        {
            queueIndex = qfpIndex;
            break;
        }
    }

    if (queueIndex == ~0)
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");

    // query for Vulkan 1.3 features
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2
        , vk::PhysicalDeviceVulkan11Features
        , vk::PhysicalDeviceVulkan13Features
        , vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    >featureChain = {
    {.features = {.samplerAnisotropy = true}}               // vk::PhysicalDeviceFeatures2
    , {.shaderDrawParameters = true}                        // vk::PhysicalDeviceVulkan11Features
    , {.dynamicRendering = true, .synchronization2 = true}  // vk::PhysicalDeviceVulkan13Features
    , {.extendedDynamicState = true}                        // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    };

    // create a Device
    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = queueIndex 
        , .queueCount = 1 
        , .pQueuePriorities = &queuePriority
    };
    
    auto availableDeviceExtensions = this->physicalDevice.enumerateDeviceExtensionProperties();
    std::vector<const char*> updatedDeviceExtenions(2);
    updatedDeviceExtenions.assign(gv::REQUIRED_DEVICE_EXT.begin(), gv::REQUIRED_DEVICE_EXT.end());
    bool hasPortabilitySubset = std::ranges::any_of(
        availableDeviceExtensions,
        [](auto const& ext) { return strcmp(ext.extensionName, "VK_KHR_portability_subset") == 0; });
    if (hasPortabilitySubset)
        updatedDeviceExtenions.push_back("VK_KHR_portability_subset");

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext                      = &featureChain.get<vk::PhysicalDeviceFeatures2>()
        , .queueCreateInfoCount     = 1
        , .pQueueCreateInfos        = &deviceQueueCreateInfo
        , .enabledExtensionCount    = static_cast<uint32_t>(updatedDeviceExtenions.size())
        , .ppEnabledExtensionNames  = updatedDeviceExtenions.data()
    };
    this->device = vk::raii::Device(this->physicalDevice, deviceCreateInfo);
    this->queue = vk::raii::Queue(this->device, queueIndex, 0);
}

void Renderer::pickPhysicalDevice(){
    std::vector<vk::raii::PhysicalDevice> physicalDevices = this->instance.enumeratePhysicalDevices();
    const auto& devIter = std::ranges::find_if(
        physicalDevices, [&](const auto& physicalDevice) {return this->isDeviceSuitable(physicalDevice);});
    if (devIter == physicalDevices.end()){
        throw std::runtime_error("No suitable Device Found");
    }
    this->physicalDevice = *devIter;
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
    if (unsupportedLayerIt != requiredLayers.end()){
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }

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
    {
        throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
    }

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
    if (gv::VALIDATION_LAYERS_ENABLED){
        this->requiredExtensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
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
        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
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

    vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(
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

bool Renderer::isDeviceSuitable( vk::raii::PhysicalDevice const& physicalDevice ) {
    // Check if the physicalDevice supports the Vulkan 1.3 API version
    bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

    //Check if any of the queue families support graphics operations
    auto queueFamilies    = physicalDevice.getQueueFamilyProperties();
    bool supportsGraphics = std::ranges::any_of(
        queueFamilies, []( auto const & qfp ) { return !!( qfp.queueFlags & vk::QueueFlagBits::eGraphics ); } 
    );

    // Check if all required physicalDevice extensions are available
    auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    bool supportsAllRequiredExtensions = std::ranges::all_of(
        gv::REQUIRED_DEVICE_EXT,
        [&availableDeviceExtensions]( auto const& requiredDeviceExtension ) {
        return std::ranges::any_of(
            availableDeviceExtensions,
            [requiredDeviceExtension]( auto const & availableDeviceExtension ){ 
            return strcmp( availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; } 
        ); } 
    );
    // Check if the physicalDevice supports the required features:
    // (shader draw parameters, dynamic rendering and extended dynamic state)
    auto features = physicalDevice.template getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT >();

    bool supportsRequiredFeatures = 
    features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy 
    && features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 
    && features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters 
    && features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering 
    && features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

    // Return true if the physicalDevice meets all the criteria
    return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}   

