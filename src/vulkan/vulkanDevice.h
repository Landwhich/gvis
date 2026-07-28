#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <GLFW/glfw3.h>

#include "vulkan_shared.h"

namespace gv = GVIS;

class VulkanDevice {
    vk::raii::Context       context; 
    vk::raii::Instance      instance = nullptr; 
public:
    void createVulkanInstance();
private:
};
