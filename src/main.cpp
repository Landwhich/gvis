#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "models.hpp" 
#include "platform.hpp" 
#include "renderer.hpp"
#include "vulkan_shared.h"

static constexpr uint32_t WIDTH = 800;
static constexpr uint32_t HEIGHT = 600;

class GvisApplication {
    Platform* _platform = nullptr;
    std::shared_ptr<ModelLoader> _modelLoader = nullptr;
    std::shared_ptr<Renderer> _renderer = nullptr;
    
    
public:
    GvisApplication(){
        createPlatform();
        _renderer = std::make_shared<Renderer>(_platform);
        _modelLoader = std::make_shared<ModelLoader>();
        _modelLoader->Initialize(_renderer);
        
        _renderer->Initialize();

        Model model = _modelLoader->LoadModel(gv::MODEL_PATH, gv::TEXTURE_PATH);

        _renderer->RenderModel(&model);
        _renderer->Build(); 
    }

    void run(){

        mainLoop();

        cleanup();
    }

    void createPlatform(){
        _platform = Platform::getInstance();
        if(_platform->initWindow(WIDTH, HEIGHT, "oOOOo geeegvis") != EXIT_SUCCESS){
            throw std::runtime_error("failed to create glfw window");
        }
    }


private:
    void mainLoop(){
    
        while (!glfwWindowShouldClose(_platform->getWindow())) {
            glfwPollEvents();
            _renderer->DrawFrame();
        }
        _renderer->IdleDevice();
    }

    void cleanup(){
        _platform->cleanupWindow();
        _renderer->Cleanup();
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
