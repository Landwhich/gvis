#include "swapchain.hpp"    

vk::raii::ImageView Swapchain::createImageView(
    vk::Image const &image, 
    vk::Format format, 
    vk::ImageAspectFlags aspectFlags,
    vk::raii::Device& device,
    uint32_t mipLevels
) {
    vk::ImageViewCreateInfo viewInfo{
        .image            = image,
        .viewType         = vk::ImageViewType::e2D,
        .format           = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,  
            .baseMipLevel = 0, 
            .levelCount = mipLevels, 
            .baseArrayLayer = 0, 
            .layerCount = 1
    }};
    return vk::raii::ImageView(device, viewInfo);
}
