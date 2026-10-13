#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include "shaders.hpp"

class DescriptorManager{
    const vk::raii::Device&                 device;

    vk::raii::DescriptorPool                descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet>    descriptorSets;
    vk::raii::DescriptorSetLayout           descriptorSetLayout = nullptr;

  public:
    // TODO: parameterize descriptor layouts for different bindings
    struct params{
        
    };

    DescriptorManager(const vk::raii::Device& device);
    ~DescriptorManager() = default;
    
    const vk::raii::DescriptorPool& getPool() const {return descriptorPool;}  
    const std::vector<vk::raii::DescriptorSet>& getSets() const {return descriptorSets;}
    const vk::raii::DescriptorSetLayout& getLayout() const {return descriptorSetLayout;}
    /*
     * descriptor bindings are used to transmit information from the 
     * cpp code to the shader code. this layout creation is used to feed
     * information to the pipeline in creation for the vertex shader
     */
    void createDescriptorSetLayout();
    void createDescriptorPool();
    /*
     * descriptor sets are created after the established layout passed to
     * the pipeline and are needed to bind UBO  
     */
    void createDescriptorSets(
        const std::vector<vk::raii::Buffer>& uniformBuffers,
        const vk::raii::ImageView& textureImageView, 
        const vk::raii::Sampler& textureSampler);
};

