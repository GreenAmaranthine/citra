// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <glad/glad.h>
#include "common/common_types.h"
#include "common/math_util.h"
#include "core/hw/gpu.h"
#include "video_core/renderer/rasterizer.h"
#include "video_core/renderer/resource_manager.h"
#include "video_core/renderer/state.h"

namespace Layout {
struct FramebufferLayout;
} // namespace Layout

namespace Core {
class System;
} // namespace Core

/// Structure used for storing information about the textures for each console screen
struct TextureInfo {
    Texture resource;
    GLsizei width;
    GLsizei height;
    GPU::Regs::PixelFormat format;
    GLenum gl_format;
    GLenum gl_type;
};

/// Structure used for storing information about the display target for each console screen
struct ScreenInfo {
    GLuint display_texture;
    MathUtil::Rectangle<float> display_texcoords;
    TextureInfo texture;
};

class Renderer {
public:
    explicit Renderer(Core::System& system);
    ~Renderer();

    /// Swap buffers (render frame)
    void SwapBuffers();

    /// Initialize the renderer
    Core::System::ResultStatus Init();

    void UpdateCurrentFramebufferLayout();

    Rasterizer* GetRasterizer();

private:
    void InitOpenGLObjects();
    void ConfigureFramebufferTexture(TextureInfo& texture,
                                     const GPU::Regs::FramebufferConfig& framebuffer);
    void DrawScreens(const Layout::FramebufferLayout& layout);
    void DrawSingleScreenRotated(const ScreenInfo& screen_info, float x, float y, float w, float h);

    // Loads framebuffer from emulated memory into the display information structure
    void LoadFBToScreenInfo(const GPU::Regs::FramebufferConfig& framebuffer,
                            ScreenInfo& screen_info, bool right_eye);
    // Fills active OpenGL texture with the given RGB color.
    void LoadColorToActiveGLTexture(u8 color_r, u8 color_g, u8 color_b, const TextureInfo& texture);

    OpenGLState state;

    // OpenGL object IDs
    VertexArray vertex_array;
    Buffer vertex_buffer;
    Program shader;
    Framebuffer screenshot_framebuffer;

    /// Display information for top and bottom screens respectively
    std::array<ScreenInfo, 3> screen_infos;

    // Shader uniform location indices
    GLuint uniform_modelview_matrix;
    GLuint uniform_color_texture;

    // Shader attribute input indices
    GLuint attrib_position;
    GLuint attrib_tex_coord;

    Core::System& system;
    std::unique_ptr<Rasterizer> rasterizer;
};
