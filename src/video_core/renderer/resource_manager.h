// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <utility>
#include <vector>
#include "common/common_types.h"
#include "video_core/renderer/shader_util.h"
#include "video_core/renderer/state.h"

class Texture : private NonCopyable {
public:
    Texture() = default;

    Texture(Texture&& o) : handle{std::exchange(o.handle, 0)} {}

    ~Texture() {
        Release();
    }

    Texture& operator=(Texture&& o) {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create() {
        if (handle != 0)
            return;
        glGenTextures(1, &handle);
    }

    /// Deletes the internal OpenGL resource
    void Release() {
        if (handle == 0)
            return;
        glDeleteTextures(1, &handle);
        OpenGLState::GetCurState().ResetTexture(handle).Apply();
        handle = 0;
    }

    GLuint handle{};
};

class Sampler : private NonCopyable {
public:
    Sampler() = default;

    Sampler(Sampler&& o) : handle{std::exchange(o.handle, 0)} {}

    ~Sampler() {
        Release();
    }

    Sampler& operator=(Sampler&& o) {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create() {
        if (handle != 0)
            return;
        glGenSamplers(1, &handle);
    }

    /// Deletes the internal OpenGL resource
    void Release() {
        if (handle == 0)
            return;
        glDeleteSamplers(1, &handle);
        OpenGLState::GetCurState().ResetSampler(handle).Apply();
        handle = 0;
    }

    GLuint handle{};
};

class Shader : private NonCopyable {
public:
    Shader() = default;

    Shader(Shader&& o) : handle{std::exchange(o.handle, 0)} {}

    ~Shader() {
        Release();
    }

    Shader& operator=(Shader&& o) {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    void Create(const char* source, GLenum type) {
        if (handle != 0)
            return;
        if (!source)
            return;
        handle = GLShader::LoadShader(source, type);
    }

    void Release() {
        if (handle == 0)
            return;
        glDeleteShader(handle);
        handle = 0;
    }

    GLuint handle{};
};

class Program : private NonCopyable {
public:
    Program() = default;

    Program(Program&& o) : handle{std::exchange(o.handle, 0)} {}

    ~Program() {
        Release();
    }

    Program& operator=(Program&& o) {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new program from given shader objects
    void Create(bool separable_program, const std::vector<GLuint>& shaders) {
        if (handle != 0)
            return;
        handle = GLShader::LoadProgram(separable_program, shaders);
    }

    /// Creates a new program from given shader source code
    void Create(const char* vert_shader, const char* frag_shader) {
        Shader vert, frag;
        vert.Create(vert_shader, GL_VERTEX_SHADER);
        frag.Create(frag_shader, GL_FRAGMENT_SHADER);
        Create(false, {vert.handle, frag.handle});
    }

    /// Deletes the internal OpenGL resource
    void Release() {
        if (handle == 0)
            return;
        glDeleteProgram(handle);
        OpenGLState::GetCurState().ResetProgram(handle).Apply();
        handle = 0;
    }

    GLuint handle{};
};

class Pipeline : private NonCopyable {
public:
    Pipeline() = default;

    Pipeline(Pipeline&& o) {
        handle = std::exchange<GLuint>(o.handle, 0);
    }

    ~Pipeline() {
        Release();
    }

    Pipeline& operator=(Pipeline&& o) {
        Release();
        handle = std::exchange<GLuint>(o.handle, 0);
        return *this;
    }

    void Create() {
        if (handle != 0)
            return;
        glGenProgramPipelines(1, &handle);
    }

    void Release() {
        if (handle == 0)
            return;
        glDeleteProgramPipelines(1, &handle);
        OpenGLState::GetCurState().ResetPipeline(handle).Apply();
        handle = 0;
    }

    GLuint handle{};
};

class Buffer : private NonCopyable {
public:
    Buffer() = default;

    Buffer(Buffer&& o) : handle{std::exchange(o.handle, 0)} {}

    ~Buffer() {
        Release();
    }

    Buffer& operator=(Buffer&& o) {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create() {
        if (handle != 0)
            return;
        glGenBuffers(1, &handle);
    }

    /// Deletes the internal OpenGL resource
    void Release() {
        if (handle == 0)
            return;
        glDeleteBuffers(1, &handle);
        OpenGLState::GetCurState().ResetBuffer(handle).Apply();
        handle = 0;
    }

    GLuint handle{};
};

class Sync : private NonCopyable {
public:
    Sync() = default;

    Sync(Sync&& o) : handle{std::exchange(o.handle, nullptr)} {}

    ~Sync() {
        Release();
    }

    Sync& operator=(Sync&& o) {
        Release();
        handle = std::exchange(o.handle, nullptr);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create() {
        if (handle != 0)
            return;
        handle = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    /// Deletes the internal OpenGL resource
    void Release() {
        if (handle == 0)
            return;
        glDeleteSync(handle);
        handle = 0;
    }

    GLsync handle{};
};

class VertexArray : private NonCopyable {
public:
    VertexArray() = default;

    VertexArray(VertexArray&& o) : handle{std::exchange(o.handle, 0)} {}

    ~VertexArray() {
        Release();
    }

    VertexArray& operator=(VertexArray&& o) {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create() {
        if (handle != 0)
            return;
        glGenVertexArrays(1, &handle);
    }

    /// Deletes the internal OpenGL resource
    void Release() {
        if (handle == 0)
            return;
        glDeleteVertexArrays(1, &handle);
        OpenGLState::GetCurState().ResetVertexArray(handle).Apply();
        handle = 0;
    }

    GLuint handle{};
};

class Framebuffer : private NonCopyable {
public:
    Framebuffer() = default;

    Framebuffer(Framebuffer&& o) : handle{std::exchange(o.handle, 0)} {}

    ~Framebuffer() {
        Release();
    }

    Framebuffer& operator=(Framebuffer&& o) {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create() {
        if (handle != 0)
            return;
        glGenFramebuffers(1, &handle);
    }

    /// Deletes the internal OpenGL resource
    void Release() {
        if (handle == 0)
            return;
        glDeleteFramebuffers(1, &handle);
        OpenGLState::GetCurState().ResetFramebuffer(handle).Apply();
        handle = 0;
    }

    GLuint handle{};
};
