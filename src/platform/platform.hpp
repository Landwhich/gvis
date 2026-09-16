#pragma once
#define GLFW_INCLUDE_VULKAN
#include <functional>
#include <GLFW/glfw3.h>
#include <string>
#include <vulkan/vulkan_raii.hpp>

class Platform{
private:
    GLFWwindow* window;
    Platform();
    
    uint32_t      windowWidth;
    uint32_t      windowHeight; 
    std::string   windowName; 
    std::function<void(int, int)> resize_cb_;
     
public:
    static Platform* getInstance();
    // keeping deleted methods public 
    // this results in better error handling
    Platform(Platform const&)     = delete;
    void operator= (Platform const&)   = delete;

    GLFWwindow* getWindow() const {return window;}

    ~Platform();

    bool initWindow(
        uint32_t        width
        , uint32_t      height 
        , std::string   name 
        , GLFWmonitor*  monitor = nullptr 
        , GLFWwindow*   share   = nullptr 
    );
    bool createWindowSurface(vk::raii::Instance* instance, VkSurfaceKHR* surface);

    void* getWindowUserPointer();
    void setWindowUserPointer(void* renderer);
    void setResizeCallback(std::function<void(int, int)> cb);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    std::pair<uint32_t, const char**> getPlatformExtensions();
    void getFramebufferSize(int* width, int* height);
    bool shouldClose();
    void wait();


    void cleanupWindow();
};
