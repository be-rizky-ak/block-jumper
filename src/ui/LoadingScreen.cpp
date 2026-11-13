#include "LoadingScreen.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "Aion/Renderer/RenderCommand.h"
#include "Aion/Renderer/Shader.h"
#include "Aion/Renderer/Texture.h"
#include "Aion/Renderer/TextureLibrary.h"
#include "Aion/Renderer/VertexArray.h"
#include "ui/KenneyFontAtlas.h"
#include "game/ResourcePath.h"

namespace BlockJumper
{
    namespace
    {
        std::string ResolvePath(const std::string& path)
        {
            return ResolveAsset(path);
        }

        const char* s_LoadingVertexShader = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

out vec2 vTexCoord;
out vec4 vColor;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
)";

        const char* s_LoadingFragmentShader = R"(#version 330 core
in vec2 vTexCoord;
in vec4 vColor;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uUseTexture;

void main()
{
    if (uUseTexture == 1)
    {
        vec4 tex = texture(uTexture, vTexCoord);
        FragColor = tex * vColor;
    }
    else
    {
        FragColor = vColor;
    }
}
)";
    } // namespace

    LoadingScreen::LoadingScreen() = default;

    LoadingScreen::~LoadingScreen()
    {
        Shutdown();
    }

    void LoadingScreen::Initialize()
    {
        if (m_initialized)
        {
            return;
        }

        m_shader = std::make_unique<Aion::Shader>(s_LoadingVertexShader, s_LoadingFragmentShader);
        m_vertexArray = std::make_unique<Aion::VertexArray>();
        m_vertexArray->SetLayout({2, 2, 4});

        m_texWhiteFallback = Aion::TextureLibrary::GetWhiteTexture();

        std::string fontPath = ResolvePath("assets/textures/kenney_ui-pack/png/kenney_future_atlas.png");
        m_texFontAtlas = Aion::TextureLibrary::Load(fontPath);

        m_initialized = true;
        std::cout << "[LoadingScreen] Initialized dedicated startup loading screen presentation layer." << std::endl;
    }

    void LoadingScreen::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        m_texFontAtlas.reset();
        m_texWhiteFallback.reset();
        m_vertexArray.reset();
        m_shader.reset();

        m_initialized = false;
    }

    void LoadingScreen::EnsureInitialized()
    {
        if (!m_initialized)
        {
            Initialize();
        }
    }

    void LoadingScreen::AddColoredQuad(
        float x0, float y0, float x1, float y1,
        const Aion::Vector4& color, std::vector<float>& buffer)
    {
        float quad[48] = {
            x0, y0, 0.0f, 0.0f, color.x, color.y, color.z, color.w,
            x1, y0, 1.0f, 0.0f, color.x, color.y, color.z, color.w,
            x1, y1, 1.0f, 1.0f, color.x, color.y, color.z, color.w,

            x0, y0, 0.0f, 0.0f, color.x, color.y, color.z, color.w,
            x1, y1, 1.0f, 1.0f, color.x, color.y, color.z, color.w,
            x0, y1, 0.0f, 1.0f, color.x, color.y, color.z, color.w
        };
        buffer.insert(buffer.end(), quad, quad + 48);
    }

    void LoadingScreen::AddTexturedQuad(
        float x0, float y0, float x1, float y1,
        float u0, float v0, float u1, float v1,
        const Aion::Vector4& color, std::vector<float>& buffer)
    {
        float r = color.x, g = color.y, b = color.z, a = color.w;
        float quad[48] = {
            // Triangle 1
            x0, y0, u0, v0, r, g, b, a,
            x1, y0, u1, v0, r, g, b, a,
            x1, y1, u1, v1, r, g, b, a,
            // Triangle 2
            x0, y0, u0, v0, r, g, b, a,
            x1, y1, u1, v1, r, g, b, a,
            x0, y1, u0, v1, r, g, b, a,
        };
        buffer.insert(buffer.end(), quad, quad + 48);
    }

    void LoadingScreen::DrawKenneyString(
        const std::string& text,
        float x, float y, float scale,
        const Aion::Vector4& color,
        std::vector<float>& vertexBuffer,
        int screenW, int screenH,
        float letterSpacing)
    {
        float pixelW = 2.0f / static_cast<float>(screenW);
        float pixelH = 2.0f / static_cast<float>(screenH);
        float atlasW = static_cast<float>(KENNEY_FONT_ATLAS_WIDTH);
        float atlasH = static_cast<float>(KENNEY_FONT_ATLAS_HEIGHT);

        float curX = x;
        for (char c : text)
        {
            const GlyphMetric* g = GetKenneyGlyph(c);
            if (!g) continue;

            if (c == ' ')
            {
                curX += (g->xAdvance * scale) * pixelW + letterSpacing;
                continue;
            }

            if (g->width > 0 && g->height > 0)
            {
                float gw = (g->width * scale) * pixelW;
                float gh = (g->height * scale) * pixelH;

                float gx0 = curX + (g->xOffset * scale) * pixelW;
                float gy1 = y - ((g->yOffset - 12) * scale) * pixelH;
                float gy0 = gy1 - gh;

                float u0 = static_cast<float>(g->x) / atlasW;
                float u1 = static_cast<float>(g->x + g->width) / atlasW;

                float v1 = 1.0f - (static_cast<float>(g->y) / atlasH);
                float v0 = 1.0f - (static_cast<float>(g->y + g->height) / atlasH);

                AddTexturedQuad(gx0, gy0, gx0 + gw, gy1, u0, v0, u1, v1, color, vertexBuffer);
            }

            curX += (g->xAdvance * scale) * pixelW + letterSpacing;
        }
    }

    float LoadingScreen::MeasureKenneyString(
        const std::string& text,
        float scale,
        int screenW,
        float letterSpacing) const
    {
        float pixelW = 2.0f / static_cast<float>(screenW);
        float totalWidth = 0.0f;
        for (char c : text)
        {
            const GlyphMetric* g = GetKenneyGlyph(c);
            if (g)
            {
                totalWidth += (g->xAdvance * scale) * pixelW + letterSpacing;
            }
        }
        return totalWidth;
    }

    void LoadingScreen::FlushBatch(
        const std::shared_ptr<Aion::Texture>& texture,
        const std::vector<float>& vertexBuffer)
    {
        if (vertexBuffer.empty() || !m_shader || !m_vertexArray)
        {
            return;
        }

        m_shader->Use();

        if (texture)
        {
            texture->Bind(0);
            m_shader->SetInt("uTexture", 0);
            m_shader->SetInt("uUseTexture", 1);
        }
        else
        {
            m_shader->SetInt("uUseTexture", 0);
        }

        m_vertexArray->SetDynamicVertexBuffer(
            vertexBuffer.data(),
            static_cast<uint32_t>(vertexBuffer.size() * sizeof(float)));
        m_vertexArray->DrawArrays(
            static_cast<uint32_t>(vertexBuffer.size() / 8),
            Aion::PrimitiveType::Triangles);
    }

    void LoadingScreen::Render(
        int screenWidth,
        int screenHeight,
        float alpha,
        float progress,
        const std::string& statusText,
        const std::string& errorMessage,
        float deltaTime)
    {
        if (screenWidth <= 0 || screenHeight <= 0 || alpha <= 0.001f)
        {
            return;
        }

        EnsureInitialized();

        m_indeterminateTime += deltaTime;
        float clampedAlpha = std::clamp(alpha, 0.0f, 1.0f);

        Aion::RenderState prevState = Aion::RenderCommand::GetCurrentState();
        Aion::RenderState uiState = prevState;
        uiState.DepthTest = false;
        uiState.CullingMode = Aion::CullMode::None;
        uiState.BlendEnable = true;
        uiState.SrcBlend = Aion::BlendFactor::SrcAlpha;
        uiState.DstBlend = Aion::BlendFactor::OneMinusSrcAlpha;
        Aion::RenderCommand::ApplyState(uiState);

        float pixelW = 2.0f / static_cast<float>(screenWidth);
        float pixelH = 2.0f / static_cast<float>(screenHeight);

        // 1. Fullscreen solid background covering the entire window
        // Block Jumper signature deep navy slate: (0.055, 0.086, 0.149)
        std::vector<float> bgVerts;
        AddColoredQuad(-1.0f, -1.0f, 1.0f, 1.0f,
                       Aion::Vector4(0.055f, 0.086f, 0.149f, clampedAlpha),
                       bgVerts);
        FlushBatch(nullptr, bgVerts);

        // 2. Visually prominent game title: "BLOCK JUMPER"
        const std::string title = "BLOCK JUMPER";
        float titleScale = 1.15f;
        float titleLetterSpacing = 3.5f * pixelW;
        float titleW = MeasureKenneyString(title, titleScale, screenWidth, titleLetterSpacing);
        float titleX = -titleW * 0.5f;
        float titleY = 0.22f;

        std::vector<float> fontVerts;

        // Title drop shadow
        DrawKenneyString(
            title,
            titleX + 3.0f * pixelW,
            titleY - 3.0f * pixelH,
            titleScale,
            Aion::Vector4(0.01f, 0.02f, 0.04f, 0.85f * clampedAlpha),
            fontVerts,
            screenWidth,
            screenHeight,
            titleLetterSpacing);

        // Title text in bright gold
        DrawKenneyString(
            title,
            titleX,
            titleY,
            titleScale,
            Aion::Vector4(1.0f, 0.88f, 0.25f, clampedAlpha),
            fontVerts,
            screenWidth,
            screenHeight,
            titleLetterSpacing);

        // 3. Status text or error display
        if (errorMessage.empty())
        {
            // Supporting status text (e.g. "PREPARING GAME ASSETS...")
            std::string sub = statusText.empty() ? "PREPARING GAME ASSETS..." : statusText;
            float subScale = 0.44f;
            float subLetterSpacing = 1.5f * pixelW;
            float subW = MeasureKenneyString(sub, subScale, screenWidth, subLetterSpacing);
            float subX = -subW * 0.5f;
            float subY = 0.05f;

            // Subtitle shadow
            DrawKenneyString(
                sub,
                subX + 1.5f * pixelW,
                subY - 1.5f * pixelH,
                subScale,
                Aion::Vector4(0.01f, 0.02f, 0.03f, 0.80f * clampedAlpha),
                fontVerts,
                screenWidth,
                screenHeight,
                subLetterSpacing);

            // Subtitle soft ice-blue
            DrawKenneyString(
                sub,
                subX,
                subY,
                subScale,
                Aion::Vector4(0.78f, 0.86f, 0.96f, 0.90f * clampedAlpha),
                fontVerts,
                screenWidth,
                screenHeight,
                subLetterSpacing);

            // 4. Loading Bar positioned below the game title
            float barTotalW = std::min(520.0f * pixelW, 1.4f);
            float barH = 22.0f * pixelH;
            float barX0 = -barTotalW * 0.5f;
            float barX1 =  barTotalW * 0.5f;
            float barY0 = -0.06f;
            float barY1 = barY0 + barH;

            // Bar drop shadow
            std::vector<float> barVerts;
            AddColoredQuad(
                barX0 + 4.0f * pixelW,
                barY0 - 4.0f * pixelH,
                barX1 + 4.0f * pixelW,
                barY1 - 4.0f * pixelH,
                Aion::Vector4(0.02f, 0.03f, 0.05f, 0.55f * clampedAlpha),
                barVerts);

            // Track outer border (dark frame)
            AddColoredQuad(
                barX0 - 2.0f * pixelW,
                barY0 - 2.0f * pixelH,
                barX1 + 2.0f * pixelW,
                barY1 + 2.0f * pixelH,
                Aion::Vector4(0.14f, 0.18f, 0.25f, 0.95f * clampedAlpha),
                barVerts);

            // Track inner background
            AddColoredQuad(
                barX0,
                barY0,
                barX1,
                barY1,
                Aion::Vector4(0.03f, 0.05f, 0.08f, 1.0f * clampedAlpha),
                barVerts);

            // Bar fill
            if (progress >= 0.0f)
            {
                float clampedProgress = std::clamp(progress, 0.0f, 1.0f);
                if (clampedProgress > 0.005f)
                {
                    float fillX1 = barX0 + barTotalW * clampedProgress;
                    // Kenney vibrant emerald green fill
                    AddColoredQuad(
                        barX0 + 2.0f * pixelW,
                        barY0 + 2.0f * pixelH,
                        fillX1 - 2.0f * pixelW,
                        barY1 - 2.0f * pixelH,
                        Aion::Vector4(0.35f, 0.90f, 0.65f, clampedAlpha),
                        barVerts);
                }
            }
            else
            {
                // Indeterminate loading animation pulse
                float pulse = (std::sin(m_indeterminateTime * 3.5f) + 1.0f) * 0.5f;
                float segW = barTotalW * 0.28f;
                float segX0 = barX0 + (barTotalW - segW) * pulse;
                float segX1 = segX0 + segW;
                AddColoredQuad(
                    segX0,
                    barY0 + 2.0f * pixelH,
                    segX1,
                    barY1 - 2.0f * pixelH,
                    Aion::Vector4(0.35f, 0.90f, 0.65f, clampedAlpha),
                    barVerts);
            }
            FlushBatch(nullptr, barVerts);

            // 5. Clean progress percentage indicator below bar
            if (progress >= 0.0f)
            {
                int pct = static_cast<int>(std::clamp(progress, 0.0f, 1.0f) * 100.0f);
                std::string pctText = std::to_string(pct) + "%";
                float pctScale = 0.36f;
                float pctW = MeasureKenneyString(pctText, pctScale, screenWidth, 1.0f * pixelW);
                float pctX = -pctW * 0.5f;
                float pctY = barY0 - 22.0f * pixelH;

                DrawKenneyString(
                    pctText,
                    pctX + 1.0f * pixelW,
                    pctY - 1.0f * pixelH,
                    pctScale,
                    Aion::Vector4(0.01f, 0.02f, 0.03f, 0.70f * clampedAlpha),
                    fontVerts,
                    screenWidth,
                    screenHeight,
                    1.0f * pixelW);

                DrawKenneyString(
                    pctText,
                    pctX,
                    pctY,
                    pctScale,
                    Aion::Vector4(0.68f, 0.80f, 0.90f, 0.85f * clampedAlpha),
                    fontVerts,
                    screenWidth,
                    screenHeight,
                    1.0f * pixelW);
            }
        }
        else
        {
            // Error State presentation
            float errScale = 0.44f;
            std::string errHeader = "INITIALIZATION ERROR";
            float errHeaderW = MeasureKenneyString(errHeader, 0.52f, screenWidth, 2.0f * pixelW);
            float errHeaderX = -errHeaderW * 0.5f;
            float errHeaderY = 0.04f;

            DrawKenneyString(
                errHeader,
                errHeaderX,
                errHeaderY,
                0.52f,
                Aion::Vector4(0.96f, 0.30f, 0.25f, clampedAlpha),
                fontVerts,
                screenWidth,
                screenHeight,
                2.0f * pixelW);

            float msgW = MeasureKenneyString(errorMessage, errScale, screenWidth, 1.0f * pixelW);
            float msgX = -msgW * 0.5f;
            float msgY = -0.06f;

            DrawKenneyString(
                errorMessage,
                msgX,
                msgY,
                errScale,
                Aion::Vector4(0.95f, 0.80f, 0.80f, clampedAlpha),
                fontVerts,
                screenWidth,
                screenHeight,
                1.0f * pixelW);

            std::string exitHint = "PLEASE VERIFY ASSETS OR PRESS ESCAPE";
            float hintW = MeasureKenneyString(exitHint, 0.36f, screenWidth, 1.0f * pixelW);
            float hintX = -hintW * 0.5f;
            float hintY = -0.16f;

            DrawKenneyString(
                exitHint,
                hintX,
                hintY,
                0.36f,
                Aion::Vector4(0.65f, 0.72f, 0.82f, 0.75f * clampedAlpha),
                fontVerts,
                screenWidth,
                screenHeight,
                1.0f * pixelW);
        }

        FlushBatch(m_texFontAtlas ? m_texFontAtlas : m_texWhiteFallback, fontVerts);

        Aion::RenderCommand::ApplyState(prevState);
    }
} // namespace BlockJumper
