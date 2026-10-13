#include "file.hpp"
#include "shaders.hpp"

namespace gv = GVIS;

ShaderModule::ShaderModule(const std::string& filename, 
    const vk::raii::Device& device) 
    //: device(device) 
{
    shaderModule = createShaderModule(gv::readShaderFile(filename), device);
}

[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, const vk::raii::Device& device) {
    vk::ShaderModuleCreateInfo createInfo {  
        .codeSize = code.size() * sizeof(char), 
        .pCode = reinterpret_cast<const uint32_t*>(code.data()) 
    };
    vk::raii::ShaderModule shaderModule{ device, createInfo };

    return shaderModule;
}

