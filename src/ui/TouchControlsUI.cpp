#include "TouchControlsUI.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "Aion/Assets/SVGImporter.h"
#include "Aion/Core/Input.h"
#include "Aion/Core/MouseCodes.h"
#include "Aion/Renderer/RenderCommand.h"
#include "Aion/Renderer/Shader.h"
#include "Aion/Renderer/Texture.h"
#include "Aion/Renderer/TextureLibrary.h"
#include "Aion/Renderer/VertexArray.h"
#include "game/ResourcePath.h"
#include "ui/KenneyFontAtlas.h"

namespace BlockJumper
{
    namespace
    {
        std::string ResolveUIPath(const std::string& path)
        {
            return ResolveAsset(path);
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

    TouchControlsUI* TouchControlsUI::s_activeInstance = nullptr;

    TouchControlsUI::TouchControlsUI()
    {
        s_activeInstance = this;
    }

    TouchControlsUI::~TouchControlsUI()
    {
        Shutdown();
        if (s_activeInstance == this)
        {
            s_activeInstance = nullptr;
        }
    }

    std::shared_ptr<Aion::Texture> TouchControlsUI::LoadTexture(const std::string& relativePath)
    {
        std::string resolved = ResolveUIPath(relativePath);
        return Aion::TextureLibrary::Load(resolved);
    }

    std::shared_ptr<Aion::Texture> TouchControlsUI::LoadSVGTexture(const std::string& relativePath, int targetWidth, int targetHeight)
    {
        std::string resolved = ResolveUIPath(relativePath);
        return Aion::TextureLibrary::LoadSVG(resolved, targetWidth, targetHeight);
    }

    void TouchControlsUI::Initialize()
    {
        if (m_initialized)
        {
            return;
        }

        m_shader = std::make_unique<Aion::Shader>(s_UIVertexShader, s_UIFragmentShader);
        m_vertexArray = std::make_unique<Aion::VertexArray>();
        m_vertexArray->SetLayout({2, 2, 4});

        m_texWhiteFallback = Aion::TextureLibrary::GetWhiteTexture();

        // Load font atlas
        m_texFontAtlas = LoadTexture("assets/textures/kenney_ui-pack/png/kenney_future_atlas.png");

        // Load Kenney vector assets
        std::string svgBase = "assets/textures/kenney_ui-pack/vector/";
        m_texJoyBase = LoadSVGTexture(svgBase + "Grey/button_round_flat.svg", 256, 256);
        m_texJoyKnob = LoadSVGTexture(svgBase + "Green/button_round_depth_flat.svg", 256, 256);
        m_texJumpBtn = LoadSVGTexture(svgBase + "Green/button_round_depth_flat.svg", 256, 256);
        m_texJumpIcon = LoadSVGTexture(svgBase + "Extra/icon_arrow_up_light.svg", 128, 128);
        m_texToggleBtn = LoadSVGTexture(svgBase + "Grey/button_rectangle_depth_flat.svg", 384, 128);
        m_texToggleBtnActive = LoadSVGTexture(svgBase + "Green/button_rectangle_depth_flat.svg", 384, 128);

        m_initialized = true;
        s_activeInstance = this;
    }

    void TouchControlsUI::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        ClearAllTouches();

        m_texJoyBase.reset();
        m_texJoyKnob.reset();
        m_texJumpBtn.reset();
        m_texJumpIcon.reset();
        m_texToggleBtn.reset();
        m_texToggleBtnActive.reset();
        m_texFontAtlas.reset();
        m_texWhiteFallback.reset();

        m_vertexArray.reset();
        m_shader.reset();

        m_initialized = false;
    }

    void TouchControlsUI::EnsureInitialized()
    {
        if (!m_initialized)
        {
            Initialize();
        }
    }

    void TouchControlsUI::Reset()
    {
        ClearAllTouches();
    }

    void TouchControlsUI::SetVisible(bool visible)
    {
        if (m_visible != visible)
        {
            m_visible = visible;
            if (!m_visible)
            {
                ClearAllTouches();
            }
        }
    }

    void TouchControlsUI::SetTouchCapable(bool capable)
    {
        m_touchCapable = capable;
        // On touch-capable devices, default controls to visible
        if (capable)
        {
            m_visible = true;
        }
    }

    bool TouchControlsUI::ConsumeJumpEdge()
    {
        bool edge = m_jumpEdge;
        m_jumpEdge = false;
        return edge;
    }

