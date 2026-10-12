#include "device.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>
//remove with debuging

VulkanDevice::VulkanDevice(const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface, 
        const DeviceParams& params) : instance(instance), surface(surface){
    this->deviceExtensions = params.requiredExts;               
    this->deviceExtensions.insert(deviceExtensions.end(), params.optionalExts.begin(), params.optionalExts.end());

    this->pickPhysicalDevice();
    this->createLogicalDevice();
}

void VulkanDevice::pickPhysicalDevice(){
    std::vector<vk::raii::PhysicalDevice> physicalDevices = this->instance.enumeratePhysicalDevices();
    const auto& devIter = std::ranges::find_if(
        physicalDevices, [&](const auto& physicalDevice) {return this->isDeviceSuitable(physicalDevice);});

    if (devIter == physicalDevices.end()){
        throw std::runtime_error("No suitable Device Found");
    }
    this->physicalDevice = *devIter;
}


// bool VulkanDevice::createSurface(){
//     VkSurfaceKHR _surface;
//     if(this->platform->createWindowSurface(&this->instance, &_surface)) 
//         return false;
//     this->surface = vk::raii::SurfaceKHR(this->instance, _surface);
//     return true;
// }

void VulkanDevice::createLogicalDevice() {
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
        ,   {.shaderDrawParameters = true}                      // vk::PhysicalDeviceVulkan11Features
        ,   {.dynamicRendering = true, .synchronization2 = true}// vk::PhysicalDeviceVulkan13Features
        ,   {.extendedDynamicState = true}                      // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
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
    updatedDeviceExtenions.assign(deviceExtensions.begin(), deviceExtensions.end());
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
    this->logicalDevice = vk::raii::Device(this->physicalDevice, deviceCreateInfo);
    this->graphicsQueue = vk::raii::Queue(this->logicalDevice, queueIndex, 0);
}

bool VulkanDevice::isDeviceSuitable( vk::raii::PhysicalDevice const& physicalDevice ) {
    // query for properties
    const auto& properties = physicalDevice.getProperties();
    std::cout << "checking device: " << properties.deviceName << std::endl;
    // Check if the physicalDevice supports the Vulkan 1.3 API version
    bool supportsVulkan1_3 = properties.apiVersion >= vk::ApiVersion13;
    if (!supportsVulkan1_3){std::cout << "- doesn't support vulkan 1.3\n";}

    //Check if any of the queue families support graphics operations
    // TODO: expand queue support checks for other future queues
    auto queueFamilies    = physicalDevice.getQueueFamilyProperties();
    bool supportsGraphics = std::ranges::any_of(
        queueFamilies, []( auto const & qfp ) { return !!( qfp.queueFlags & vk::QueueFlagBits::eGraphics ); } 
    );
    if (!supportsGraphics){std::cout << "- doesn't support graphics\n";}

    // Check if all required physicalDevice extensions are available
    auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    bool supportsAllRequiredExtensions = std::ranges::all_of(
        this->deviceExtensions,
        [&availableDeviceExtensions]( auto const& requiredDeviceExtension ) {
        return std::ranges::any_of(
            availableDeviceExtensions,
            [requiredDeviceExtension]( auto const & availableDeviceExtension ){ 
            return strcmp( availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; } 
        ); } 
    );
    // TODO: clean up feature checks
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
    return supportsVulkan1_3 
        && supportsGraphics 
        && supportsAllRequiredExtensions 
        && supportsRequiredFeatures;
}   

vk::Format VulkanDevice::findSupportedFormat(
    const std::vector<vk::Format>& candidates, 
    vk::ImageTiling tiling, 
    vk::FormatFeatureFlags features
) const {
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

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++){
        // make sure index matches filter and req'd properties
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

