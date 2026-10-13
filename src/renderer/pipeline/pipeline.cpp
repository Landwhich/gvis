#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "models.hpp"
#include "pipeline.hpp"
#include "shaders/shaders.hpp"
#include "vulkan_shared.h"

namespace gv = GVIS;

Pipeline::Pipeline(const vk::raii::Device& device) : device(device){
    if (device == nullptr) throw std::runtime_error("device is invalid");
    descriptorManager = std::make_unique<DescriptorManager>(device);
};

void Pipeline::DescriptorSetup() {
    descriptorManager->createDescriptorSetLayout(); 
}

void Pipeline::GenDescriptorSet(const std::vector<vk::raii::Buffer>& uniformBuffers,
    const vk::raii::ImageView& textureImageView, const vk::raii::Sampler& textureSampler
) const {
    descriptorManager->createDescriptorPool(); 
    descriptorManager->createDescriptorSets(uniformBuffers, textureImageView, textureSampler);
}

void Pipeline::Create(vk::SurfaceFormatKHR swapChainSurfaceFormat, vk::Format depthFormat){
    
    ShaderModule shaderModule("slang", device);

    // info given to pipeline to describe the shaders used 
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ 
        .stage = vk::ShaderStageFlagBits::eVertex, 
        .module = shaderModule.shaderModule,  
        .pName = "vertMain" 
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ 
        .stage = vk::ShaderStageFlagBits::eFragment, 
        .module = shaderModule.shaderModule, 
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
        .cullMode                = vk::CullModeFlagBits::eNone,
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

    vk::DescriptorSetLayout setLayout = *descriptorManager->getLayout();
     
    // used for shader uniforms. required even if not
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 1, 
        .pSetLayouts = &setLayout,  
        .pushConstantRangeCount = 0
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable       = vk::True,
        .depthWriteEnable      = vk::True,
        .depthCompareOp        = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable     = vk::False
    };
    // vk::Format depthFormat = findDepthFormat();

    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
     
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {{
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
    }, {
        .colorAttachmentCount = 1, 
        .pColorAttachmentFormats = &swapChainSurfaceFormat.format, 
        .depthAttachmentFormat = depthFormat
    }}; 
 
    // TODO: explore pipeline caching options
    graphicsPipeline = vk::raii::Pipeline(
        device, 
        nullptr, 
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()
    );
}

