#include "Window.hpp"
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/Swapchain.hpp"
#include <cassert>

int64_t glfwCount = 0;

Window::Window(){
    if(glfwCount == 0){
        glfwInit();
    }
    glfwCount++;
}
Window::~Window(){
    swapchain.clear();
    commandPool.clear();
    
    close();
    glfwCount--;
    if(glfwCount == 0){
        glfwTerminate();
    }
}
void Window::addRequirements(InstanceSetup& vs){
    // add extensions to wishlist:
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    vs.extensions.insert(glfwExtensions,glfwExtensions+glfwExtensionCount);

    // check if the extensions are available
    vk::raii::Context context;
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfwExtensionCount; ++i)
    {
        if (std::ranges::none_of(extensionProperties,
            [glfwExtension = glfwExtensions[i]](auto const& extensionProperty)
            { return strcmp(extensionProperty.extensionName, glfwExtension) == 0; }))
        {
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
        }
    }
}
void Window::create(Instance& instance,DeviceSetup& dSetup,int width, int height, const char *title){
    // window 
    assert(!window);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width,height,title, nullptr, nullptr);
    glfwSetWindowUserPointer(window,(void*)this);

    // surface
    VkSurfaceKHR       _surface;
    if (glfwCreateWindowSurface(*(vk::raii::Instance&)instance, window, nullptr, &_surface) != 0) {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);

    gQueue.create(dSetup,*this);
    Swapchain::addRequirements(dSetup);
    dSetup.callAfterCreation.push_back(this);

    initEventCallback();
}
void Window::afterDeviceInit(class Device& device){
    commandPool.create(device, gQueue);
    swapchain.create(device,*this);
    depthBuffer.create(*this, false);
}

void Window::close(){
    glfwDestroyWindow(window);
    window = nullptr;
}
Window::operator GLFWwindow*(){
    return window;
}
void Window::initEventCallback(){
     // callback 
    glfwSetFramebufferSizeCallback(window,[](GLFWwindow* window, int width, int height){
        auto self = (Window*)glfwGetWindowUserPointer(window);
        self->shouldRecreateSwapchain = true;
        if(self->inputHandler)
            self->inputHandler->receive({
                .window = *self,
                .value  = Event::FramebufferSize{.width = width,.height = height}
            });
    });
    glfwSetKeyCallback(window,[](GLFWwindow* window, int key, int scancode, int action, int mods){
        auto self = (Window*)glfwGetWindowUserPointer(window);
        if(key == GLFW_KEY_F11 && action == GLFW_PRESS){
            self->toggleFullscreen();
        }else if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
            self->toggleMouseGrab();
        }
        if(self->inputHandler)
            self->inputHandler->receive({
                .window = *self,
                .value  = Event::Key{
                    .key = key,
                    .scancode = scancode,
                    .action = action,
                    .mods = mods
                }
            });
    });
    glfwSetCharCallback(window, [](GLFWwindow* window,unsigned int codepoint){
        auto self = (Window*)glfwGetWindowUserPointer(window);
        if(self->inputHandler)
            self->inputHandler->receive({
                .window = *self,
                .value  = Event::Char{
                    .codepoint = codepoint
                }
            });
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods){
        auto self = (Window*)glfwGetWindowUserPointer(window);
        if(self->inputHandler)
            self->inputHandler->receive({
                .window = *self,
                .value  = Event::MouseButton{
                    .button = button,
                    .action = action,
                    .mods   = mods
                }
            });
    });
    toggleMouseGrab();
}

void Window::toggleFullscreen(){
    int count;
    auto monitors =  glfwGetMonitors(&count);
    auto monitor = monitors[0];
    if(glfwGetWindowMonitor(window) == nullptr){
        const GLFWvidmode * mode = glfwGetVideoMode(monitor);
        glfwGetWindowPos(window,&beforeFullscreen.xpos,&beforeFullscreen.xpos);
        glfwGetWindowSize(window,&beforeFullscreen.sizex,&beforeFullscreen.sizey);
        glfwSetWindowMonitor(window,monitor,0,0,mode->width,mode->height,0);
        return;
    }else{
        glfwSetWindowMonitor(window,nullptr,beforeFullscreen.xpos,beforeFullscreen.ypos,beforeFullscreen.sizex,beforeFullscreen.sizey,0);
    }
}
void Window::toggleMouseGrab(){
    grabMouse =! grabMouse;
    if(grabMouse)  {
        glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    else    
        glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
}
bool Window::isMouseGrabbed()const{
    return grabMouse;
}

CommandBuffer* Window::update(){
    assert(window);
    if(glfwWindowShouldClose(window))
        return nullptr;

    if(currentCB){
        swapchain.images[swapchain.imageIndex].endRendering(*currentCB);
        currentCB->end();
        swapchain.end();
    }

    // new image
    while(!swapchain.begin());
    currentCB = &swapchain.getCommandBuffer();
    currentCB->begin();
    swapchain.images[swapchain.imageIndex].beginRendering(*currentCB,&depthBuffer);
    return currentCB;
}