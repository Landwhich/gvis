#include "file.hpp"
#include "shaders.hpp"

<<<<<<< HEAD
namespace gv = GVIS;

ShaderModule::ShaderModule(const std::string& filename, vk::raii::Device& device){
    shaderModule = createShaderModule(gv::readShaderFile(filename), device);
}

[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, vk::raii::Device& device) {
    vk::ShaderModuleCreateInfo createInfo {  
=======
[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, const vk::raii::Device& device) {
    vk::ShaderModuleCreateInfo createInfo{ 
>>>>>>> origin/better-resources
        .codeSize = code.size() * sizeof(char), 
        .pCode = reinterpret_cast<const uint32_t*>(code.data()) 
    };
    vk::raii::ShaderModule shaderModule{ device, createInfo };

    return shaderModule;
}

void ShaderModule::createDescriptorSetLayout() {
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings{{{
        .binding = 0, 
        .descriptorType = vk::DescriptorType::eUniformBuffer, 
        .descriptorCount = 1, 
        .stageFlags = vk::ShaderStageFlagBits::eVertex
    } ,{
        .binding = 1, 
        .descriptorType = vk::DescriptorType::eCombinedImageSampler, 
        .descriptorCount = 1, 
        // stage flag for tex could also be in vert shader (heightmaps)
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    }}};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()), 
        .pBindings = bindings.data()
    };
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void ShaderModule::createDescriptorPool() {
    std::array<vk::DescriptorPoolSize, 2> poolSize{{{
        .type = vk::DescriptorType::eUniformBuffer, 
        .descriptorCount = gv::MAX_FRAMES_IN_FLIGHT
    }, {
        .type = vk::DescriptorType::eCombinedImageSampler, 
        .descriptorCount = gv::MAX_FRAMES_IN_FLIGHT
    }}};
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = gv::MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
        .pPoolSizes    = poolSize.data()
    };
    descriptorPool = vk::raii::DescriptorPool(this->device, poolInfo);
}

void ShaderModule::createDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(gv::MAX_FRAMES_IN_FLIGHT, *this->descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool     = this->descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts        = layouts.data()
    };

    this->descriptorSets = this->device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < gv::MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{ 
