#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "models.hpp" 
#include "platform.hpp" 
#include "renderer.hpp"
#include "resource_manager.hpp"
#include "materials.hpp"
#include "vulkan_shared.h"

namespace gv = GVIS;

static constexpr uint32_t WIDTH = 800;
static constexpr uint32_t HEIGHT = 600;

class GvisApplication {
    Platform* platform = nullptr;
    std::shared_ptr<ModelLoader> modelLoader = nullptr;
    std::shared_ptr<Renderer> renderer = nullptr;
    std::unique_ptr<ResourceManager> resourceManager = nullptr;
    
public:
    GvisApplication(){

        createPlatform();
        renderer = std::make_shared<Renderer>(platform);
        renderer->Initialize();

        resourceManager = std::make_unique<ResourceManager>(renderer->getContext()); 
        auto pVikingTex = resourceManager->LoadResource<Texture>("viking_tex");
        
        

        modelLoader = std::make_shared<ModelLoader>();
        modelLoader->Initialize(renderer);
        Model model = modelLoader->LoadModel(gv::MODEL_PATH, gv::TEXTURE_PATH);

        renderer->RenderModel(&model);
        renderer->Build();
    }

    void run(){

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
