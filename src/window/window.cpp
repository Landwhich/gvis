#include "window.h"

// private:
WindowManager::WindowManager() { 
    if (!glfwInit()){
        throw std::runtime_error("failed to instantiate glfw");
    }
    window = nullptr; 
}

// public:
WindowManager* WindowManager::getInstance(){
    static WindowManager instance;
    return &instance;
}
// keeping deleted methods public 
// this results in better error handling
// they are defined in header
WindowManager::~WindowManager() {
    cleanupWindow(); 
    glfwTerminate();
}

int WindowManager::initWindow(
    uint32_t        width
    , uint32_t      height 
    , std::string   name 
    , GLFWmonitor*  monitor 
    , GLFWwindow*   share 
) { 
    if (this->window) {
        return EXIT_FAILURE;
    }
    this->windowWidth  = width;
    this->windowHeight = height;
    this->windowName   = name;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    this->window = glfwCreateWindow(width, height, name.c_str(), monitor, share);
    if (!this->window) { 
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void WindowManager::cleanupWindow(){
    if (this->window) glfwDestroyWindow(this->window);
    this->window = nullptr;
}

