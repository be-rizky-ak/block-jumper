#include "GameOverUI.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "Aion/Core/Input.h"
#include "Aion/Core/KeyCodes.h"
#include "Aion/Core/MouseCodes.h"
#include "Aion/Renderer/RenderCommand.h"
#include "Aion/Renderer/Shader.h"
#include "Aion/Renderer/Texture.h"
#include "Aion/Renderer/TextureLibrary.h"
#include "Aion/Renderer/VertexArray.h"
#include "ui/KenneyFontAtlas.h"

namespace BlockJumper
{
    namespace
    {
        std::string ResolveUIPath(const std::string& path)
        {
            const std::vector<std::string> searchPrefixes = {
                "",
                "games/BlockJumper/",
                "assets/",
                "build/games/BlockJumper/",
                "../",
                "../games/BlockJumper/",
                "../../games/BlockJumper/",
                "prorotype-stagging/games/BlockJumper/",
                "../prorotype-stagging/games/BlockJumper/",
                "block-jumper-stagging/",
            };

            for (const auto& prefix : searchPrefixes)
            {
                std::string candidate = prefix + path;
                if (std::filesystem::exists(candidate))
                {
                    return candidate;
                }
            }
            return path;
        }

        const char* s_UIVertexShader = R"(#version 330 core
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