    void TouchControlsUI::ClearAllTouches()
    {
        m_joyTouchId = -1;
        m_jumpTouchId = -1;
        m_toggleTouchId = -1;
        m_forwardInput = 0.0f;
        m_rightInput = 0.0f;
        m_jumpHeld = false;
        m_jumpEdge = false;
        m_joyKnobX = m_joyCenterX;
        m_joyKnobY = m_joyCenterY;
        m_toggleHovered = false;
    }

    void TouchControlsUI::UpdateLayout(int screenWidth, int screenHeight)
    {
        if (screenWidth <= 0 || screenHeight <= 0)
        {
            return;
        }

        m_lastScreenWidth = screenWidth;
        m_lastScreenHeight = screenHeight;

        float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
        float scale = std::clamp(minDim / 720.0f, 0.70f, 1.25f);

        float margin = std::max(22.0f, 32.0f * scale);

        // Joystick sizing
        m_joyBaseRadius = 75.0f * scale;
        m_joyKnobRadius = 34.0f * scale;
        m_joyCenterX = margin + m_joyBaseRadius;
        m_joyCenterY = static_cast<float>(screenHeight) - margin - m_joyBaseRadius;

        if (m_joyTouchId < 0)
        {
            m_joyKnobX = m_joyCenterX;
            m_joyKnobY = m_joyCenterY;
        }

        // Jump button sizing
        m_jumpRadius = 55.0f * scale;
        m_jumpCenterX = static_cast<float>(screenWidth) - margin - m_jumpRadius;
        m_jumpCenterY = static_cast<float>(screenHeight) - margin - m_jumpRadius;

        // Toggle button sizing & position near top-right HUD
        float toggleW = 140.0f * scale;
        float toggleH = 34.0f * scale;

        if (screenWidth >= 520)
        {
            // Positioned to the left of the Options button [screenWidth - 170px .. screenWidth - 25px]
            float optOffset = (screenWidth >= 800) ? 170.0f : 140.0f;
            m_toggleX1 = static_cast<float>(screenWidth) - optOffset - 12.0f;
            m_toggleX0 = m_toggleX1 - toggleW;
            m_toggleY0 = 20.0f;
            m_toggleY1 = m_toggleY0 + toggleH;
        }
        else
        {
            // Narrow mobile portrait: place cleanly below top-right Options button
            m_toggleX1 = static_cast<float>(screenWidth) - 20.0f;
            m_toggleX0 = m_toggleX1 - toggleW;
            m_toggleY0 = 54.0f + 8.0f;
            m_toggleY1 = m_toggleY0 + toggleH;
        }
    }

    bool TouchControlsUI::IsTouchInJoystick(float screenX, float screenY) const
    {
        float dx = screenX - m_joyCenterX;
        float dy = screenY - m_joyCenterY;
        float touchRadius = m_joyBaseRadius * 1.35f;
        return (dx * dx + dy * dy) <= (touchRadius * touchRadius);
    }

    bool TouchControlsUI::IsTouchInJump(float screenX, float screenY) const
    {
        float dx = screenX - m_jumpCenterX;
        float dy = screenY - m_jumpCenterY;
        float touchRadius = m_jumpRadius * 1.30f;
        return (dx * dx + dy * dy) <= (touchRadius * touchRadius);
    }

    bool TouchControlsUI::IsTouchInToggle(float screenX, float screenY) const
    {
        return (screenX >= m_toggleX0 && screenX <= m_toggleX1 &&
                screenY >= m_toggleY0 && screenY <= m_toggleY1);
    }

