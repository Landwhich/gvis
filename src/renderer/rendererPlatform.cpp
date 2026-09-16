#include <renderer.hpp>

// static is used cause glfw sucks at in interpretting per-object "this"
// therefor have to use a static class method
void Renderer::framebufferResizeCallback(int width, int height) {
    this->framebufferResized = true;
}
