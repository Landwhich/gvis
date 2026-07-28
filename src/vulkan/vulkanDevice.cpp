#include "vulkanDevice.h"

#include <vector>

static std::vector<const char *> getRequiredInstanceExtensions(){
    uint32_t glfwExtensionCount = 0;
    auto     glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (gv::VALIDATION_LAYERS_ENABLED){
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
}

void VulkanDevice::createVulkanInstance(){
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

    auto requiredExtensions = getRequiredInstanceExtensions();
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
    requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
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
