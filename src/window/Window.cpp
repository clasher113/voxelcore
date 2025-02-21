#include "Window.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "debug/Logger.hpp"
#include "graphics/core/ImageData.hpp"
#include "graphics/core/Texture.hpp"
#include "settings.hpp"
#include "util/ObjectsKeeper.hpp"
#include "Events.hpp"

#ifdef USE_DIRECTX
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define NOMINMAX
#include <GLFW/glfw3native.h>
#include "../directx/window/DXDevice.hpp"
#include "../directx/util/TextureUtil.hpp"
#include "../directx/util/AdapterReader.hpp"
#elif USE_OPENGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif // USE_DIRECTX

#include "util/platform.hpp"

static debug::Logger logger("window");

GLFWwindow* Window::window = nullptr;
DisplaySettings* Window::settings = nullptr;
std::stack<glm::vec4> Window::scissorStack;
glm::vec4 Window::scissorArea;
uint Window::width = 0;
uint Window::height = 0;
int Window::posX = 0;
int Window::posY = 0;
int Window::framerate = -1;
double Window::prevSwap = 0.0;
bool Window::fullscreen = false;

static util::ObjectsKeeper observers_keeper;

void cursor_position_callback(GLFWwindow*, double xpos, double ypos) {
    Events::setPosition(xpos, ypos);
}

void mouse_button_callback(GLFWwindow*, int button, int action, int) {
    Events::setButton(button, action == GLFW_PRESS);
}

void key_callback(
    GLFWwindow*, int key, int /*scancode*/, int action, int /*mode*/
) {
    if (key == GLFW_KEY_UNKNOWN) return;
    if (action == GLFW_PRESS) {
        Events::setKey(key, true);
        Events::pressedKeys.push_back(static_cast<keycode>(key));
    } else if (action == GLFW_RELEASE) {
        Events::setKey(key, false);
    } else if (action == GLFW_REPEAT) {
        Events::pressedKeys.push_back(static_cast<keycode>(key));
    }
}

void scroll_callback(GLFWwindow*, double xoffset, double yoffset) {
    Events::scroll += yoffset;
}

bool Window::isMaximized() {
    return glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
}

bool Window::isIconified() {
    return glfwGetWindowAttrib(window, GLFW_ICONIFIED);
}

bool Window::isFocused() {
    return glfwGetWindowAttrib(window, GLFW_FOCUSED);
}

void window_size_callback(GLFWwindow*, int width, int height) {
    if (width && height) {
#ifdef USE_DIRECTX
        DXDevice::onWindowResize(width, height);
#elif USE_OPENGL
        glViewport(0, 0, width, height);
#endif // !USE_DIRECTX
        Window::width = width;
        Window::height = height;
        

        if (!Window::isFullscreen() && !Window::isMaximized()) {
            Window::getSettings()->width.set(width);
            Window::getSettings()->height.set(height);
        }
    }
    Window::resetScissor();
}

void character_callback(GLFWwindow*, unsigned int codepoint) {
    Events::codepoints.push_back(codepoint);
}

const char* glfwErrorName(int error) {
    switch (error) {
        case GLFW_NO_ERROR:
            return "no error";
        case GLFW_NOT_INITIALIZED:
            return "not initialized";
        case GLFW_NO_CURRENT_CONTEXT:
            return "no current context";
        case GLFW_INVALID_ENUM:
            return "invalid enum";
        case GLFW_INVALID_VALUE:
            return "invalid value";
        case GLFW_OUT_OF_MEMORY:
            return "out of memory";
        case GLFW_API_UNAVAILABLE:
            return "api unavailable";
        case GLFW_VERSION_UNAVAILABLE:
            return "version unavailable";
        case GLFW_PLATFORM_ERROR:
            return "platform error";
        case GLFW_FORMAT_UNAVAILABLE:
            return "format unavailable";
        case GLFW_NO_WINDOW_CONTEXT:
            return "no window context";
        default:
            return "unknown error";
    }
}

