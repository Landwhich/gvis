#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <vector>

#include <vulkan_shared.h>

namespace gv = GVIS;

// TODO: cleanup device params
struct DeviceParams{
    std::vector<const char*> requiredExts;             
    std::vector<const char*> optionalExts;

};

// TODO: implement a queue structure for differing queue types 
//      based on params
// TODO: duplicate queue arch to provide optimized queueing for both
// graphics and transfer ops

class VulkanDevice{
  public:
    VulkanDevice(const vk::raii::Instance& instance, 
            const vk::raii::SurfaceKHR& surface, const DeviceParams& params);
    // destructor done via raii
    ~VulkanDevice() = default; 

    void idle() const {logicalDevice.waitIdle();}
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, 
        vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;
    uint32_t findMemoryType(uint32_t typeFilter, 
        vk::MemoryPropertyFlags properties) const; 

    const uint32_t& getQueueIndex() const {return queueIndex;}
    const vk::raii::Queue& getQueue() const {return graphicsQueue;}
    const vk::raii::Device& getDevice() const {return logicalDevice;}
    const vk::raii::PhysicalDevice& getPhysicalDevice() const {
        return physicalDevice;}
    const gv::DeviceContext getContext() const { return gv::DeviceContext{
            .device         = logicalDevice 
        ,   .physicalDevice = physicalDevice
        ,   .queue          = graphicsQueue
    }; }

  private:
    //vulkan params
    const vk::raii::Instance&         instance;
    const vk::raii::SurfaceKHR&       surface;

    // vulkan devices
    vk::raii::PhysicalDevice    physicalDevice = nullptr;
    vk::raii::Device            logicalDevice = nullptr;
    // assorted queues
    vk::raii::Queue             graphicsQueue = nullptr;

    // queue index needed for selection during command pool creation
    uint32_t queueIndex = ~0;

    std::vector<const char*>     deviceExtensions;

    /*
     * Finds a device which supports:
     * - required queue families
     * - the needed device EXTs
     * - needed features
     * should expand this to optimize device on higher-end PCs
     */
    void pickPhysicalDevice();
    bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice);

    /*
     * Create real vulkan device a logical interface to the physical device:
     * - check for swapchain support
     * - find supporting queue given what we need (ofs render doesn't need present func)
     * - query for features and support needed within the device
     * - query for extensions
     * - finally create device and queue 
     */
    void createLogicalDevice();
};
