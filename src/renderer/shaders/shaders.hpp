#pragma once

#include <vector>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

<<<<<<< HEAD
class ShaderModule{
    vk::raii::Device&                       device;
    vk::raii::ShaderModule                  shaderModule = nullptr;
    vk::raii::DescriptorSetLayout           descriptorSetLayout = nullptr;
    vk::raii::DescriptorPool                descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet>    descriptorSets;
  public: 
    ShaderModule(const std::string& filename, vk::raii::Device& device);

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
    void createDescriptorSets();

    const vk::raii::ShaderModule& getModule() const {return shaderModule;}
    
};

[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, vk::raii::Device& device);
=======
[[nodiscard]] vk::raii::ShaderModule createShaderModule(
        const std::vector<char>& code, const vk::raii::Device& device);
>>>>>>> origin/better-resources


