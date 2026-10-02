#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <vector>
#include <array>

namespace GVIS{
#ifdef NDEBUG
    static constexpr bool VALIDATION_LAYERS_ENABLED = false;
#else
    static constexpr bool VALIDATION_LAYERS_ENABLED = true;
#endif
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    const std::string  MODEL_PATH           = "resources/models/viking_room.obj";
    const std::string  TEXTURE_PATH         = "resources/textures/viking_room.png";

    static constexpr std::array<const char*, 1> VALIDATION_LAYERS = {
            "VK_LAYER_KHRONOS_validation"
    }; 
    static std::vector<const char*> REQUIRED_DEVICE_EXT = {
            vk::KHRSwapchainExtensionName
    };
} // GVIS_VULKAN
