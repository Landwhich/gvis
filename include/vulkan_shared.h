#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

namespace GVIS{
#ifdef NDEBUG
    static constexpr bool VALIDATION_LAYERS_ENABLED = false;
#else
    static constexpr bool VALIDATION_LAYERS_ENABLED = true;
#endif

static constexpr std::array<const char*, 1> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
}; 
} // GVIS_VULKAN
