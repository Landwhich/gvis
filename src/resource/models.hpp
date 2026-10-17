#pragma once

// uses range of 0 to 1 instead of default -1 to 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// for custom hash function needed for vertex deduplication
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "resource_manager.hpp"
#include "materials.hpp"

namespace fs = std::filesystem;

// forward decls
class Renderer;
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

struct Mesh : public Resource {
    explicit Mesh(const std::string& id, const fs::path& path) 
        : Resource(id), modelPath(path) {};
    ~Mesh() { Unload(); }

    bool doLoad(const std::pair<gv::DeviceContext, gv::GraphicsContext>& 
            resourceContext) override;
    void doUnload() override {};

    std::vector<uint32_t>indices; 
    std::vector<Vertex>vertices; 

    const fs::path& modelPath;
};

struct Model {
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Mesh> mesh;
};
