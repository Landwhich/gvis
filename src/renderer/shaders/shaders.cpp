#include "shaders.hpp"

[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, vk::raii::Device& device) {
    vk::ShaderModuleCreateInfo createInfo{ 
        .codeSize = code.size() * sizeof(char), 
        .pCode = reinterpret_cast<const uint32_t*>(code.data()) 
    };
    vk::raii::ShaderModule shaderModule{ device, createInfo };

    return shaderModule;
}

