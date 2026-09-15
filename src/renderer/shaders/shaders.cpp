#include "shaders.hpp"

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    return {
        .binding = 0
        , .stride = sizeof(Vertex)
        , .inputRate = vk::VertexInputRate::eVertex
    };
}

std::array<vk::VertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions() {
    return {{
        {.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, pos)},
        {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)}
    }};
}


[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, vk::raii::Device& device) {
    vk::ShaderModuleCreateInfo createInfo{ 
        .codeSize = code.size() * sizeof(char), 
        .pCode = reinterpret_cast<const uint32_t*>(code.data()) 
    };
    vk::raii::ShaderModule shaderModule{ device, createInfo };

    return shaderModule;
}