    void TouchControlsUI::OnTouchEvent(int id, int phase, float normX, float normY)
    {
        float screenX = normX * static_cast<float>(m_lastScreenWidth);
        float screenY = normY * static_cast<float>(m_lastScreenHeight);

        // Phase: 0 = Down, 1 = Move, 2 = Up, 3 = Cancel
        if (phase == 0) // Touch Down
        {
            // 1. Toggle Button Check
            if (IsTouchInToggle(screenX, screenY))
            {
                m_toggleTouchId = id;
                ToggleVisible();
                return;
            }

            // If controls are hidden, do not process gameplay controls
            if (!m_visible)
            {
                return;
            }

            // 2. Jump Button Check
            if (IsTouchInJump(screenX, screenY))
            {
                m_jumpTouchId = id;
                m_jumpHeld = true;
                m_jumpEdge = true; // Edge trigger
                return;
            }

            // 3. Joystick Check
            if (IsTouchInJoystick(screenX, screenY))
            {
                m_joyTouchId = id;
                float dx = screenX - m_joyCenterX;
                float dy = screenY - m_joyCenterY;
                float dist = std::hypot(dx, dy);
                float maxRadius = m_joyBaseRadius;

                if (dist > maxRadius && dist > 0.001f)
                {
                    dx = (dx / dist) * maxRadius;
                    dy = (dy / dist) * maxRadius;
                    dist = maxRadius;
                }

                m_joyKnobX = m_joyCenterX + dx;
                m_joyKnobY = m_joyCenterY + dy;

                float normDist = dist / maxRadius;
                if (normDist > 0.08f)
                {
                    m_rightInput = dx / maxRadius;
                    m_forwardInput = -(dy / maxRadius); // Screen UP is negative Y, game forward is positive
                }
                else
                {
                    m_rightInput = 0.0f;
                    m_forwardInput = 0.0f;
                }
                return;
            }
        }
        else if (phase == 1) // Touch Move
        {
            if (id == m_joyTouchId)
            {
                float dx = screenX - m_joyCenterX;
                float dy = screenY - m_joyCenterY;
                float dist = std::hypot(dx, dy);
                float maxRadius = m_joyBaseRadius;

                if (dist > maxRadius && dist > 0.001f)
                {
                    dx = (dx / dist) * maxRadius;
                    dy = (dy / dist) * maxRadius;
                    dist = maxRadius;
                }

                m_joyKnobX = m_joyCenterX + dx;
                m_joyKnobY = m_joyCenterY + dy;

                float normDist = dist / maxRadius;
                if (normDist > 0.08f)
                {
                    m_rightInput = dx / maxRadius;
                    m_forwardInput = -(dy / maxRadius);
                }
                else
                {
                    m_rightInput = 0.0f;
                    m_forwardInput = 0.0f;
                }
            }
        }
        else if (phase == 2 || phase == 3) // Touch Up or Cancel
        {
            if (id == m_joyTouchId)
            {
                m_joyTouchId = -1;
                m_joyKnobX = m_joyCenterX;
                m_joyKnobY = m_joyCenterY;
                m_forwardInput = 0.0f;
                m_rightInput = 0.0f;
            }

            if (id == m_jumpTouchId)
            {
                m_jumpTouchId = -1;
                m_jumpHeld = false;
            }

            if (id == m_toggleTouchId)
            {
                m_toggleTouchId = -1;
            }
        }
    }

    void TouchControlsUI::Update(float deltaTime, int screenWidth, int screenHeight, bool blockInput)
    {
        (void)deltaTime;
        EnsureInitialized();
        UpdateLayout(screenWidth, screenHeight);

        if (blockInput)
        {
            ClearAllTouches();
            return;
        }

        // Desktop mouse simulation (for testing touch controls with mouse cursor)
        float mx = Aion::Input::GetMouseX();
        float my = Aion::Input::GetMouseY();
        bool mouseLeft = Aion::Input::GetMouseButton(Aion::Mouse::ButtonLeft);

        m_toggleHovered = IsTouchInToggle(mx, my);

        // If mouse left pressed this frame
        if (mouseLeft && !m_prevMouseLeft)
        {
            float normX = mx / static_cast<float>(screenWidth);
            float normY = my / static_cast<float>(screenHeight);
            OnTouchEvent(-999, 0, normX, normY);
        }
        else if (mouseLeft && m_prevMouseLeft)
        {
            float normX = mx / static_cast<float>(screenWidth);
            float normY = my / static_cast<float>(screenHeight);
            OnTouchEvent(-999, 1, normX, normY);
        }
        else if (!mouseLeft && m_prevMouseLeft)
        {
            float normX = mx / static_cast<float>(screenWidth);
            float normY = my / static_cast<float>(screenHeight);
            OnTouchEvent(-999, 2, normX, normY);
        }

        m_prevMouseLeft = mouseLeft;
    }

