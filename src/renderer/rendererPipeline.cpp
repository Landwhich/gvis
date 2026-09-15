#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "file.hpp"
#include "renderer.hpp"
#include "shaders/shaders.hpp"

// struct Vertex {
// 
//     glm::vec3 pos;
//     glm::vec3 color;
//     glm::vec2 texCoord;
// 
//     bool operator==(const Vertex& other) const {
//         return pos == other.pos && color == other.color && texCoord == other.texCoord;
//     }
// 
//     static vk::VertexInputBindingDescription getBindingDescription() {
//         return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
//     }
// 
//     static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
//         return {{
//             {.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, pos)},
//             {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)},
//             {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, texCoord)}
//         }};
//     }
// };

void Renderer::createGraphicsPipeline(){
    
    vk::raii::ShaderModule shaderModule = createShaderModule(readFile("shaders/slang.spv"), device);
 
    // info given to pipeline to describe the shaders used 
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ 
        .stage = vk::ShaderStageFlagBits::eVertex, 
        .module = shaderModule,  
        .pName = "vertMain" 
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ 
        .stage = vk::ShaderStageFlagBits::eFragment, 
        .module = shaderModule, 
        .pName = "fragMain" 
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
 
    // bindings and attributes useful for custom vector use, like with instancing
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions    = attributeDescriptions.data()
    };  
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList
    }; 

    // TODO use MSAA
    vk::SampleCountFlagBits msaaSamples{vk::SampleCountFlagBits::e1}; 
    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = msaaSamples, 
        .sampleShadingEnable = vk::False
    };
 
    // dynamic state is configurable in vulkan without rebuilding the pipeline
    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), 
        .pDynamicStates = dynamicStates.data()
    };

    // some of the few configurable vulkan pipeline states: viewpoert and scissor
    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};
 
    // performs most of the built-in culling like depth and bf
    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        // .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0f
    };    

    // for blending colours between current and past frambeuffer colour data
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable    = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR                                           
                        | vk::ColorComponentFlagBits::eG 
                        | vk::ColorComponentFlagBits::eB 
                        | vk::ColorComponentFlagBits::eA
    };
    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False, 
        .logicOp = vk::LogicOp::eCopy, 
        .attachmentCount = 1, 
        .pAttachments = &colorBlendAttachment
    };
     
    // used for shader uniforms. required even if not
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 1, 
        .pSetLayouts = &*descriptorSetLayout,  
        .pushConstantRangeCount = 0
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable       = vk::False,
        .depthWriteEnable      = vk::False,
        .depthCompareOp        = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable     = vk::False
    };

    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
     
        // vk::Format depthFormat = findDepthFormat();
        // vk::Format depthFormat = 1 ;
     
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
       {
        .stageCount          = 2,
        .pStages             = shaderStages,
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pDepthStencilState  = &depthStencil,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = pipelineLayout,
        .renderPass          = nullptr
       },
      {
        .colorAttachmentCount = 1, 
        .pColorAttachmentFormats = &swapChainSurfaceFormat.format, 
        // .depthAttachmentFormat = depthFormat
       }
    }; 
 
    // TODO: explore pipeline caching options
    graphicsPipeline = vk::raii::Pipeline(
        device, 
        nullptr, 
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()
    );
}

[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code, vk::raii::Device device)  {
    vk::ShaderModuleCreateInfo createInfo{ 
        .codeSize = code.size() * sizeof(char), 
        .pCode = reinterpret_cast<const uint32_t*>(code.data()) 
    };
    vk::raii::ShaderModule shaderModule{ device, createInfo };

    return shaderModule;
}



