#pragma once
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>

class WindowManager{
private:
    GLFWwindow* window;
    WindowManager();
    
    uint32_t      windowWidth;
    uint32_t      windowHeight; 
    std::string   windowName; 
     
public:
    static WindowManager* getInstance();
    // keeping deleted methods public 
    // this results in better error handling
    WindowManager(WindowManager const&)     = delete;
    void operator= (WindowManager const&)   = delete;

    ~WindowManager();

    int initWindow(
        uint32_t        width
        , uint32_t      height 
        , std::string   name 
        , GLFWmonitor*  monitor = nullptr 
        , GLFWwindow*   share   = nullptr 
    );
    GLFWwindow* getWindow() const {return window;}
    void cleanupWindow();
};
