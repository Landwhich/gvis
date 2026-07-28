#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

#include "window.h" 
#include "vulkanDevice.h"

static constexpr uint32_t WIDTH = 800;
static constexpr uint32_t HEIGHT = 600;

class GvisApplication {
    WindowManager* glfwWindowManager = nullptr;
    VulkanDevice vulkanDevice{};
    
public:
    void run(){
        glfwWindowManager = WindowManager::getInstance();
        if(glfwWindowManager->initWindow(WIDTH, HEIGHT, "oOOOo geeegvis") != EXIT_SUCCESS){
            throw std::runtime_error("failed to create glfw window");
        }
        vulkanDevice.createVulkanInstance();
        mainLoop();

        cleanup();
    }

private:
    void mainLoop(){
        while (!glfwWindowShouldClose(glfwWindowManager->getWindow())) {
            glfwPollEvents();
        }
    }

    void cleanup(){
        glfwWindowManager->cleanupWindow();
    }
};

// throw std::runtime_error("failed");
int main(){
    GvisApplication gvis;

    try {
        gvis.run();
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
