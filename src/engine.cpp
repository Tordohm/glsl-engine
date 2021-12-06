#include "engine.hpp"

#include <cstdarg>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <GL/glew.h>

#include "pass.hpp"

Engine::Engine(int width, int height)
{
    this->width = width;
    this->height = height;
}

Engine::~Engine()
{
    this->destroy();
}

void Engine::init()
{
    if(this->context != nullptr)
        throw std::runtime_error("Context is already initialized");

    // Initialize GLFW
    if(glfwInit() == GLFW_FALSE)
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    this->context = glfwCreateWindow(this->width, this->height, "GPU Engine", NULL, NULL);

    if(this->context == NULL)
        throw std::runtime_error("Failed to initialize window");

    glfwSetWindowUserPointer(this->context, this);

    //glfwSetFramebufferSizeCallback(context, framebuffer_size_callback);
    glfwSetKeyCallback(context, []( GLFWwindow* context, int key, int scancode, int action, int mods ) {
        auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(context));
        engine->key_callback(context, key, scancode, action, mods);
    });

    glfwMakeContextCurrent(context);

    // Initialize OpenGL
    if(glewInit() != GLEW_OK)
        throw std::runtime_error("Failed to initialize OpenGL");

    glViewport(0, 0, this->width, this->height);
    
    // Generate buffers

    glGenBuffers(1, &this->ibo); // Input Buffer Object
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->ibo);

    glGenBuffers(1, &this->wgbo); // Work Group Buffer Object
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, this->wgbo);
    glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(((EngineBuffer){0}).workGroups), &this->buffer.workGroups, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, this->wgbo);

    glGenBuffers(1, &this->dcbo); // Draw Command Buffer Object
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->dcbo);
    glBufferStorage(GL_DRAW_INDIRECT_BUFFER, 20 * sizeof(unsigned int) * 5, nullptr, GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, this->dcbo);

    glGenVertexArrays(1, &this->vao); // Vertex Array Object
    glBindVertexArray(this->vao);

    glGenBuffers(1, &this->vbo); // Vertex Buffer Object
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int) * 100, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, this->vbo);

    glGenBuffers(1, &this->ebo); // Element Buffer Object
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 100, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, this->ebo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
}

void Engine::update()
{
    if(this->context == nullptr)
        throw std::runtime_error("Context is not initialized");

    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ibo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(((EngineBuffer){0}).keyState), &this->buffer.keyState, GL_STATIC_READ);

    for(auto pass : this->passes)
    {
        glUseProgram(pass->programId);

        if(pass->getFramebufferId() != 0)
            glBindFramebuffer(GL_FRAMEBUFFER, pass->getFramebufferId());

        int i = 0;
        for(auto const& [name, location, type] : pass->textures)
        {
            if(type == GL_IMAGE_2D)
            {
                glUniform1i(location, i);
                glBindImageTexture(i, this->textures[name], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
            } 
            else if(type == GL_SAMPLER_2D)
            {
                glUniform1i(location, i);
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, this->textures[name]);
                glBindTextureUnit(i, this->textures[name]);
            }
                
            i++;
        }

        if(pass->isCompute() == true)
        {
            glDispatchComputeIndirect(0);
            glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
        else
        {
            glBindVertexArray(this->vao);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 1, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER , 0);
    }
    
    glfwPollEvents();
    glfwSwapBuffers(this->context);
}

void Engine::destroy()
{
    if(this->context == nullptr)
        return;

    for(auto pass : this->passes)
    {
        auto framebuffer = pass->getFramebufferId();
        if(framebuffer != 0)
            glDeleteFramebuffers(1, &framebuffer);
    }

    glDeleteVertexArrays(1, &this->vao);
    //glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &this->ibo);
    glDeleteBuffers(1, &this->wgbo);
    glDeleteBuffers(1, &this->dcbo);
    glDeleteBuffers(1, &this->vbo);
    glDeleteBuffers(1, &this->ebo);
    
    glfwTerminate();

    this->context = nullptr;
}

bool Engine::shouldClose()
{
    if(this->context == nullptr)
        throw std::runtime_error("Context is not initialized");

    return glfwWindowShouldClose(this->context);
}

void Engine::key_callback(GLFWwindow *context, int key, int scancode, int action, int mods)
{
    if(action == 1 || action == 2)
        this->buffer.keyState[key] = 1;
    else if(action == 0)
        this->buffer.keyState[key] = 0;
}

void Engine::loadShader(std::string filename)
{
    if(this->context == nullptr)
        throw std::runtime_error("Context is not initialized");

    std::ifstream stream(filename);

    if(stream.fail())
        throw std::invalid_argument("Could not open specified shader file");

    std::stringstream buffer;
    buffer << stream.rdbuf();

    for(size_t i = 0; i < MAX_PASS_COUNT; i++)
    {
        if(buffer.str().find("#ifdef PASS_" + std::to_string(i)) == std::string::npos)
            break;

        this->print("Compiling pass %zu\n", i);
        auto pass = new Pass(this, i);
        pass->compile(buffer.str());
        this->passes.push_back(pass);
    }
}

GLuint Engine::createTexture(std::string name, int width, int height)
{
    if(this->context == nullptr)
        throw std::runtime_error("Context is not initialized");

    if(this->textures.contains(name))
    {
        this->print("Loaded texture %s with ID: %d\n", name.c_str(), this->textures[name]);
        return this->textures[name];
    }

    this->print("Generating texture %s\n", name.c_str());

    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, GL_RGBA8, width, height);
    
    this->textures[name] = texture;
    this->print("Loaded texture %s with ID: %d\n", name.c_str(), texture);
    return texture;
}

void Engine::print(const char *format, ...)
{
    if(!this->verbose)
        return;

    va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}