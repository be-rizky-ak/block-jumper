#include "GameOverUI.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "Aion/Core/Input.h"
#include "Aion/Core/KeyCodes.h"
#include "Aion/Core/MouseCodes.h"
#include "glad/glad.h"

namespace BlockJumper
{
    namespace
    {
        // 5x7 bitmap font definitions for uppercase alphanumeric and common symbols
        // Each character is represented by 7 rows, where each row has 5 bits (bit 4 to bit 0).
        uint8_t GetGlyphRow(char c, int row)
        {
            if (row < 0 || row >= 7) return 0;
            char uc = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

            switch (uc)
            {
                case 'A': {
                    static const uint8_t g[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
                    return g[row];
                }
                case 'B': {
                    static const uint8_t g[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
                    return g[row];
                }
                case 'C': {
                    static const uint8_t g[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
                    return g[row];
                }
                case 'D': {
                    static const uint8_t g[7] = {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C};
                    return g[row];
                }
                case 'E': {
                    static const uint8_t g[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
                    return g[row];
                }
                case 'F': {
                    static const uint8_t g[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
                    return g[row];
                }
                case 'G': {
                    static const uint8_t g[7] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F};
                    return g[row];
                }
                case 'H': {
                    static const uint8_t g[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
                    return g[row];
                }
                case 'I': {
                    static const uint8_t g[7] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
                    return g[row];
                }
                case 'J': {
                    static const uint8_t g[7] = {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C};
                    return g[row];
                }
                case 'K': {
                    static const uint8_t g[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
                    return g[row];
                }
                case 'L': {
                    static const uint8_t g[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
                    return g[row];
                }
                case 'M': {
                    static const uint8_t g[7] = {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11};
                    return g[row];
                }
                case 'N': {
                    static const uint8_t g[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
                    return g[row];
                }
                case 'O': {
                    static const uint8_t g[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
                    return g[row];
                }
                case 'P': {
                    static const uint8_t g[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
                    return g[row];
                }
                case 'Q': {
                    static const uint8_t g[7] = {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
                    return g[row];
                }
                case 'R': {
                    static const uint8_t g[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
                    return g[row];
                }
                case 'S': {
                    static const uint8_t g[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
                    return g[row];
                }
                case 'T': {
                    static const uint8_t g[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
                    return g[row];
                }
                case 'U': {
                    static const uint8_t g[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
                    return g[row];
                }
                case 'V': {
                    static const uint8_t g[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
                    return g[row];
                }
                case 'W': {
                    static const uint8_t g[7] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
                    return g[row];
                }
                case 'X': {
                    static const uint8_t g[7] = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
                    return g[row];
                }
                case 'Y': {
                    static const uint8_t g[7] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
                    return g[row];
                }
                case 'Z': {
                    static const uint8_t g[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
                    return g[row];
                }
                case '0': {
                    static const uint8_t g[7] = {0x0E, 0x13, 0x15, 0x19, 0x11, 0x11, 0x0E};
                    return g[row];
                }
                case '1': {
                    static const uint8_t g[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
                    return g[row];
                }
                case '2': {
                    static const uint8_t g[7] = {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F};
                    return g[row];
                }
                case '3': {
                    static const uint8_t g[7] = {0x1F, 0x02, 0x04, 0x0E, 0x01, 0x11, 0x0E};
                    return g[row];
                }
                case '4': {
                    static const uint8_t g[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
                    return g[row];
                }
                case '5': {
                    static const uint8_t g[7] = {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E};
                    return g[row];
                }
                case '6': {
                    static const uint8_t g[7] = {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E};
                    return g[row];
                }
                case '7': {
                    static const uint8_t g[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
                    return g[row];
                }
                case '8': {
                    static const uint8_t g[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
                    return g[row];
                }
                case '9': {
                    static const uint8_t g[7] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E};
                    return g[row];
                }
                case ':': {
                    static const uint8_t g[7] = {0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00};
                    return g[row];
                }
                case '-': {
                    static const uint8_t g[7] = {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
                    return g[row];
                }
                case '[': {
                    static const uint8_t g[7] = {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E};
                    return g[row];
                }
                case ']': {
                    static const uint8_t g[7] = {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E};
                    return g[row];
                }
                case '|': {
                    static const uint8_t g[7] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
                    return g[row];
                }
                case '/': {
                    static const uint8_t g[7] = {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10};
                    return g[row];
                }
                case '!': {
                    static const uint8_t g[7] = {0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00};
                    return g[row];
                }
                case '*': {
                    static const uint8_t g[7] = {0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00};
                    return g[row];
                }
                case '.': {
                    static const uint8_t g[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00};
                    return g[row];
                }
                default:
                    return 0;
            }
        }

        const char* s_VertexShaderSource = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;

out vec4 vColor;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)";

        const char* s_FragmentShaderSource = R"(#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vColor;
}
)";
    } // namespace

    GameOverUI::~GameOverUI()
    {
        Shutdown();
    }

    void GameOverUI::Initialize()
    {
        if (m_initialized)
        {
            return;
        }

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &s_VertexShaderSource, nullptr);
        glCompileShader(vs);

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &s_FragmentShaderSource, nullptr);
        glCompileShader(fs);

        m_shaderProgram = glCreateProgram();
        glAttachShader(m_shaderProgram, vs);
        glAttachShader(m_shaderProgram, fs);
        glLinkProgram(m_shaderProgram);

        glDeleteShader(vs);
        glDeleteShader(fs);

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

        // Position: 2 floats
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));

        // Color: 4 floats
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        m_initialized = true;
    }

    void GameOverUI::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        if (m_vbo != 0)
        {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }
        if (m_vao != 0)
        {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }
        if (m_shaderProgram != 0)
        {
            glDeleteProgram(m_shaderProgram);
            m_shaderProgram = 0;
        }

        m_initialized = false;
    }

    void GameOverUI::EnsureInitialized()
    {
        if (!m_initialized)
        {
            Initialize();
        }
    }

    bool GameOverUI::IsMouseOverButton(float mouseX, float mouseY, int screenWidth, int screenHeight) const
    {
        if (screenWidth <= 0 || screenHeight <= 0)
        {
            return false;
        }

        float ndcX = (mouseX / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
        float ndcY = 1.0f - (mouseY / static_cast<float>(screenHeight)) * 2.0f;

        return (ndcX >= m_btnMinX && ndcX <= m_btnMaxX && ndcY >= m_btnMinY && ndcY <= m_btnMaxY);
    }

    bool GameOverUI::CheckRetryAction(int screenWidth, int screenHeight) const
    {
        // 1. Keyboard trigger: R, Space, or Enter
        if (Aion::Input::GetKey(Aion::Key::R) ||
            Aion::Input::GetKey(Aion::Key::Space) ||
            Aion::Input::GetKey(Aion::Key::Enter))
        {
            return true;
        }

        // 2. Mouse click on Retry button or card
        if (Aion::Input::GetMouseButton(Aion::Mouse::ButtonLeft))
        {
            float mx = Aion::Input::GetMouseX();
            float my = Aion::Input::GetMouseY();
            if (IsMouseOverButton(mx, my, screenWidth, screenHeight))
            {
                return true;
            }
        }

        return false;
    }

    void GameOverUI::AddQuad(
        float x0, float y0, float x1, float y1,
        const Aion::Vector4& color,
        std::vector<float>& vertexBuffer)
    {
        // 2 triangles = 6 vertices (x, y, r, g, b, a)
        float r = color.x;
        float g = color.y;
        float b = color.z;
        float a = color.w;

        float quadVerts[36] = {
            x0, y0, r, g, b, a,
            x1, y0, r, g, b, a,
            x1, y1, r, g, b, a,

            x0, y0, r, g, b, a,
            x1, y1, r, g, b, a,
            x0, y1, r, g, b, a,
        };

        vertexBuffer.insert(vertexBuffer.end(), quadVerts, quadVerts + 36);
    }

    void GameOverUI::DrawGlyph(
        char c,
        float x, float y,
        float pixelW, float pixelH,
        const Aion::Vector4& color,
        std::vector<float>& vertexBuffer)
    {
        for (int row = 0; row < 7; ++row)
        {
            uint8_t rowBits = GetGlyphRow(c, row);
            float py0 = y - static_cast<float>(row + 1) * pixelH;
            float py1 = y - static_cast<float>(row) * pixelH;

            for (int col = 0; col < 5; ++col)
            {
                if (rowBits & (1 << (4 - col)))
                {
                    float px0 = x + static_cast<float>(col) * pixelW;
                    float px1 = px0 + pixelW;
                    AddQuad(px0, py0, px1, py1, color, vertexBuffer);
                }
            }
        }
    }

    void GameOverUI::DrawString(
        const std::string& text,
        float startX, float startY,
        float pixelW, float pixelH,
        float letterSpacing,
        const Aion::Vector4& color,
        std::vector<float>& vertexBuffer)
    {
        float curX = startX;
        for (char c : text)
        {
            if (c == ' ')
            {
                curX += (5.0f * pixelW) + letterSpacing;
                continue;
            }

            DrawGlyph(c, curX, startY, pixelW, pixelH, color, vertexBuffer);
            curX += (5.0f * pixelW) + letterSpacing;
        }
    }

    void GameOverUI::RenderHUD(
        int screenWidth,
        int screenHeight,
        float currentDistance,
        int difficultyStage,
        const char* stageName,
        float bestDistance)
    {
        if (screenWidth <= 0 || screenHeight <= 0)
        {
            return;
        }

        EnsureInitialized();

        // Backup relevant OpenGL state
        GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
        GLint prevBlendSrc = GL_ONE, prevBlendDst = GL_ZERO;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendSrc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendDst);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        std::vector<float> verts;
        verts.reserve(1024);

        float aspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
        float basePixelH = 2.0f / static_cast<float>(screenHeight);
        float basePixelW = basePixelH / aspect;

        // Compact HUD panel in top-left
        float hudX0 = -0.96f;
        float hudX1 = -0.42f;
        float hudY0 = 0.77f;
        float hudY1 = 0.95f;

        // Shadow behind HUD panel
        AddQuad(hudX0 + 0.01f, hudY0 - 0.015f, hudX1 + 0.01f, hudY1 - 0.015f,
                Aion::Vector4(0.02f, 0.03f, 0.04f, 0.40f), verts);

        // Panel bevel edges
        float borderW = 0.006f;
        AddQuad(hudX0, hudY1 - borderW, hudX1, hudY1, Aion::Vector4(0.32f, 0.68f, 0.55f, 0.90f), verts);
        AddQuad(hudX0, hudY0, hudX0 + borderW, hudY1, Aion::Vector4(0.32f, 0.68f, 0.55f, 0.70f), verts);
        AddQuad(hudX0, hudY0, hudX1, hudY0 + borderW, Aion::Vector4(0.03f, 0.04f, 0.06f, 0.90f), verts);
        AddQuad(hudX1 - borderW, hudY0, hudX1, hudY1, Aion::Vector4(0.03f, 0.04f, 0.06f, 0.90f), verts);

        // Panel body
        AddQuad(hudX0 + borderW, hudY0 + borderW, hudX1 - borderW, hudY1 - borderW,
                Aion::Vector4(0.08f, 0.12f, 0.16f, 0.82f), verts);

        // Format strings
        int intDist = static_cast<int>(std::floor(std::max(0.0f, currentDistance)));
        int intBest = static_cast<int>(std::floor(std::max(0.0f, bestDistance)));

        std::string distStr = "DIST: " + std::to_string(intDist) + "M";
        if (intBest > 0)
        {
            distStr += "   BEST: " + std::to_string(intBest) + "M";
        }

        std::string stageStr = "STAGE " + std::to_string(difficultyStage) +
                               " [" + std::string(stageName ? stageName : "EASY") + "]";

        float hudPixelW = basePixelW * 2.1f;
        float hudPixelH = basePixelH * 2.1f;
        float hudSpacing = hudPixelW * 1.15f;

        float textStartX = hudX0 + 0.025f;
        float textLine1Y = 0.91f;
        float textLine2Y = 0.83f;

        // Line 1: Distance & Best
        DrawString(distStr, textStartX + 0.002f, textLine1Y - 0.002f,
                   hudPixelW, hudPixelH, hudSpacing,
                   Aion::Vector4(0.02f, 0.03f, 0.05f, 0.85f), verts);
        DrawString(distStr, textStartX, textLine1Y,
                   hudPixelW, hudPixelH, hudSpacing,
                   Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f), verts);

        // Line 2: Difficulty Stage
        DrawString(stageStr, textStartX + 0.002f, textLine2Y - 0.002f,
                   hudPixelW, hudPixelH, hudSpacing,
                   Aion::Vector4(0.02f, 0.03f, 0.05f, 0.85f), verts);
        DrawString(stageStr, textStartX, textLine2Y,
                   hudPixelW, hudPixelH, hudSpacing,
                   Aion::Vector4(0.35f, 0.85f, 0.65f, 1.0f), verts);

        // Submit batch
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(float)), verts.data(), GL_DYNAMIC_DRAW);

        glUseProgram(m_shaderProgram);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size() / 6));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);

        // Restore OpenGL state
        if (!blendEnabled) glDisable(GL_BLEND);
        glBlendFunc(prevBlendSrc, prevBlendDst);
        if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
        if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    }

    void GameOverUI::RenderGameOver(
        int screenWidth,
        int screenHeight,
        float finalDistance,
        float bestDistance,
        bool isNewRecord,
        int completedSegments,
        const char* difficultyStage)
    {
        if (screenWidth <= 0 || screenHeight <= 0)
        {
            return;
        }

        EnsureInitialized();

        // Backup relevant OpenGL state
        GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
        GLint prevBlendSrc = GL_ONE, prevBlendDst = GL_ZERO;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendSrc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendDst);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        std::vector<float> verts;
        verts.reserve(2048);

        // Aspect ratio compensation for pixel width/height in NDC
        float aspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
        float basePixelH = 2.0f / static_cast<float>(screenHeight);
        float basePixelW = basePixelH / aspect;

        // 1. Fullscreen Dimming Overlay
        AddQuad(-1.0f, -1.0f, 1.0f, 1.0f, Aion::Vector4(0.06f, 0.08f, 0.12f, 0.72f), verts);

        // 2. Central Card Dialog Box
        float cardX0 = -0.48f;
        float cardX1 =  0.48f;
        float cardY0 = -0.46f;
        float cardY1 =  0.46f;

        // Drop shadow under card
        AddQuad(cardX0 + 0.02f, cardY0 - 0.025f, cardX1 + 0.02f, cardY1 - 0.025f, Aion::Vector4(0.02f, 0.03f, 0.04f, 0.50f), verts);

        // Card bevel edges (Isometric block 3D look)
        float borderW = 0.012f;
        // Top edge highlight
        AddQuad(cardX0, cardY1 - borderW, cardX1, cardY1, Aion::Vector4(0.88f, 0.48f, 0.35f, 1.0f), verts);
        // Left edge highlight
        AddQuad(cardX0, cardY0, cardX0 + borderW, cardY1, Aion::Vector4(0.88f, 0.48f, 0.35f, 1.0f), verts);
        // Bottom edge shadow
        AddQuad(cardX0, cardY0, cardX1, cardY0 + borderW, Aion::Vector4(0.03f, 0.04f, 0.06f, 1.0f), verts);
        // Right edge shadow
        AddQuad(cardX1 - borderW, cardY0, cardX1, cardY1, Aion::Vector4(0.03f, 0.04f, 0.06f, 1.0f), verts);

        // Card body (Deep slate teal)
        AddQuad(cardX0 + borderW, cardY0 + borderW, cardX1 - borderW, cardY1 - borderW,
                Aion::Vector4(0.12f, 0.16f, 0.22f, 0.96f), verts);

        // 3. Header Accent Bar
        AddQuad(cardX0 + borderW + 0.01f, 0.38f, cardX1 - borderW - 0.01f, 0.39f,
                Aion::Vector4(0.95f, 0.80f, 0.20f, 0.85f), verts);

        // 4. "GAME OVER" Title Banner
        float titlePixelW = basePixelW * 5.0f;
        float titlePixelH = basePixelH * 5.0f;
        float titleSpacing = titlePixelW * 1.5f;
        std::string titleText = "GAME OVER";
        float totalTitleWidth = (9.0f * 5.0f * titlePixelW) + (8.0f * titleSpacing);
        float titleStartX = -totalTitleWidth * 0.5f;
        float titleStartY = 0.33f;

        // Shadow behind title
        DrawString(titleText, titleStartX + 0.006f, titleStartY - 0.006f,
                   titlePixelW, titlePixelH, titleSpacing,
                   Aion::Vector4(0.04f, 0.05f, 0.08f, 0.90f), verts);

        // Vibrant Coral Title
        DrawString(titleText, titleStartX, titleStartY,
                   titlePixelW, titlePixelH, titleSpacing,
                   Aion::Vector4(1.0f, 0.48f, 0.20f, 1.0f), verts);

        // 5. New Record Feedback / Accent
        float recPixelW = basePixelW * 2.7f;
        float recPixelH = basePixelH * 2.7f;
        float recSpacing = recPixelW * 1.25f;
        if (isNewRecord)
        {
            std::string recText = "*** NEW RECORD! ***";
            float recWidth = (static_cast<float>(recText.length()) * 5.0f * recPixelW) +
                             (static_cast<float>(recText.length() - 1) * recSpacing);
            float recStartX = -recWidth * 0.5f;
            float recStartY = 0.21f;

            // Golden glowing badge banner
            AddQuad(-recWidth * 0.5f - 0.03f, recStartY - 0.02f,
                     recWidth * 0.5f + 0.03f, recStartY + 7.0f * recPixelH + 0.01f,
                    Aion::Vector4(0.35f, 0.25f, 0.05f, 0.70f), verts);

            DrawString(recText, recStartX + 0.003f, recStartY - 0.003f,
                       recPixelW, recPixelH, recSpacing,
                       Aion::Vector4(0.08f, 0.05f, 0.02f, 0.90f), verts);
            DrawString(recText, recStartX, recStartY,
                       recPixelW, recPixelH, recSpacing,
                       Aion::Vector4(1.0f, 0.88f, 0.20f, 1.0f), verts);
        }

        // 6. Run Progression Stats Summary: Distance & Best Distance
        int intDist = static_cast<int>(std::floor(std::max(0.0f, finalDistance)));
        int intBest = static_cast<int>(std::floor(std::max(0.0f, bestDistance)));

        float subPixelW = basePixelW * 2.5f;
        float subPixelH = basePixelH * 2.5f;
        float subSpacing = subPixelW * 1.2f;

        std::string distLine = "DISTANCE: " + std::to_string(intDist) + "M   BEST: " + std::to_string(intBest) + "M";
        float distWidth = (static_cast<float>(distLine.length()) * 5.0f * subPixelW) +
                          (static_cast<float>(distLine.length() - 1) * subSpacing);
        float distStartX = -distWidth * 0.5f;
        float distStartY = isNewRecord ? 0.11f : 0.17f;

        DrawString(distLine, distStartX + 0.003f, distStartY - 0.003f,
                   subPixelW, subPixelH, subSpacing,
                   Aion::Vector4(0.03f, 0.04f, 0.06f, 0.80f), verts);
        DrawString(distLine, distStartX, distStartY,
                   subPixelW, subPixelH, subSpacing,
                   Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f), verts);

        // Cleared Segments and Tier
        std::string statsLine = "CLEARED: " + std::to_string(completedSegments) +
                                " SEGMENTS | TIER: " + std::string(difficultyStage);
        float statsWidth = (static_cast<float>(statsLine.length()) * 5.0f * subPixelW) +
                           (static_cast<float>(statsLine.length() - 1) * subSpacing);
        float statsStartX = -statsWidth * 0.5f;
        float statsStartY = isNewRecord ? 0.02f : 0.06f;

        DrawString(statsLine, statsStartX, statsStartY,
                   subPixelW, subPixelH, subSpacing,
                   Aion::Vector4(0.70f, 0.88f, 0.85f, 0.95f), verts);

        // 7. Interactive Retry Button
        float mx = Aion::Input::GetMouseX();
        float my = Aion::Input::GetMouseY();
        bool hovered = IsMouseOverButton(mx, my, screenWidth, screenHeight);

        Aion::Vector4 btnColor = hovered ? Aion::Vector4(0.35f, 0.78f, 0.58f, 1.0f)
                                         : Aion::Vector4(0.25f, 0.62f, 0.45f, 1.0f);
        Aion::Vector4 btnTopLight = hovered ? Aion::Vector4(0.55f, 0.92f, 0.72f, 1.0f)
                                            : Aion::Vector4(0.40f, 0.75f, 0.55f, 1.0f);
        Aion::Vector4 btnBotShadow = hovered ? Aion::Vector4(0.20f, 0.52f, 0.38f, 1.0f)
                                             : Aion::Vector4(0.12f, 0.38f, 0.28f, 1.0f);

        // Button shadow
        AddQuad(m_btnMinX + 0.01f, m_btnMinY - 0.015f, m_btnMaxX + 0.01f, m_btnMaxY - 0.015f,
                Aion::Vector4(0.04f, 0.06f, 0.08f, 0.40f), verts);

        // Button 3D block border bevel
        float bEdge = 0.008f;
        AddQuad(m_btnMinX, m_btnMaxY - bEdge, m_btnMaxX, m_btnMaxY, btnTopLight, verts);
        AddQuad(m_btnMinX, m_btnMinY, m_btnMinX + bEdge, m_btnMaxY, btnTopLight, verts);
        AddQuad(m_btnMinX, m_btnMinY, m_btnMaxX, m_btnMinY + bEdge, btnBotShadow, verts);
        AddQuad(m_btnMaxX - bEdge, m_btnMinY, m_btnMaxX, m_btnMaxY, btnBotShadow, verts);

        // Button face
        AddQuad(m_btnMinX + bEdge, m_btnMinY + bEdge, m_btnMaxX - bEdge, m_btnMaxY - bEdge, btnColor, verts);

        // Button Text: "RETRY [R]"
        float btnPixelW = basePixelW * 3.4f;
        float btnPixelH = basePixelH * 3.4f;
        float btnSpacing = btnPixelW * 1.3f;
        std::string btnText = "RETRY [R]";
        float btnTextWidth = (static_cast<float>(btnText.length()) * 5.0f * btnPixelW) +
                             (static_cast<float>(btnText.length() - 1) * btnSpacing);
        float btnStartX = -btnTextWidth * 0.5f;
        float btnStartY = m_btnMinY + (m_btnMaxY - m_btnMinY) * 0.5f + (3.5f * btnPixelH);

        // Text shadow inside button
        DrawString(btnText, btnStartX + 0.003f, btnStartY - 0.003f,
                   btnPixelW, btnPixelH, btnSpacing,
                   Aion::Vector4(0.06f, 0.18f, 0.12f, 0.85f), verts);

        DrawString(btnText, btnStartX, btnStartY,
                   btnPixelW, btnPixelH, btnSpacing,
                   Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f), verts);

        // 8. Hint Footer: "PRESS [R] OR [SPACE] TO RETRY"
        float hintPixelW = basePixelW * 1.9f;
        float hintPixelH = basePixelH * 1.9f;
        float hintSpacing = hintPixelW * 1.1f;
        std::string hintText = "PRESS [R] OR [SPACE] TO RETRY";
        float hintWidth = (static_cast<float>(hintText.length()) * 5.0f * hintPixelW) +
                          (static_cast<float>(hintText.length() - 1) * hintSpacing);
        float hintStartX = -hintWidth * 0.5f;
        float hintStartY = -0.34f;

        DrawString(hintText, hintStartX, hintStartY,
                   hintPixelW, hintPixelH, hintSpacing,
                   Aion::Vector4(0.85f, 0.88f, 0.92f, 0.80f), verts);

        // 9. Submit batch to GPU
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(float)), verts.data(), GL_DYNAMIC_DRAW);

        glUseProgram(m_shaderProgram);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size() / 6));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);

        // Restore OpenGL state
        if (!blendEnabled) glDisable(GL_BLEND);
        glBlendFunc(prevBlendSrc, prevBlendDst);
        if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
        if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    }

    void GameOverUI::Render(
        int screenWidth,
        int screenHeight,
        int completedSegments,
        const char* difficultyStage)
    {
        RenderGameOver(screenWidth, screenHeight, 0.0f, 0.0f, false, completedSegments, difficultyStage);
    }
} // namespace BlockJumper