    void TouchControlsUI::AddTexturedQuad(
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

    void TouchControlsUI::Add9Slice(
        float x0, float y0, float x1, float y1,
        float cornerPx, float texW, float texH,
        const Aion::Vector4& tint,
        std::vector<float>& vertexBuffer,
        int screenW, int screenH)
    {
        float pixelW = 2.0f / static_cast<float>(screenW);
        float pixelH = 2.0f / static_cast<float>(screenH);

        float cornerW = cornerPx * pixelW;
        float cornerH = cornerPx * pixelH;

        float u0 = 0.0f;
        float u1 = cornerPx / texW;
        float u2 = 1.0f - (cornerPx / texW);
        float u3 = 1.0f;

        float v0 = 0.0f;
        float v1 = cornerPx / texH;
        float v2 = 1.0f - (cornerPx / texH);
        float v3 = 1.0f;

        float xs[4] = { x0, x0 + cornerW, x1 - cornerW, x1 };
        float ys[4] = { y0, y0 + cornerH, y1 - cornerH, y1 };
        float us[4] = { u0, u1, u2, u3 };
        float vs[4] = { v0, v1, v2, v3 };

        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                AddTexturedQuad(
                    xs[col], ys[row], xs[col + 1], ys[row + 1],
                    us[col], vs[row], us[col + 1], vs[row + 1],
                    tint, vertexBuffer);
            }
        }
    }

