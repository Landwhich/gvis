#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vector>

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct ShaderModule{
    vk::raii::ShaderModule                  shaderModule = nullptr;

    ShaderModule(const std::string& filename, const vk::raii::Device& device);

  private: 
//    const vk::raii::Device&                 device;
};

[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code,      const vk::raii::Device& device);

