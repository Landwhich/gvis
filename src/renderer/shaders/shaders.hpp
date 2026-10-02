#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <array>
// uses range of 0 to 1 instead of default -1 to 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// for custom hash function needed for vertex deduplication
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
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

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) 
                ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) 
                ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, vk::raii::Device& device);


