#include "models.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#pragma GCC diagnostic pop


#include <iostream>

/*
 * tiny obj impl
 */
bool Mesh::doLoad(const std::pair<gv::DeviceContext, gv::GraphicsContext>& resourceContext){
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      warn, err;
    
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    fs::path path{("resources/models") / this->modelPath}; 

    if (!tinyobj::LoadObj(
        &attrib, &shapes, &materials, &warn, &err, path.string().c_str())
    ) {
        std::cerr << modelPath.string();
        return false;
    }

    for (const auto& shape : shapes){
        for (const auto& index : shape.mesh.indices){
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (!attrib.texcoords.empty()) {
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            vertex.color = {1.0f, 1.0f, 1.0f};

            auto [it, inserted] = uniqueVertices.insert({vertex, static_cast<uint32_t>(this->vertices.size())});
            if (inserted)
                this->vertices.push_back(vertex);

            this->indices.push_back(it->second);
        }
    }
    return true;
}

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    return {
        .binding = 0
        , .stride = sizeof(Vertex)
        , .inputRate = vk::VertexInputRate::eVertex
    };
}

std::array<vk::VertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions() {
    return {{
        {.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, pos)}
        ,   {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)}
        ,   {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, texCoord)}
    }};
}


