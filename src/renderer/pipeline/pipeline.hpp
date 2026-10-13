#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include "descriptor_manager.hpp"

class Pipeline{
    const vk::raii::Device&                 device;
    std::unique_ptr<DescriptorManager>      descriptorManager = nullptr;

    vk::raii::Pipeline                      graphicsPipeline = nullptr; 
    vk::raii::PipelineLayout                pipelineLayout = nullptr;

  public:
    Pipeline(const vk::raii::Device& device);
    ~Pipeline() = default; 

    void DescriptorSetup();
    void Create(vk::SurfaceFormatKHR swapChainSurfaceFormat, vk::Format depthFormat);
    void GenDescriptorSet(
        const std::vector<vk::raii::Buffer>& uniformBuffers,
        const vk::raii::ImageView& textureImageView, 
        const vk::raii::Sampler& textureSampler) const; 

    const vk::raii::Pipeline& getPipeline() const {return graphicsPipeline;}
    const vk::raii::PipelineLayout& getLayout() const {return pipelineLayout;}
    const vk::raii::DescriptorPool& getDescriptorPool() const {
        return descriptorManager->getPool();}  
    const std::vector<vk::raii::DescriptorSet>& getDescriptorSets() const {
        return descriptorManager->getSets();}
    const vk::raii::DescriptorSetLayout& getDescriptorLayout() const {
        return descriptorManager->getLayout();}
};
