#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <glm/glm.hpp>
#include <vector>

/*
 */
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    /*
     * Vertex bindings describe the "rate" at which vertices are parsed
     * in a sense. things like insatancing stride bytes etc...
     */
    static vk::VertexInputBindingDescription getBindingDescription();
    /*
     * Vertex attributes are each grouping of data: pos, colour, tex etc...
     */
    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions();

};

[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, vk::raii::Device& device);


