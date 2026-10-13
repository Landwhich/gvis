#include "descriptor_manager.hpp"
#include "vulkan_shared.h"

namespace gv = GVIS;

DescriptorManager::DescriptorManager(const vk::raii::Device& device) : device(device){

}

void DescriptorManager::createDescriptorPool() {
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

void DescriptorManager::createDescriptorSets(
    const std::vector<vk::raii::Buffer>& uniformBuffers,
    const vk::raii::ImageView& textureImageView, 
    const vk::raii::Sampler& textureSampler
) { 
    std::vector<vk::DescriptorSetLayout> layouts(gv::MAX_FRAMES_IN_FLIGHT, 
            *this->descriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool     = this->descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts        = layouts.data()
    };

    descriptorSets = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < gv::MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{ 
            .buffer = uniformBuffers[i], 
            .offset = 0, 
            .range = sizeof(UniformBufferObject) 
        };
        vk::DescriptorImageInfo imageInfo{
            .sampler = textureSampler, 
            .imageView = textureImageView, 
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };
        std::array<vk::WriteDescriptorSet, 2> descriptorWrites{{
        {
            .dstSet          = this->descriptorSets[i],
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo     = &bufferInfo
        }, {
            .dstSet          = this->descriptorSets[i],
            .dstBinding      = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo      = &imageInfo
        }}};

        this->device.updateDescriptorSets(descriptorWrites, {}); 
    }
}

void DescriptorManager::createDescriptorSetLayout() { 
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