void error_callback(int error, const char* description) {
    std::cerr << "GLFW error [0x" << std::hex << error << "]: ";
    std::cerr << glfwErrorName(error) << std::endl;
    if (description) {
        std::cerr << description << std::endl;
    }
}

int Window::initialize(DisplaySettings* settings) {
    Window::settings = settings;
    Window::width = settings->width.get();
    Window::height = settings->height.get();

    std::string title = "VoxelCore v" +
                        std::to_string(ENGINE_VERSION_MAJOR) + "." +
                        std::to_string(ENGINE_VERSION_MINOR) +
#ifdef USE_DIRECTX
        " DirectX";
#elif USE_OPENGL
        " OpenGL";
#endif // USE_DIRECTX
    if (ENGINE_DEBUG_BUILD) {
        title += " [debug]";
}
    glfwSetErrorCallback(error_callback);
    if (glfwInit() == GLFW_FALSE) {
        logger.error() << "failed to initialize GLFW";
        return -1;
    }

#ifdef USE_DIRECTX
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#elif USE_OPENGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#else
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, settings->samples.get());
#endif // USE_DIRECTX

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (window == nullptr) {
        logger.error() << "failed to create GLFW window";
        glfwTerminate();
        return -1;
    }

#ifdef USE_DIRECTX
    HWND windowHandle = glfwGetWin32Window(window);
    DXDevice::initialize(windowHandle, width, height);
    DXDevice::setSwapInterval(1);

    const AdapterData& adapterData = DXDevice::getAdapterData();

    logger.info() << "DirectX Vendor: " << adapterData.m_vendor;
    logger.info() << "DirectX Renderer: " << std::string(std::begin(adapterData.m_description.Description), std::end(adapterData.m_description.Description));

#elif USE_OPENGL
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;

    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        if (glewErr == GLEW_ERROR_NO_GLX_DISPLAY) {
            // see issue #240
            logger.warning()
                << "glewInit() returned GLEW_ERROR_NO_GLX_DISPLAY; ignored";
        } else {
            logger.error() << "failed to initialize GLEW:\n"
                           << glewGetErrorString(glewErr);
            return -1;
        }
    }

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint maxTextureSize[1] {static_cast<GLint>(Texture::MAX_RESOLUTION)};
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, maxTextureSize);
    if (maxTextureSize[0] > 0) {
        Texture::MAX_RESOLUTION = maxTextureSize[0];
        logger.info() << "max texture size is " << Texture::MAX_RESOLUTION;
    }
    glfwSwapInterval(1);
    setFramerate(settings->framerate.get());

    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    logger.info() << "GL Vendor: " << (char*)vendor;
    logger.info() << "GL Renderer: " << (char*)renderer;
#endif // USE_DIRECTX
    logger.info() << "GLFW: " << glfwGetVersionString();
    glm::vec2 scale;
    glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &scale.x, &scale.y);
    logger.info() << "monitor content scale: " << scale.x << "x" << scale.y;

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetCharCallback(window, character_callback);
    glfwSetScrollCallback(window, scroll_callback);

    observers_keeper = util::ObjectsKeeper();
    observers_keeper.keepAlive(settings->fullscreen.observe(
        [=](bool value) {
            if (value != isFullscreen()) {
                toggleFullscreen();
            }
        },
        true
    ));

    input_util::initialize();
    return 0;
}

void Window::clear() {
#ifdef USE_DIRECTX
    DXDevice::clear();
#elif USE_OPENGL
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif // USE_DIRECTX
}

void Window::clearDepth() {
#ifdef USE_DIRECTX
    DXDevice::clearDepth();
#elif USE_OPENGL
    glClear(GL_DEPTH_BUFFER_BIT);
#endif // USE_DIRECTX
}

void Window::setBgColor(glm::vec3 color) {
#ifdef USE_DIRECTX
    DXDevice::setClearColor(color.r, color.g, color.b, 1.f);
#elif USE_OPENGL
    glClearColor(color.r, color.g, color.b, 1.0f);
#endif // USE_DIRECTX
}

