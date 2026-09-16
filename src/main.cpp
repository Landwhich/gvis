#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <memory>

#include "platform.hpp" 
#include "renderer.hpp"

static constexpr uint32_t WIDTH = 800;
static constexpr uint32_t HEIGHT = 600;

class GvisApplication {
    Platform* platform = nullptr;
    std::unique_ptr<Renderer> renderer = nullptr;
    
public:
    void run(){
        createPlatform();
        renderer = std::make_unique<Renderer>();
        renderer->Initialize(platform);
        mainLoop();

        cleanup();
    }

    void createPlatform(){
        platform = Platform::getInstance();
        if(platform->initWindow(WIDTH, HEIGHT, "oOOOo geeegvis") != EXIT_SUCCESS){
            throw std::runtime_error("failed to create glfw window");
        }
    }


private:
    void mainLoop(){
    
        while (!glfwWindowShouldClose(platform->getWindow())) {
            glfwPollEvents();
            renderer->DrawFrame();
        }
        renderer->IdleDevice();
    }

    void cleanup(){
        platform->cleanupWindow();
        renderer->Cleanup();
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