    void TouchControlsUI::DrawKenneyString(
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

            float v1 = 1.0f - (static_cast<float>(g->y) / static_cast<float>(KENNEY_FONT_ATLAS_HEIGHT));
            float v0 = 1.0f - (static_cast<float>(g->y + g->height) / static_cast<float>(KENNEY_FONT_ATLAS_HEIGHT));

            AddTexturedQuad(gx0, gy0, gx0 + gw, gy1, u0, v0, u1, v1, color, vertexBuffer);
            curX += (g->xAdvance * scale) * pixelW + letterSpacing;
        }
    }

    float TouchControlsUI::MeasureKenneyString(
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

    void TouchControlsUI::FlushBatch(const std::shared_ptr<Aion::Texture>& texture, const std::vector<float>& vertexBuffer)
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

    void TouchControlsUI::Render(int screenWidth, int screenHeight)
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

        // -------------------------------------------------------------
        // 1. Render Top-Right Toggle Button (Always visible)
        // -------------------------------------------------------------
        float tNdcX0 = (m_toggleX0 / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
        float tNdcX1 = (m_toggleX1 / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
        float tNdcY1 = 1.0f - (m_toggleY0 / static_cast<float>(screenHeight)) * 2.0f;
        float tNdcY0 = 1.0f - (m_toggleY1 / static_cast<float>(screenHeight)) * 2.0f;

        std::shared_ptr<Aion::Texture> toggleTex = m_visible ? m_texToggleBtnActive : m_texToggleBtn;
        if (!toggleTex)
        {
            toggleTex = m_texWhiteFallback;
        }

        Aion::Vector4 toggleTint = m_visible ? Aion::Vector4(0.85f, 1.0f, 0.85f, 0.95f)
                                             : Aion::Vector4(0.85f, 0.85f, 0.85f, 0.75f);
        if (m_toggleHovered)
        {
            toggleTint.w = 1.0f;
        }

        std::vector<float> toggleVerts;
        Add9Slice(tNdcX0, tNdcY0, tNdcX1, tNdcY1, 16.0f, 384.0f, 128.0f, toggleTint, toggleVerts, screenWidth, screenHeight);
        FlushBatch(toggleTex, toggleVerts);

        // Text inside toggle button
        std::string toggleLabel = m_visible ? "TOUCH: ON" : "TOUCH: OFF";
        float fontScale = 0.40f;
        float textW = MeasureKenneyString(toggleLabel, fontScale, screenWidth);
        float textX = (tNdcX0 + tNdcX1) * 0.5f - textW * 0.5f;
        float textY = (tNdcY0 + tNdcY1) * 0.5f + 8.0f * pixelH;

        Aion::Vector4 textColor = m_visible ? Aion::Vector4(0.05f, 0.20f, 0.05f, 1.0f)
                                            : Aion::Vector4(0.20f, 0.20f, 0.20f, 0.95f);
        std::vector<float> textVerts;
        DrawKenneyString(toggleLabel, textX, textY, fontScale, textColor, textVerts, screenWidth, screenHeight);
        FlushBatch(m_texFontAtlas ? m_texFontAtlas : m_texWhiteFallback, textVerts);

        // -------------------------------------------------------------
        // 2. Render Virtual Joystick & Jump Button (When Visible)
        // -------------------------------------------------------------
        if (m_visible)
        {
            // A. Joystick Base (Semi-transparent circular disc)
            float jbNdcX = (m_joyCenterX / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
            float jbNdcY = 1.0f - (m_joyCenterY / static_cast<float>(screenHeight)) * 2.0f;
            float jbRadX = m_joyBaseRadius * pixelW;
            float jbRadY = m_joyBaseRadius * pixelH;

            std::vector<float> joyBaseVerts;
            AddTexturedQuad(
                jbNdcX - jbRadX, jbNdcY - jbRadY,
                jbNdcX + jbRadX, jbNdcY + jbRadY,
                0.0f, 0.0f, 1.0f, 1.0f,
                Aion::Vector4(1.0f, 1.0f, 1.0f, 0.45f), joyBaseVerts);
            FlushBatch(m_texJoyBase ? m_texJoyBase : m_texWhiteFallback, joyBaseVerts);

            // B. Joystick Movable Knob
            float jkNdcX = (m_joyKnobX / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
            float jkNdcY = 1.0f - (m_joyKnobY / static_cast<float>(screenHeight)) * 2.0f;
            float jkRadX = m_joyKnobRadius * pixelW;
            float jkRadY = m_joyKnobRadius * pixelH;

            float knobAlpha = (m_joyTouchId >= 0) ? 0.90f : 0.70f;
            std::vector<float> joyKnobVerts;
            AddTexturedQuad(
                jkNdcX - jkRadX, jkNdcY - jkRadY,
                jkNdcX + jkRadX, jkNdcY + jkRadY,
                0.0f, 0.0f, 1.0f, 1.0f,
                Aion::Vector4(1.0f, 1.0f, 1.0f, knobAlpha), joyKnobVerts);
            FlushBatch(m_texJoyKnob ? m_texJoyKnob : m_texWhiteFallback, joyKnobVerts);

            // C. Jump Button Base
            float jmpNdcX = (m_jumpCenterX / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
            float jmpNdcY = 1.0f - (m_jumpCenterY / static_cast<float>(screenHeight)) * 2.0f;
            float jmpRadX = m_jumpRadius * pixelW;
            float jmpRadY = m_jumpRadius * pixelH;

            float pressOffset = m_jumpHeld ? (-3.0f * pixelH) : 0.0f;
            float jmpAlpha = m_jumpHeld ? 0.95f : 0.75f;
            Aion::Vector4 jmpTint = m_jumpHeld ? Aion::Vector4(0.80f, 0.95f, 0.80f, jmpAlpha)
                                               : Aion::Vector4(1.0f, 1.0f, 1.0f, jmpAlpha);

            std::vector<float> jumpBtnVerts;
            AddTexturedQuad(
                jmpNdcX - jmpRadX, jmpNdcY - jmpRadY + pressOffset,
                jmpNdcX + jmpRadX, jmpNdcY + jmpRadY + pressOffset,
                0.0f, 0.0f, 1.0f, 1.0f,
                jmpTint, jumpBtnVerts);
            FlushBatch(m_texJumpBtn ? m_texJumpBtn : m_texWhiteFallback, jumpBtnVerts);

            // D. Jump Button Upward Arrow Icon
            float iconRadX = (m_jumpRadius * 0.55f) * pixelW;
            float iconRadY = (m_jumpRadius * 0.55f) * pixelH;

            std::vector<float> jumpIconVerts;
            AddTexturedQuad(
                jmpNdcX - iconRadX, jmpNdcY - iconRadY + pressOffset,
                jmpNdcX + iconRadX, jmpNdcY + iconRadY + pressOffset,
                0.0f, 0.0f, 1.0f, 1.0f,
                Aion::Vector4(1.0f, 1.0f, 1.0f, 0.95f), jumpIconVerts);
            FlushBatch(m_texJumpIcon ? m_texJumpIcon : m_texWhiteFallback, jumpIconVerts);
        }

        Aion::RenderCommand::ApplyState(prevState);
    }
} // namespace BlockJumper

// ==============================================================================
// C-Linkage exports for WebAssembly browser bridge
// ==============================================================================
extern "C" {

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void BlockJumper_OnTouch(int id, int phase, float normX, float normY)
{
    BlockJumper::TouchControlsUI* instance = BlockJumper::TouchControlsUI::GetActiveInstance();
    if (instance)
    {
        instance->OnTouchEvent(id, phase, normX, normY);
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void BlockJumper_ClearTouches()
{
    BlockJumper::TouchControlsUI* instance = BlockJumper::TouchControlsUI::GetActiveInstance();
    if (instance)
    {
        instance->ClearAllTouches();
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void BlockJumper_SetTouchCapable(int capable)
{
    BlockJumper::TouchControlsUI* instance = BlockJumper::TouchControlsUI::GetActiveInstance();
    if (instance)
    {
        instance->SetTouchCapable(capable != 0);
    }
}

} // extern "C"