void Window::setBgColor(glm::vec4 color) {
#ifdef USE_DIRECTX
    DXDevice::setClearColor(color.r, color.g, color.b, color.a);
#elif USE_OPENGL
    glClearColor(color.r, color.g, color.b, color.a);
#endif // USE_DIRECTX
}

void Window::viewport(int x, int y, int width, int height){
#ifdef USE_DIRECTX
    DXDevice::resizeViewPort(x, y, width, height);
#elif USE_OPENGL
    glViewport(x, y, width, height);
#endif // USE_DIRECTX
}

void Window::setCursorMode(int mode) {
    glfwSetInputMode(window, GLFW_CURSOR, mode);
}

void Window::resetScissor() {
    scissorArea = glm::vec4(0.0f, 0.0f, width, height);
    scissorStack = std::stack<glm::vec4>();
#ifdef USE_DIRECTX
    DXDevice::setScissorTest(false);
#elif USE_OPENGL
    glDisable(GL_SCISSOR_TEST);
#endif // USE_DIRECTX
}

void Window::pushScissor(glm::vec4 area) {
    if (scissorStack.empty()) {
#ifdef USE_DIRECTX
        DXDevice::setScissorTest(true);
#elif USE_OPENGL
        glEnable(GL_SCISSOR_TEST);
#endif // USE_DIRECTX
    }
    scissorStack.push(scissorArea);

    area.z += area.x;
    area.w += area.y;

    area.x = fmax(area.x, scissorArea.x);
    area.y = fmax(area.y, scissorArea.y);

    area.z = fmin(area.z, scissorArea.z);
    area.w = fmin(area.w, scissorArea.w);

    if (area.z < 0.0f || area.w < 0.0f) {
#ifdef USE_DIRECTX
        DXDevice::setScissorRect(0, 0, 0, 0);
#elif USE_OPENGL
        glScissor(0, 0, 0, 0);
#endif // USE_DIRECTX
    }
    else {
#ifdef USE_DIRECTX
        DXDevice::setScissorRect(area.x, area.y,
            std::max(0, int(area.z - area.x)),
            std::max(0, int(area.w - area.y)));
#elif USE_OPENGL
        glScissor(area.x, Window::height - area.w,
            std::max(0, int(area.z - area.x)),
            std::max(0, int(area.w - area.y)));
#endif // USE_DIRECTX
    }
    scissorArea = area;
}

void Window::popScissor() {
    if (scissorStack.empty()) {
        logger.warning() << "extra Window::popScissor call";
        return;
    }
    glm::vec4 area = scissorStack.top();
    scissorStack.pop();
    if (area.z < 0.0f || area.w < 0.0f) {
#ifdef USE_DIRECTX
        DXDevice::setScissorRect(0, 0, 0, 0);
#elif USE_OPENGL
        glScissor(0, 0, 0, 0);
#endif // USE_DIRECTX
    }
    else {
#ifdef USE_DIRECTX
        DXDevice::setScissorRect(area.x, area.y,
            std::max(0, int(area.z - area.x)),
            std::max(0, int(area.w - area.y)));
#elif USE_OPENGL
        glScissor(area.x, Window::height - area.w,
            std::max(0, int(area.z - area.x)),
            std::max(0, int(area.w - area.y)));
#endif // USE_DIRECTX
    }
    if (scissorStack.empty()) {
#ifdef USE_DIRECTX
        DXDevice::setScissorTest(false);
#elif USE_OPENGL
        glDisable(GL_SCISSOR_TEST);
#endif // USE_DIRECTX
    }
    scissorArea = area;
}

void Window::terminate() {
    observers_keeper = util::ObjectsKeeper();
#ifdef USE_DIRECTX
    DXDevice::terminate();
#endif // USE_DIRECTX
    glfwTerminate();
}

bool Window::isShouldClose() {
    return glfwWindowShouldClose(window);
}

void Window::setShouldClose(bool flag) {
    glfwSetWindowShouldClose(window, flag);
}

