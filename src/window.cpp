#include <merely3d/window.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "shader.hpp"
#include "command_buffer.hpp"
#include "renderer.hpp"

typedef void(*GlfwWindowDestroyFunc)(GLFWwindow *);
typedef std::unique_ptr<GLFWwindow, GlfwWindowDestroyFunc> GlfwWindowPtr;

namespace merely3d
{
    class Window::WindowData
    {
    public:
        WindowData(GlfwWindowPtr ptr, Renderer renderer)
            : glfw_window(std::move(ptr)),
              viewport_size(0, 0),
              renderer(std::move(renderer))
        {}

        GlfwWindowPtr glfw_window;
        std::pair<int, int> viewport_size;

        Camera camera;

        CommandBuffer command_buffer;
        Renderer renderer;
    };

    static void check_and_update_viewport_size(GLFWwindow * window, int & viewport_width, int & viewport_height)
    {
        int fb_width;
        int fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);

        if (fb_width != viewport_width || fb_height != viewport_height)
        {
            viewport_width = fb_width;
            viewport_height = fb_height;
            glViewport(0, 0, viewport_width, viewport_height);
        }
    }

    Window::Window(Window && other)
        : _d(nullptr)
    {
        if (other._d)
        {
            _d = other._d;
            other._d = nullptr;
        }
    }

    Window::~Window()
    {
        // Must check for valid data because data might have been moved
        if (_d)
        {
            // TODO: Destroy ALL vertex buffers/objects and so forth
            glfwMakeContextCurrent(_d->glfw_window.get());
            delete _d;
        }
    }

    Window::Window(WindowData * data)
    {
        assert(data);
        _d = data;
    }

    void Window::render_frame_impl(Frame & frame)
    {
        assert(_d);
        glfwMakeContextCurrent(_d->glfw_window.get());

        auto & vp_width = _d->viewport_size.first;
        auto & vp_height = _d->viewport_size.second;

        check_and_update_viewport_size(_d->glfw_window.get(), vp_width, vp_height);

        _d->renderer.render(_d->command_buffer, _d->camera, vp_width, vp_height);

        get_command_buffer()->clear();

        glfwSwapBuffers(_d->glfw_window.get());
        glfwPollEvents();
    }

    bool Window::should_close() const
    {
        return glfwWindowShouldClose(_d->glfw_window.get());
    }

    Camera & Window::camera()
    {
        return _d->camera;
    }

    const Camera & Window::camera() const
    {
        return _d->camera;
    }

    CommandBuffer * Window::get_command_buffer()
    {
        return &_d->command_buffer;
    }

    Window WindowBuilder::build() const
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // This is apparently needed on Mac OS X. Can we simply set it for all platforms...?
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        GLFWwindow * glfw_window = glfwCreateWindow(_width, _height, _title.c_str(), NULL, NULL);

        if (!glfw_window)
        {
            // TODO: Better error, more information
            throw std::runtime_error("Failed to initialize GLFWwindow");
        }

        // TODO: Should glad initialization happen in a different location...?
        // Is it safe to re-initialize GLAD?
        glfwMakeContextCurrent(glfw_window);
        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
        {
            // TODO: Better error message
            throw std::runtime_error("Failed to initialize GLAD");
        }

        auto renderer = Renderer::build();
        auto window_ptr = GlfwWindowPtr(glfw_window, glfwDestroyWindow);
        auto window_data = new Window::WindowData(std::move(window_ptr), std::move(renderer));
        auto window = Window(window_data);
        return std::move(window);
    }

}