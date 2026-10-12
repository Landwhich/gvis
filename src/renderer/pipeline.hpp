#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

class Pipeline{
    vk::raii::Device&                       device;                       

    vk::raii::Pipeline                      graphicsPipeline = nullptr; 

  public:
    Pipeline(vk::raii::Device& device);
    ~Pipeline() = default; 

    void Create();
};