void Window::setFramerate(int framerate) {
    if ((framerate != -1) != (Window::framerate != -1)) {
#ifdef USE_DIRECTX
        DXDevice::setSwapInterval(framerate == -1);
#elif USE_OPENGL
        glfwSwapInterval(framerate == -1);
#endif  // USE_DIRECTX
    }
    Window::framerate = framerate;
}

void Window::toggleFullscreen() {
    fullscreen = !fullscreen;

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    if (Events::_cursor_locked) Events::toggleCursor();

    if (fullscreen) {
        glfwGetWindowPos(window, &posX, &posY);
        glfwSetWindowMonitor(
            window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE
        );
    } else {
        glfwSetWindowMonitor(
            window,
            nullptr,
            posX,
            posY,
            settings->width.get(),
            settings->height.get(),
            GLFW_DONT_CARE
        );
    }

    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    Events::setPosition(xPos, yPos);
}

bool Window::isFullscreen() {
    return fullscreen;
}

void Window::swapBuffers(){
#ifdef USE_DIRECTX
    DXDevice::display();
#elif USE_OPENGL
    glfwSwapBuffers(window);
#endif // USE_DIRECTX
    Window::resetScissor();
    if (framerate > 0) {
        auto elapsedTime = time() - prevSwap;
        auto frameTime = 1.0 / framerate;
        if (elapsedTime < frameTime) {
            platform::sleep(
                static_cast<size_t>((frameTime - elapsedTime) * 1000)
            );
        }
    }
    prevSwap = time();
}

double Window::time() {
    return glfwGetTime();
}

DisplaySettings* Window::getSettings() {
    return settings;
}

std::unique_ptr<ImageData> Window::takeScreenshot() {
#ifdef USE_DIRECTX
    auto data = std::make_unique<ubyte[]>(width * height * 4);
    auto texture = DXDevice::getSurface();
    ID3D11Texture2D* staged = nullptr;
    TextureUtil::stageTexture(texture.Get(), &staged);
    TextureUtil::readPixels(staged, data.get());
    staged->Release();
    return std::make_unique<ImageData>(
        ImageFormat::rgba8888, width, height, data.release()
    ); 
#elif USE_OPENGL
    auto data = std::make_unique<ubyte[]>(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data.get());
    return std::make_unique<ImageData>(
        ImageFormat::rgb888, width, height, data.release()
    );
#endif // USE_DIRECTX
}

const char* Window::getClipboardText() {
    return glfwGetClipboardString(window);
}

void Window::setClipboardText(const char* text) {
    glfwSetClipboardString(window, text);
}

bool Window::tryToMaximize(GLFWwindow* window, GLFWmonitor* monitor) {
    glm::ivec4 windowFrame(0);
    glm::ivec4 workArea(0);
    glfwGetWindowFrameSize(
        window, &windowFrame.x, &windowFrame.y, &windowFrame.z, &windowFrame.w
    );
    glfwGetMonitorWorkarea(
        monitor, &workArea.x, &workArea.y, &workArea.z, &workArea.w
    );
    if (Window::width > (uint)workArea.z) Window::width = (uint)workArea.z;
    if (Window::height > (uint)workArea.w) Window::height = (uint)workArea.w;
    if (Window::width >= (uint)(workArea.z - (windowFrame.x + windowFrame.z)) &&
        Window::height >=
            (uint)(workArea.w - (windowFrame.y + windowFrame.w))) {
        glfwMaximizeWindow(window);
        return true;
    }
    glfwSetWindowSize(window, Window::width, Window::height);
    glfwSetWindowPos(
        window,
        workArea.x + (workArea.z - Window::width) / 2,
        workArea.y + (workArea.w - Window::height) / 2 + windowFrame.y / 2
    );
    return false;
}

void Window::setIcon(const ImageData* image) {
    GLFWimage icon {
        static_cast<int>(image->getWidth()),
        static_cast<int>(image->getHeight()),
        image->getData()};
    glfwSetWindowIcon(window, 1, &icon);
}