        const char* s_UIFragmentShader = R"(#version 330 core
in vec2 vTexCoord;
in vec4 vColor;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uUseTexture;

void main()
{
    if (uUseTexture == 1)
    {
        vec4 texColor = texture(uTexture, vTexCoord);
        FragColor = texColor * vColor;
    }
    else
    {
        FragColor = vColor;
    }
}
)";
    } // namespace

    GameOverUI::GameOverUI() = default;

    GameOverUI::~GameOverUI()
    {
        Shutdown();
    }

    std::shared_ptr<Aion::Texture> GameOverUI::LoadTexture(const std::string& relativePath)
    {
        std::string resolved = ResolveUIPath(relativePath);
        return Aion::TextureLibrary::Load(resolved);
    }

    std::shared_ptr<Aion::Texture> GameOverUI::LoadSVGTexture(const std::string& relativePath, int targetWidth, int targetHeight)
    {
        std::string resolved = ResolveUIPath(relativePath);
        return Aion::TextureLibrary::LoadSVG(resolved, targetWidth, targetHeight);
    }

    void GameOverUI::Initialize()
    {
        if (m_initialized)
        {
            return;
        }

        m_shader = std::make_unique<Aion::Shader>(s_UIVertexShader, s_UIFragmentShader);
        m_vertexArray = std::make_unique<Aion::VertexArray>();
        m_vertexArray->SetLayout({2, 2, 4});

        m_texWhiteFallback = Aion::TextureLibrary::GetWhiteTexture();

        // Load font atlas texture (Kenney Future font atlas remains pre-rendered PNG)
        m_texFontAtlas      = LoadTexture("assets/textures/kenney_ui-pack/png/kenney_future_atlas.png");

        // Load all Kenney UI vector assets directly from original SVG files at runtime via Aion SVGImporter
        std::string svgBase = "assets/textures/kenney_ui-pack/vector/";
        m_texHudPanel       = LoadSVGTexture(svgBase + "Grey/button_rectangle_depth_flat.svg", 384, 128);
        m_texCardPanel      = LoadSVGTexture(svgBase + "Grey/button_rectangle_depth_flat.svg", 576, 384);
        m_texBtnNormal      = LoadSVGTexture(svgBase + "Green/button_rectangle_depth_flat.svg", 384, 128);
        m_texBtnHover       = LoadSVGTexture(svgBase + "Green/button_rectangle_depth_gradient.svg", 384, 128);
        m_texBtnPressed     = LoadSVGTexture(svgBase + "Green/button_rectangle_flat.svg", 384, 128);
        m_texStarGold       = LoadSVGTexture(svgBase + "Yellow/star.svg", 256, 256);
        m_texIconRepeat     = LoadSVGTexture(svgBase + "Extra/icon_repeat_light.svg", 256, 256);
        m_texStageIndicator = LoadSVGTexture(svgBase + "Yellow/slide_horizontal_color_section.svg", 384, 64);
        m_texDivider        = LoadSVGTexture(svgBase + "Extra/divider.svg", 384, 16);

        std::cout << "[GameOverUI] Initialized Kenney UI Pack with direct SVG vector rendering & Kenney Future font." << std::endl;
        bool allLoaded = (m_texFontAtlas && m_texHudPanel && m_texCardPanel &&
                          m_texBtnNormal && m_texBtnHover && m_texBtnPressed &&
                          m_texStarGold && m_texIconRepeat && m_texStageIndicator &&
                          m_texDivider);
        if (allLoaded)
        {
            std::cout << "[GameOverUI] All 9 vector SVG UI textures and font atlas loaded successfully." << std::endl;
        }
        else
        {
            std::cerr << "[GameOverUI WARNING] Some UI vector assets failed to load! Check error logs above." << std::endl;
        }
        m_initialized = true;
    }

    void GameOverUI::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        m_texFontAtlas.reset();
        m_texHudPanel.reset();
        m_texCardPanel.reset();
        m_texBtnNormal.reset();
        m_texBtnHover.reset();
        m_texBtnPressed.reset();
        m_texStarGold.reset();
        m_texIconRepeat.reset();
        m_texStageIndicator.reset();
        m_texDivider.reset();
        m_texWhiteFallback.reset();

        m_vertexArray.reset();
        m_shader.reset();

        m_initialized = false;
    }

    void GameOverUI::EnsureInitialized()
    {
        if (!m_initialized)
        {
            Initialize();
        }
    }

    void GameOverUI::AddColoredQuad(
        float x0, float y0, float x1, float y1,
        const Aion::Vector4& color,
        std::vector<float>& vertexBuffer)
    {
        float r = color.x, g = color.y, b = color.z, a = color.w;
        float verts[48] = {
            // Triangle 1
            x0, y0, 0.0f, 0.0f, r, g, b, a,
            x1, y0, 1.0f, 0.0f, r, g, b, a,
            x1, y1, 1.0f, 1.0f, r, g, b, a,
            // Triangle 2
            x0, y0, 0.0f, 0.0f, r, g, b, a,
            x1, y1, 1.0f, 1.0f, r, g, b, a,
            x0, y1, 0.0f, 1.0f, r, g, b, a,
        };
        vertexBuffer.insert(vertexBuffer.end(), verts, verts + 48);
    }

    void GameOverUI::AddTexturedQuad(
        float x0, float y0, float x1, float y1,
        float u0, float v0, float u1, float v1,
        const Aion::Vector4& tint,
        std::vector<float>& vertexBuffer)
    {
        float r = tint.x, g = tint.y, b = tint.z, a = tint.w;
        float verts[48] = {
            // Triangle 1
            x0, y0, u0, v0, r, g, b, a,
            x1, y0, u1, v0, r, g, b, a,
            x1, y1, u1, v1, r, g, b, a,
            // Triangle 2
            x0, y0, u0, v0, r, g, b, a,
            x1, y1, u1, v1, r, g, b, a,
            x0, y1, u0, v1, r, g, b, a,
        };
        vertexBuffer.insert(vertexBuffer.end(), verts, verts + 48);
    }

    void GameOverUI::Add9Slice(
        float x0, float y0, float x1, float y1,
        float cornerPx,
        float texW, float texH,
        const Aion::Vector4& tint,
        std::vector<float>& vertexBuffer,
        int screenW, int screenH)
    {
        float pixelW = 2.0f / static_cast<float>(screenW);
        float pixelH = 2.0f / static_cast<float>(screenH);

        float cornerW = std::min((x1 - x0) * 0.45f, cornerPx * pixelW);
        float cornerH = std::min((y1 - y0) * 0.45f, cornerPx * pixelH);

        float xa[4] = { x0, x0 + cornerW, x1 - cornerW, x1 };
        float ya[4] = { y0, y0 + cornerH, y1 - cornerH, y1 };

        float uCorn = cornerPx / texW;
        float vCorn = cornerPx / texH;

        float ua[4] = { 0.0f, uCorn, 1.0f - uCorn, 1.0f };
        // For standard OpenGL loaded images (V=0 at bottom, V=1 at top):
        float va[4] = { 0.0f, vCorn, 1.0f - vCorn, 1.0f };

        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                AddTexturedQuad(
                    xa[col], ya[row], xa[col + 1], ya[row + 1],
                    ua[col], va[row], ua[col + 1], va[row + 1],
                    tint,
                    vertexBuffer);
            }
        }
    }

    void GameOverUI::DrawKenneyString(
        const std::string& text,
        float startX, float startY,
        float scale,
        const Aion::Vector4& color,
        std::vector<float>& vertexBuffer,
        int screenW, int screenH,
        float letterSpacing)
    {
        float pixelW = 2.0f / static_cast<float>(screenW);
        float pixelH = 2.0f / static_cast<float>(screenH);

        float curX = startX;
        for (char c : text)
        {
            const GlyphMetric* g = GetKenneyGlyph(c);
            if (!g) continue;

            if (c == ' ')
            {
                curX += (g->xAdvance * scale) * pixelW + letterSpacing;
                continue;
            }

            float gw = g->width * scale * pixelW;
            float gh = g->height * scale * pixelH;

            float gx0 = curX + (g->xOffset * scale) * pixelW;
            float gy1 = startY - ((g->yOffset - 12) * scale) * pixelH;
            float gy0 = gy1 - gh;

            float u0 = static_cast<float>(g->x) / static_cast<float>(KENNEY_FONT_ATLAS_WIDTH);
            float u1 = static_cast<float>(g->x + g->width) / static_cast<float>(KENNEY_FONT_ATLAS_WIDTH);

            // In our generated font atlas, image Y=0 is top, Y=512 is bottom.
            // In OpenGL texture coordinates, V=1.0 is top, V=0.0 is bottom.
            float v1 = 1.0f - (static_cast<float>(g->y) / static_cast<float>(KENNEY_FONT_ATLAS_HEIGHT));
            float v0 = 1.0f - (static_cast<float>(g->y + g->height) / static_cast<float>(KENNEY_FONT_ATLAS_HEIGHT));

            AddTexturedQuad(gx0, gy0, gx0 + gw, gy1, u0, v0, u1, v1, color, vertexBuffer);
            curX += (g->xAdvance * scale) * pixelW + letterSpacing;
        }
    }

    float GameOverUI::MeasureKenneyString(
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

    void GameOverUI::FlushBatch(const std::shared_ptr<Aion::Texture>& texture, const std::vector<float>& vertexBuffer)
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

        m_vertexArray->SetDynamicVertexBuffer(vertexBuffer.data(), static_cast<uint32_t>(vertexBuffer.size() * sizeof(float)));
        m_vertexArray->DrawArrays(static_cast<uint32_t>(vertexBuffer.size() / 8), Aion::PrimitiveType::Triangles);
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

    bool GameOverUI::CheckRetryAction(int screenWidth, int screenHeight)
    {
        m_buttonPressed = false;

        // 1. Keyboard trigger: R, Space, or Enter
        if (Aion::Input::GetKey(Aion::Key::R) ||
            Aion::Input::GetKey(Aion::Key::Space) ||
            Aion::Input::GetKey(Aion::Key::Enter))
        {
            m_buttonPressed = true;
            return true;
        }

        // 2. Mouse click on Retry button
        if (Aion::Input::GetMouseButton(Aion::Mouse::ButtonLeft))
        {
            float mx = Aion::Input::GetMouseX();
            float my = Aion::Input::GetMouseY();
            if (IsMouseOverButton(mx, my, screenWidth, screenHeight))
            {
                m_buttonPressed = true;
                return true;
            }
        }

        return false;
    }

    void GameOverUI::RenderLoading(
        int screenWidth,
        int screenHeight,
        float progress,
        const std::string& statusText)
    {
        if (screenWidth <= 0 || screenHeight <= 0)
        {
            return;
        }

        EnsureInitialized();

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

        // Fullscreen dimming overlay
        std::vector<float> dimVerts;
        AddColoredQuad(-1.0f, -1.0f, 1.0f, 1.0f,
                       Aion::Vector4(0.04f, 0.06f, 0.10f, 0.75f), dimVerts);
        FlushBatch(nullptr, dimVerts);

        // Central card panel (520px x 190px)
        float cardHalfW = 260.0f * pixelW;
        float cardHalfH = 95.0f * pixelH;
        float cardX0 = -cardHalfW;
        float cardX1 =  cardHalfW;
        float cardY0 = -cardHalfH;
        float cardY1 =  cardHalfH;

        // Card drop shadow
        std::vector<float> shadowVerts;
        AddColoredQuad(cardX0 + 8.0f * pixelW, cardY0 - 8.0f * pixelH,
                       cardX1 + 8.0f * pixelW, cardY1 - 8.0f * pixelH,
                       Aion::Vector4(0.02f, 0.03f, 0.05f, 0.50f), shadowVerts);
        FlushBatch(nullptr, shadowVerts);

        // Card body: 9-slice Kenney Card Panel
        std::vector<float> cardVerts;
        Add9Slice(cardX0, cardY0, cardX1, cardY1, 24.0f, 576.0f, 384.0f,
                  Aion::Vector4(0.14f, 0.18f, 0.24f, 0.98f), cardVerts,
                  screenWidth, screenHeight);
        FlushBatch(m_texCardPanel ? m_texCardPanel : m_texWhiteFallback, cardVerts);

        // Loading Header text: statusText or "LOADING..."
        std::vector<float> fontVerts;
        std::string text = statusText.empty() ? "LOADING..." : statusText;
        float textScale = 0.58f;
        float textW = MeasureKenneyString(text, textScale, screenWidth, 2.0f * pixelW);
        float textX = -textW * 0.5f;
        float textY = cardY1 - 44.0f * pixelH;

        // Drop shadow behind text
        DrawKenneyString(text, textX + 2.0f * pixelW, textY - 2.0f * pixelH,
                         textScale, Aion::Vector4(0.02f, 0.03f, 0.05f, 0.90f),
                         fontVerts, screenWidth, screenHeight, 2.0f * pixelW);
        // Gold/coral header text
        DrawKenneyString(text, textX, textY,
                         textScale, Aion::Vector4(1.0f, 0.88f, 0.25f, 1.0f),
                         fontVerts, screenWidth, screenHeight, 2.0f * pixelW);

        // Progress bar track
        float barW = (cardHalfW * 2.0f) - 64.0f * pixelW;
        float barH = 22.0f * pixelH;
        float barX0 = -barW * 0.5f;
        float barX1 =  barW * 0.5f;
        float barY0 = cardY0 + 38.0f * pixelH;
        float barY1 = barY0 + barH;

        // Track background
        std::vector<float> trackVerts;
        AddColoredQuad(barX0, barY0, barX1, barY1,
                       Aion::Vector4(0.06f, 0.08f, 0.12f, 1.0f), trackVerts);
        FlushBatch(nullptr, trackVerts);

        // Progress bar fill (Kenney green / gold)
        float clampedProgress = std::clamp(progress, 0.0f, 1.0f);
        if (clampedProgress > 0.001f)
        {
            float fillX1 = barX0 + barW * clampedProgress;
            std::vector<float> fillVerts;
            if (m_texStageIndicator)
            {
                AddTexturedQuad(barX0, barY0, fillX1, barY1,
                                0.0f, 0.0f, clampedProgress, 1.0f,
                                Aion::Vector4(0.35f, 0.90f, 0.65f, 1.0f), fillVerts);
                FlushBatch(m_texStageIndicator, fillVerts);
            }
            else
            {
                AddColoredQuad(barX0, barY0, fillX1, barY1,
                               Aion::Vector4(0.35f, 0.90f, 0.65f, 1.0f), fillVerts);
                FlushBatch(nullptr, fillVerts);
            }
        }

        FlushBatch(m_texFontAtlas, fontVerts);

        Aion::RenderCommand::ApplyState(prevState);
    }

    void GameOverUI::RenderHUD(
        int screenWidth,
        int screenHeight,
        float currentDistance,
        int difficultyStage,
        const char* stageName,
        float bestDistance,
        float stageMilestoneTimer)
    {
        if (screenWidth <= 0 || screenHeight <= 0)
        {
            return;
        }

        EnsureInitialized();

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

        // Responsive top-left anchoring for isometric gameplay camera
        float panelW = 460.0f * pixelW;
        float panelH = 125.0f * pixelH;
        float hudX0 = -1.0f + 25.0f * pixelW;
        float hudX1 = hudX0 + panelW;
        float hudY1 = 1.0f - 20.0f * pixelH;
        float hudY0 = hudY1 - panelH;

        // 1. Drop shadow behind HUD panel
        std::vector<float> shadowVerts;
        AddColoredQuad(hudX0 + 6.0f * pixelW, hudY0 - 6.0f * pixelH,
                       hudX1 + 6.0f * pixelW, hudY1 - 6.0f * pixelH,
                       Aion::Vector4(0.02f, 0.03f, 0.05f, 0.45f), shadowVerts);
        FlushBatch(nullptr, shadowVerts);

        // 2. 9-Slice Kenney Grey panel with subtle slate tint
        std::vector<float> panelVerts;
        Aion::Vector4 panelTint(0.20f, 0.26f, 0.32f, 0.90f);
        if (stageMilestoneTimer > 0.0f)
        {
            float pulse = std::sin(stageMilestoneTimer * 12.0f) * 0.5f + 0.5f;
            panelTint = Aion::Vector4(0.20f + pulse * 0.15f, 0.30f + pulse * 0.15f, 0.35f, 0.92f);
        }
        Add9Slice(hudX0, hudY0, hudX1, hudY1, 24.0f, 384.0f, 128.0f, panelTint, panelVerts, screenWidth, screenHeight);
        FlushBatch(m_texHudPanel, panelVerts);

        // 3. Format telemetry strings
        int intDist = static_cast<int>(std::floor(std::max(0.0f, currentDistance)));
        int intBest = static_cast<int>(std::floor(std::max(0.0f, bestDistance)));

        std::string distStr = "DIST: " + std::to_string(intDist) + "M";
        std::string bestStr = (intBest > 0) ? ("  BEST: " + std::to_string(intBest) + "M") : "";

        std::string stageLabel = "STAGE " + std::to_string(difficultyStage) +
                                 " [" + std::string(stageName ? stageName : "EASY") + "]";

        // 4. Render typography with Kenney Future font
        std::vector<float> textVerts;
        float fontScale = 0.46f;
        float textStartX = hudX0 + 24.0f * pixelW;
        float line1Y = hudY1 - 38.0f * pixelH;
        float line2Y = hudY1 - 85.0f * pixelH;

        // Line 1: Distance & Best record
        // Dark drop shadow behind text for crisp contrast
        DrawKenneyString(distStr + bestStr, textStartX + 2.0f * pixelW, line1Y - 2.0f * pixelH,
                         fontScale, Aion::Vector4(0.03f, 0.04f, 0.06f, 0.85f), textVerts,
                         screenWidth, screenHeight);

        // Distance text: Pure white
        DrawKenneyString(distStr, textStartX, line1Y,
                         fontScale, Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f), textVerts,
                         screenWidth, screenHeight);

        if (!bestStr.empty())
        {
            float distW = MeasureKenneyString(distStr, fontScale, screenWidth);
            DrawKenneyString(bestStr, textStartX + distW, line1Y,
                             fontScale, Aion::Vector4(0.95f, 0.85f, 0.35f, 1.0f), textVerts,
                             screenWidth, screenHeight);
        }

        // Line 2: Difficulty stage with Kenney green/emerald styling
        DrawKenneyString(stageLabel, textStartX + 2.0f * pixelW, line2Y - 2.0f * pixelH,
                         fontScale * 0.90f, Aion::Vector4(0.03f, 0.04f, 0.06f, 0.85f), textVerts,
                         screenWidth, screenHeight);

        Aion::Vector4 stageColor(0.35f, 0.90f, 0.65f, 1.0f);
        if (stageMilestoneTimer > 0.0f)
        {
            stageColor = Aion::Vector4(1.0f, 0.90f, 0.30f, 1.0f); // Golden pulse during milestone
        }
        DrawKenneyString(stageLabel, textStartX, line2Y,
                         fontScale * 0.90f, stageColor, textVerts,
                         screenWidth, screenHeight);

        float stageW = MeasureKenneyString(stageLabel, fontScale * 0.90f, screenWidth);

        // 5. Render difficulty stage pips using Kenney slide section graphic
        std::vector<float> indicatorVerts;
        float pipSize = 13.0f;
        float pipW = pipSize * pixelW;
        float pipH = pipSize * pixelH;
        float pipStartX = textStartX + stageW + 10.0f * pixelW;
        float pipCenterY = line2Y - 12.0f * pixelH;

        for (int i = 0; i < 5; ++i)
        {
            float px0 = pipStartX + i * (pipSize + 4.0f) * pixelW;
            float px1 = px0 + pipW;
            float py0 = pipCenterY - pipH * 0.5f;
            float py1 = pipCenterY + pipH * 0.5f;

            bool isActive = (i < difficultyStage);
            Aion::Vector4 pipTint = isActive ? (stageMilestoneTimer > 0.0f
                                                ? Aion::Vector4(1.0f, 0.90f, 0.30f, 1.0f)
                                                : Aion::Vector4(0.35f, 0.90f, 0.65f, 1.0f))
                                             : Aion::Vector4(0.30f, 0.35f, 0.40f, 0.40f);

            AddTexturedQuad(px0, py0, px1, py1, 0.0f, 0.0f, 1.0f, 1.0f, pipTint, indicatorVerts);
        }
        FlushBatch(m_texStageIndicator, indicatorVerts);

        if (stageMilestoneTimer > 0.0f)
        {
            std::string upStr = " [STAGE UP!]";
            float upStartX = pipStartX + 5 * (pipSize + 4.0f) * pixelW + 6.0f * pixelW;
            DrawKenneyString(upStr, upStartX, line2Y,
                             fontScale * 0.85f, Aion::Vector4(1.0f, 0.80f, 0.20f, 1.0f), textVerts,
                             screenWidth, screenHeight);
        }

        FlushBatch(m_texFontAtlas, textVerts);

        Aion::RenderCommand::ApplyState(prevState);
    }

    void GameOverUI::RenderGameOver(
        int screenWidth,
        int screenHeight,
        float finalDistance,
        float bestDistance,
        bool isNewRecord,
        int completedSegments,
        const char* difficultyStage,
        float gameOverDuration)
    {
        if (screenWidth <= 0 || screenHeight <= 0)
        {
            return;
        }

        EnsureInitialized();

        // Smooth alpha fade-in transition (fully visible if duration <= 0, with minimum floor to prevent invisible UI)
        float alpha = (gameOverDuration <= 0.0f) ? 1.0f : std::min(1.0f, std::max(0.4f, gameOverDuration * 4.0f));

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

        // 1. Fullscreen Dimming Overlay
        std::vector<float> dimVerts;
        AddColoredQuad(-1.0f, -1.0f, 1.0f, 1.0f,
                       Aion::Vector4(0.04f, 0.06f, 0.10f, 0.72f * alpha), dimVerts);
        FlushBatch(nullptr, dimVerts);

        // 2. Central Card Dialog Box (620px x 460px)
        float cardHalfW = 310.0f * pixelW;
        float cardHalfH = 230.0f * pixelH;
        float cardCenterY = 0.02f;
        float cardX0 = -cardHalfW;
        float cardX1 =  cardHalfW;
        float cardY0 = cardCenterY - cardHalfH;
        float cardY1 = cardCenterY + cardHalfH;

        // Card drop shadow
        std::vector<float> cardShadowVerts;
        AddColoredQuad(cardX0 + 12.0f * pixelW, cardY0 - 12.0f * pixelH,
                       cardX1 + 12.0f * pixelW, cardY1 - 12.0f * pixelH,
                       Aion::Vector4(0.02f, 0.03f, 0.05f, 0.55f * alpha), cardShadowVerts);
        FlushBatch(nullptr, cardShadowVerts);

        // Card body: 9-slice Kenney Card Panel (Deep slate navy with crisp rounded bevel)
        std::vector<float> cardVerts;
        Add9Slice(cardX0, cardY0, cardX1, cardY1, 28.0f, 576.0f, 384.0f,
                  Aion::Vector4(0.14f, 0.18f, 0.24f, 0.98f * alpha), cardVerts,
                  screenWidth, screenHeight);
        FlushBatch(m_texCardPanel ? m_texCardPanel : m_texWhiteFallback, cardVerts);

        // 3. Header Accent Divider
        std::vector<float> divVerts;
        float divY = cardY1 - 85.0f * pixelH;
        AddTexturedQuad(cardX0 + 30.0f * pixelW, divY - 4.0f * pixelH,
                        cardX1 - 30.0f * pixelW, divY + 4.0f * pixelH,
                        0.0f, 0.0f, 1.0f, 1.0f,
                        Aion::Vector4(0.95f, 0.75f, 0.25f, 0.85f * alpha), divVerts);
        FlushBatch(m_texDivider, divVerts);

        // 4. "GAME OVER" Title in Kenney Future bold typography
        std::vector<float> fontVerts;
        std::string titleText = "GAME OVER";
        float titleScale = 0.95f;
        float titleW = MeasureKenneyString(titleText, titleScale, screenWidth, 3.0f * pixelW);
        float titleX = -titleW * 0.5f;
        float titleY = cardY1 - 40.0f * pixelH;

        // Drop shadow behind title
        DrawKenneyString(titleText, titleX + 4.0f * pixelW, titleY - 4.0f * pixelH,
                         titleScale, Aion::Vector4(0.04f, 0.05f, 0.08f, 0.90f * alpha),
                         fontVerts, screenWidth, screenHeight, 3.0f * pixelW);

        // Warm Coral/Gold Title
        DrawKenneyString(titleText, titleX, titleY,
                         titleScale, Aion::Vector4(1.0f, 0.48f, 0.22f, 1.0f * alpha),
                         fontVerts, screenWidth, screenHeight, 3.0f * pixelW);

        // 5. New Record Notice with Kenney Gold Stars
        float starScale = 36.0f;
        float contentStartY = cardY1 - 120.0f * pixelH;

        if (isNewRecord)
        {
            std::string recText = "*** NEW RECORD! ***";
            float recScale = 0.55f;
            float recW = MeasureKenneyString(recText, recScale, screenWidth, 2.0f * pixelW);
            float recX = -recW * 0.5f;
            float recY = contentStartY;

            // Golden stars on left and right
            std::vector<float> starVerts;
            float starHalfW = (starScale * 0.5f) * pixelW;
            float starHalfH = (starScale * 0.5f) * pixelH;
            float starCenterY = recY - 8.0f * pixelH;

            // Left star
            AddTexturedQuad(recX - 45.0f * pixelW - starHalfW, starCenterY - starHalfH,
                            recX - 45.0f * pixelW + starHalfW, starCenterY + starHalfH,
                            0.0f, 0.0f, 1.0f, 1.0f,
                            Aion::Vector4(1.0f, 0.88f, 0.20f, 1.0f * alpha), starVerts);
            // Right star
            AddTexturedQuad(recX + recW + 45.0f * pixelW - starHalfW, starCenterY - starHalfH,
                            recX + recW + 45.0f * pixelW + starHalfW, starCenterY + starHalfH,
                            0.0f, 0.0f, 1.0f, 1.0f,
                            Aion::Vector4(1.0f, 0.88f, 0.20f, 1.0f * alpha), starVerts);
            FlushBatch(m_texStarGold, starVerts);

            // New record text
            DrawKenneyString(recText, recX + 2.0f * pixelW, recY - 2.0f * pixelH,
                             recScale, Aion::Vector4(0.08f, 0.05f, 0.02f, 0.90f * alpha),
                             fontVerts, screenWidth, screenHeight, 2.0f * pixelW);
            DrawKenneyString(recText, recX, recY,
                             recScale, Aion::Vector4(1.0f, 0.88f, 0.20f, 1.0f * alpha),
                             fontVerts, screenWidth, screenHeight, 2.0f * pixelW);

            contentStartY -= 42.0f * pixelH;
        }

        // 6. Run Statistics (Distance, Best Record, Cleared Segments, Difficulty Tier)
        int intDist = static_cast<int>(std::floor(std::max(0.0f, finalDistance)));
        int intBest = static_cast<int>(std::floor(std::max(0.0f, bestDistance)));

        float statScale = 0.50f;
        std::string distLine = "DISTANCE: " + std::to_string(intDist) + "M   BEST: " + std::to_string(intBest) + "M";
        float distW = MeasureKenneyString(distLine, statScale, screenWidth, 1.5f * pixelW);
        float distStartX = -distW * 0.5f;
        float distStartY = contentStartY;

        DrawKenneyString(distLine, distStartX + 2.0f * pixelW, distStartY - 2.0f * pixelH,
                         statScale, Aion::Vector4(0.02f, 0.03f, 0.05f, 0.85f * alpha),
                         fontVerts, screenWidth, screenHeight, 1.5f * pixelW);
        DrawKenneyString(distLine, distStartX, distStartY,
                         statScale, Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f * alpha),
                         fontVerts, screenWidth, screenHeight, 1.5f * pixelW);

        std::string tierLine = "CLEARED: " + std::to_string(completedSegments) +
                               " SEGMENTS | TIER: " + std::string(difficultyStage);
        float tierW = MeasureKenneyString(tierLine, statScale * 0.90f, screenWidth, 1.5f * pixelW);
        float tierStartX = -tierW * 0.5f;
        float tierStartY = distStartY - 36.0f * pixelH;

        DrawKenneyString(tierLine, tierStartX + 2.0f * pixelW, tierStartY - 2.0f * pixelH,
                         statScale * 0.90f, Aion::Vector4(0.02f, 0.03f, 0.05f, 0.85f * alpha),
                         fontVerts, screenWidth, screenHeight, 1.5f * pixelW);
        DrawKenneyString(tierLine, tierStartX, tierStartY,
                         statScale * 0.90f, Aion::Vector4(0.65f, 0.85f, 0.80f, 0.95f * alpha),
                         fontVerts, screenWidth, screenHeight, 1.5f * pixelW);

        // 7. Interactive Kenney Retry Button
        float btnHalfW = 150.0f * pixelW;
        float btnHalfH = 34.0f * pixelH;
        float btnCenterY = cardY0 + 80.0f * pixelH;
        m_btnMinX = -btnHalfW;
        m_btnMaxX =  btnHalfW;
        m_btnMinY = btnCenterY - btnHalfH;
        m_btnMaxY = btnCenterY + btnHalfH;

        float mx = Aion::Input::GetMouseX();
        float my = Aion::Input::GetMouseY();
        bool hovered = IsMouseOverButton(mx, my, screenWidth, screenHeight);
        bool pressed = m_buttonPressed || (hovered && Aion::Input::GetMouseButton(Aion::Mouse::ButtonLeft));

        if (m_buttonVisualStateOverride == 0) // Force Normal
        {
            hovered = false;
            pressed = false;
        }
        else if (m_buttonVisualStateOverride == 1) // Force Hover
        {
            hovered = true;
            pressed = false;
        }
        else if (m_buttonVisualStateOverride == 2) // Force Pressed
        {
            hovered = true;
            pressed = true;
        }

        std::shared_ptr<Aion::Texture> activeBtnTex = m_texBtnNormal;
        if (pressed)
        {
            activeBtnTex = m_texBtnPressed;
        }
        else if (hovered)
        {
            activeBtnTex = m_texBtnHover;
        }

        float pressYOffset = pressed ? (-3.0f * pixelH) : 0.0f;
        Aion::Vector4 btnTint;
        if (activeBtnTex)
        {
            btnTint = pressed ? Aion::Vector4(0.92f, 0.92f, 0.92f, 1.0f * alpha)
                    : (hovered ? Aion::Vector4(1.08f, 1.08f, 1.08f, 1.0f * alpha)
                               : Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f * alpha));
        }
        else
        {
            btnTint = pressed ? Aion::Vector4(0.016f, 0.427f, 0.255f, 1.0f * alpha)
                    : (hovered ? Aion::Vector4(0.184f, 0.843f, 0.573f, 1.0f * alpha)
                               : Aion::Vector4(0.086f, 0.733f, 0.467f, 1.0f * alpha));
        }

        // Button shadow
        std::vector<float> btnShadowVerts;
        AddColoredQuad(m_btnMinX + 5.0f * pixelW, m_btnMinY - 5.0f * pixelH,
                       m_btnMaxX + 5.0f * pixelW, m_btnMaxY - 5.0f * pixelH,
                       Aion::Vector4(0.02f, 0.03f, 0.05f, 0.40f * alpha), btnShadowVerts);
        FlushBatch(nullptr, btnShadowVerts);

        // 9-Slice Kenney Button (Normal, Hover, or Pressed flat)
        std::vector<float> btnVerts;
        Add9Slice(m_btnMinX, m_btnMinY + (pressed ? pressYOffset : 0.0f),
                  m_btnMaxX, m_btnMaxY + (pressed ? pressYOffset : 0.0f),
                  20.0f, 384.0f, 128.0f,
                  btnTint, btnVerts, screenWidth, screenHeight);
        FlushBatch(activeBtnTex ? activeBtnTex : m_texWhiteFallback, btnVerts);

        // Retry repeat icon on button
        std::vector<float> iconVerts;
        float iconSize = 28.0f;
        float iconHalfW = (iconSize * 0.5f) * pixelW;
        float iconHalfH = (iconSize * 0.5f) * pixelH;

        std::string btnText = "RETRY [R]";
        float btnTextScale = 0.52f;
        float btnTextW = MeasureKenneyString(btnText, btnTextScale, screenWidth, 1.5f * pixelW);
        float totalBtnContentW = (iconSize + 12.0f) * pixelW + btnTextW;
        float iconCenterX = -totalBtnContentW * 0.5f + iconHalfW;
        float btnTextX = iconCenterX + iconHalfW + 12.0f * pixelW;
        float iconCenterY = btnCenterY + 2.0f * pixelH + pressYOffset;
        float btnTextY = btnCenterY + 10.0f * pixelH + pressYOffset;

        if (m_texIconRepeat)
        {
            AddTexturedQuad(iconCenterX - iconHalfW, iconCenterY - iconHalfH,
                            iconCenterX + iconHalfW, iconCenterY + iconHalfH,
                            0.0f, 0.0f, 1.0f, 1.0f,
                            Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f * alpha), iconVerts);
            FlushBatch(m_texIconRepeat, iconVerts);
        }

        // Button text
        DrawKenneyString(btnText, btnTextX + 2.0f * pixelW, btnTextY - 2.0f * pixelH,
                         btnTextScale, Aion::Vector4(0.04f, 0.15f, 0.08f, 0.90f * alpha),
                         fontVerts, screenWidth, screenHeight, 1.5f * pixelW);
        DrawKenneyString(btnText, btnTextX, btnTextY,
                         btnTextScale, Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f * alpha),
                         fontVerts, screenWidth, screenHeight, 1.5f * pixelW);

        // 8. Hint Footer
        std::string hintText = "PRESS [R] OR [SPACE] TO RETRY";
        float hintScale = 0.36f;
        float hintW = MeasureKenneyString(hintText, hintScale, screenWidth, 1.0f * pixelW);
        float hintX = -hintW * 0.5f;
        float hintY = cardY0 + 26.0f * pixelH;

        DrawKenneyString(hintText, hintX, hintY,
                         hintScale, Aion::Vector4(0.78f, 0.82f, 0.88f, 0.80f * alpha),
                         fontVerts, screenWidth, screenHeight, 1.0f * pixelW);

        FlushBatch(m_texFontAtlas, fontVerts);

        Aion::RenderCommand::ApplyState(prevState);
    }

    void GameOverUI::Render(
        int screenWidth,
        int screenHeight,
        int completedSegments,
        const char* difficultyStage)
    {
        RenderGameOver(screenWidth, screenHeight, 0.0f, 0.0f, false, completedSegments, difficultyStage, 1.0f);
    }
} // namespace BlockJumper
