#include "platform.hpp"
#include <stdexcept>

// private:
Platform::Platform() { 
    if (!glfwInit()){
        throw std::runtime_error("failed to instantiate glfw");
    }
    window = nullptr; 
}

// public:
Platform* Platform::getInstance(){
    static Platform instance;
    return &instance;
}
// keeping deleted methods public 
// this results in better error handling
// they are defined in header
Platform::~Platform() {
    cleanupWindow(); 
    glfwTerminate();
}

bool Platform::initWindow(
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    this->window = glfwCreateWindow(width, height, name.c_str(), monitor, share);

    if (!this->window) { 
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void* Platform::getWindowUserPointer(){
    return glfwGetWindowUserPointer(this->window);
}

void Platform::setWindowUserPointer(void* renderer){
    glfwSetWindowUserPointer(window, renderer);
}

// Platform still owns the one GLFW callback slot
void Platform::setResizeCallback(std::function<void(int, int)> cb) {
    resize_cb_ = std::move(cb);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, &Platform::framebufferSizeCallback);
}

void Platform::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* platform = static_cast<Platform*>(glfwGetWindowUserPointer(window));
    if (platform && platform->resize_cb_) platform->resize_cb_(width, height);
}

std::pair<uint32_t, const char**> Platform::getPlatformExtensions(){
    uint32_t glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    return  {glfwExtensionCount, glfwExtensions}; 
}

bool Platform::createWindowSurface(vk::raii::Instance* instance, VkSurfaceKHR* surface){
    if (glfwCreateWindowSurface(**instance, this->window, nullptr, surface)){
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

bool Platform::shouldClose(){
    return glfwWindowShouldClose(window);
}

void Platform::wait(){
    glfwWaitEvents();
}

void Platform::getFramebufferSize(int* width, int* height){
    glfwGetFramebufferSize(this->window, width, height);
}

void Platform::cleanupWindow(){
    if (this->window) glfwDestroyWindow(this->window);
    this->window = nullptr;
}

