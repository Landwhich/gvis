#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <array>
// uses range of 0 to 1 instead of default -1 to 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

/*
 */
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    /*
     * Vertex bindings describe the "rate" at which vertices are parsed
     * in a sense. things like insatancing stride bytes etc...
     */
    static vk::VertexInputBindingDescription getBindingDescription();
    /*
     * Vertex attributes are each grouping of data: pos, colour, tex etc...
     */
    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions();

};

[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, vk::raii::Device& device);


