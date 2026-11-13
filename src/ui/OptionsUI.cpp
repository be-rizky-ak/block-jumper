#include "OptionsUI.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "Aion/Assets/SVGImporter.h"
#include "Aion/Audio/AudioClip.h"
#include "Aion/Audio/AudioSystem.h"
#include "Aion/Core/Input.h"
#include "Aion/Core/KeyCodes.h"
#include "Aion/Core/MouseCodes.h"
#include "Aion/Core/Time.h"
#include "Aion/Renderer/RenderCommand.h"
#include "Aion/Renderer/Shader.h"
#include "Aion/Renderer/Texture.h"
#include "Aion/Renderer/TextureLibrary.h"
#include "Aion/Renderer/VertexArray.h"
#include "game/SaveManager.h"
#include "ui/KenneyFontAtlas.h"
#include "game/ResourcePath.h"

namespace BlockJumper
{
    namespace
    {
        std::string ResolveAssetPath(const std::string& path)
        {
            return ResolveAsset(path);
        }

        const char* s_OptionsVertexShader = R"(#version 330 core
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

        const char* s_OptionsFragmentShader = R"(#version 330 core
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

    OptionsUI::OptionsUI() = default;

    OptionsUI::~OptionsUI()
    {
        Shutdown();
    }

    std::shared_ptr<Aion::Texture> OptionsUI::LoadTexture(const std::string& relativePath)
    {
        std::string resolved = ResolveAssetPath(relativePath);
        return Aion::TextureLibrary::Load(resolved);
    }

    std::shared_ptr<Aion::Texture> OptionsUI::LoadSVGTexture(const std::string& relativePath, int targetWidth, int targetHeight)
    {
        std::string resolved = ResolveAssetPath(relativePath);
        return Aion::TextureLibrary::LoadSVG(resolved, targetWidth, targetHeight);
    }

    void OptionsUI::Initialize()
    {
        if (m_initialized)
        {
            return;
        }

        m_shader = std::make_unique<Aion::Shader>(s_OptionsVertexShader, s_OptionsFragmentShader);
        m_vertexArray = std::make_unique<Aion::VertexArray>();
        m_vertexArray->SetLayout({2, 2, 4});

        m_texWhiteFallback = Aion::TextureLibrary::GetWhiteTexture();

        // Load font atlas
        m_texFontAtlas = LoadTexture("assets/textures/kenney_ui-pack/png/kenney_future_atlas.png");

        // Load Kenney UI vector textures
        std::string svgBase = "assets/textures/kenney_ui-pack/vector/";
        m_texCardPanel    = LoadSVGTexture(svgBase + "Grey/button_rectangle_depth_flat.svg", 576, 384);
        m_texSliderTrack  = LoadSVGTexture(svgBase + "Grey/slide_horizontal_grey.svg", 384, 64);
        m_texSliderFill   = LoadSVGTexture(svgBase + "Green/slide_horizontal_color.svg", 384, 64);
        m_texSliderHandle = LoadSVGTexture(svgBase + "Grey/slide_hangle.svg", 128, 128);
        m_texBtnNormal    = LoadSVGTexture(svgBase + "Green/button_rectangle_depth_flat.svg", 384, 128);
        m_texBtnHover     = LoadSVGTexture(svgBase + "Green/button_rectangle_depth_gradient.svg", 384, 128);
        m_texBtnPressed   = LoadSVGTexture(svgBase + "Green/button_rectangle_flat.svg", 384, 128);
        m_texDivider      = LoadSVGTexture(svgBase + "Extra/divider.svg", 384, 16);

        // Preload preview audio clip for immediate SFX feedback
        std::string sfxFeedbackPath = ResolveAssetPath("assets/audio/kenney_digital-audio/audio/tone1.ogg");
        m_feedbackClip = Aion::AudioSystem::LoadClip(sfxFeedbackPath, Aion::AudioCategory::SFX, false);

        std::cout << "[OptionsUI] Initialized Kenney UI Options window." << std::endl;
        m_initialized = true;
    }

    void OptionsUI::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        m_texFontAtlas.reset();
        m_texCardPanel.reset();
        m_texSliderTrack.reset();
        m_texSliderFill.reset();
        m_texSliderHandle.reset();
        m_texBtnNormal.reset();
        m_texBtnHover.reset();
        m_texBtnPressed.reset();
        m_texDivider.reset();
        m_texWhiteFallback.reset();
        m_feedbackClip.reset();

        m_vertexArray.reset();
        m_shader.reset();

        m_initialized = false;
    }

    void OptionsUI::EnsureInitialized()
    {
        if (!m_initialized)
        {
            Initialize();
        }
    }

    void OptionsUI::Open(SaveManager* saveManager)
    {
        EnsureInitialized();
        m_saveManager = saveManager;

        // Synchronize with current audio subsystem category volumes
        m_masterVolume = std::clamp(Aion::AudioSystem::GetCategoryVolume(Aion::AudioCategory::Master), 0.0f, 1.0f);
        m_musicVolume = std::clamp(Aion::AudioSystem::GetCategoryVolume(Aion::AudioCategory::Music), 0.0f, 1.0f);
        m_sfxVolume = std::clamp(Aion::AudioSystem::GetCategoryVolume(Aion::AudioCategory::SFX), 0.0f, 1.0f);

        m_dirty = false;
        m_isOpen = true;
        m_activeDraggingSlider = AudioSliderCategory::None;
        m_escPrev = true;
        m_mousePrev = true;

        std::cout << "[OptionsUI] Options window opened (Master="
                  << static_cast<int>(std::round(m_masterVolume * 100.0f))
                  << "%, Music=" << static_cast<int>(std::round(m_musicVolume * 100.0f))
                  << "%, SFX=" << static_cast<int>(std::round(m_sfxVolume * 100.0f)) << "%)." << std::endl;
    }

    void OptionsUI::Close()
    {
        if (m_dirty && m_saveManager)
        {
            m_saveManager->SetVolumeSettings(m_masterVolume, m_musicVolume, m_sfxVolume);
            m_saveManager->Save();
            m_dirty = false;
            std::cout << "[OptionsUI] Volume settings persisted to " << m_saveManager->GetFilePath()
                      << " (Master=" << static_cast<int>(std::round(m_masterVolume * 100.0f))
                      << "%, Music=" << static_cast<int>(std::round(m_musicVolume * 100.0f))
                      << "%, SFX=" << static_cast<int>(std::round(m_sfxVolume * 100.0f)) << "%)." << std::endl;
        }

        m_isOpen = false;
        m_activeDraggingSlider = AudioSliderCategory::None;
        m_escPrev = true;
        m_mousePrev = true;
        std::cout << "[OptionsUI] Options window closed." << std::endl;
    }

    void OptionsUI::SetMasterVolume(float vol)
    {
        ApplyVolume(AudioSliderCategory::Master, vol);
    }

    void OptionsUI::SetMusicVolume(float vol)
    {
        ApplyVolume(AudioSliderCategory::Music, vol);
    }

    void OptionsUI::SetSFXVolume(float vol)
    {
        ApplyVolume(AudioSliderCategory::SFX, vol);
    }

    void OptionsUI::ApplyVolume(AudioSliderCategory category, float value)
    {
        float clamped = std::clamp(value, 0.0f, 1.0f);
        m_dirty = true;

        switch (category)
        {
            case AudioSliderCategory::Master:
                m_masterVolume = clamped;
                Aion::AudioSystem::SetCategoryVolume(Aion::AudioCategory::Master, clamped);
                break;
            case AudioSliderCategory::Music:
                m_musicVolume = clamped;
                Aion::AudioSystem::SetCategoryVolume(Aion::AudioCategory::Music, clamped);
                break;
            case AudioSliderCategory::SFX:
                m_sfxVolume = clamped;
                Aion::AudioSystem::SetCategoryVolume(Aion::AudioCategory::SFX, clamped);
                TriggerSFXFeedback(false);
                break;
            default:
                break;
        }
    }

    void OptionsUI::TriggerSFXFeedback(bool force)
    {
        float now = Aion::Time::GetTime();
        // Rate-limit immediate SFX feedback during dragging so sound is crisp and not noisy
        if (force || (now - m_lastSFXFeedbackTime >= 0.14f && std::abs(m_sfxVolume - m_lastSFXFeedbackVol) >= 0.02f))
        {
            if (m_feedbackClip && m_feedbackClip->IsValid())
            {
                Aion::AudioSystem::PlayOneShot(m_feedbackClip, 1.0f);
            }
            m_lastSFXFeedbackTime = now;
            m_lastSFXFeedbackVol = m_sfxVolume;
        }
    }

    bool OptionsUI::IsMouseOverCloseButton(float ndcX, float ndcY) const
    {
        return (ndcX >= m_closeBtnMinX && ndcX <= m_closeBtnMaxX &&
                ndcY >= m_closeBtnMinY && ndcY <= m_closeBtnMaxY);
    }

    bool OptionsUI::Update(float deltaTime, int screenWidth, int screenHeight)
    {
        (void)deltaTime;
        if (!m_isOpen)
        {
            return false;
        }

        if (screenWidth <= 0 || screenHeight <= 0)
        {
            return true;
        }

        float mouseX = Aion::Input::GetMouseX();
        float mouseY = Aion::Input::GetMouseY();
        float ndcX = (mouseX / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
        float ndcY = 1.0f - (mouseY / static_cast<float>(screenHeight)) * 2.0f;

        bool mouseButtonDown = Aion::Input::GetMouseButton(Aion::Mouse::ButtonLeft);

        // 1. Keyboard Shortcut: ESC or Backspace closes Options
        bool escCurrent = Aion::Input::GetKey(Aion::Key::Escape) || Aion::Input::GetKey(Aion::Key::Backspace);
        if (escCurrent && !m_escPrev)
        {
            m_escPrev = true;
            Close();
            return true;
        }
        m_escPrev = escCurrent;

        // 2. Close Button Click
        bool mouseJustClicked = (mouseButtonDown && !m_mousePrev);
        m_mousePrev = mouseButtonDown;

        if (mouseJustClicked && IsMouseOverCloseButton(ndcX, ndcY))
        {
            m_closeButtonPressed = true;
            Close();
            return true;
        }

        // 3. Slider Dragging and Click-to-Position
        float trackPaddingY = 18.0f * (2.0f / static_cast<float>(screenHeight));

        if (mouseButtonDown)
        {
            if (m_activeDraggingSlider == AudioSliderCategory::None && mouseJustClicked)
            {
                // Test Master track hit
                if (ndcX >= m_sliderMasterNDC.trackX0 - 0.02f && ndcX <= m_sliderMasterNDC.trackX1 + 0.02f &&
                    ndcY >= m_sliderMasterNDC.trackY0 - trackPaddingY && ndcY <= m_sliderMasterNDC.trackY1 + trackPaddingY)
                {
                    m_activeDraggingSlider = AudioSliderCategory::Master;
                }
                // Test Music track hit
                else if (ndcX >= m_sliderMusicNDC.trackX0 - 0.02f && ndcX <= m_sliderMusicNDC.trackX1 + 0.02f &&
                         ndcY >= m_sliderMusicNDC.trackY0 - trackPaddingY && ndcY <= m_sliderMusicNDC.trackY1 + trackPaddingY)
                {
                    m_activeDraggingSlider = AudioSliderCategory::Music;
                }
                // Test SFX track hit
                else if (ndcX >= m_sliderSFXNDC.trackX0 - 0.02f && ndcX <= m_sliderSFXNDC.trackX1 + 0.02f &&
                         ndcY >= m_sliderSFXNDC.trackY0 - trackPaddingY && ndcY <= m_sliderSFXNDC.trackY1 + trackPaddingY)
                {
                    m_activeDraggingSlider = AudioSliderCategory::SFX;
                }
            }

            // Update active slider value based on mouse NDC
            if (m_activeDraggingSlider != AudioSliderCategory::None)
            {
                SliderNDC bounds;
                if (m_activeDraggingSlider == AudioSliderCategory::Master) bounds = m_sliderMasterNDC;
                else if (m_activeDraggingSlider == AudioSliderCategory::Music) bounds = m_sliderMusicNDC;
                else if (m_activeDraggingSlider == AudioSliderCategory::SFX) bounds = m_sliderSFXNDC;

                float span = bounds.trackX1 - bounds.trackX0;
                if (span > 0.0001f)
                {
                    float normalized = std::clamp((ndcX - bounds.trackX0) / span, 0.0f, 1.0f);
                    ApplyVolume(m_activeDraggingSlider, normalized);
                }
            }
        }
        else
        {
            // Mouse released: if SFX slider was being dragged, play final feedback sound
            if (m_activeDraggingSlider == AudioSliderCategory::SFX)
            {
                TriggerSFXFeedback(true);
            }
            m_activeDraggingSlider = AudioSliderCategory::None;
        }

        // Options window absorbs all input, preventing gameplay leak-through
        return true;
    }

    void OptionsUI::AddColoredQuad(
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

    void OptionsUI::AddTexturedQuad(
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

    void OptionsUI::Add9Slice(
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

    void OptionsUI::DrawKenneyString(
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

    float OptionsUI::MeasureKenneyString(
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

    void OptionsUI::FlushBatch(const std::shared_ptr<Aion::Texture>& texture, const std::vector<float>& vertexBuffer)
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

    void OptionsUI::Render(int screenWidth, int screenHeight)
    {
        if (!m_isOpen || screenWidth <= 0 || screenHeight <= 0)
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

        // 1. Fullscreen Dimming Overlay
        std::vector<float> dimVerts;
        AddColoredQuad(-1.0f, -1.0f, 1.0f, 1.0f,
                       Aion::Vector4(0.04f, 0.06f, 0.10f, 0.75f), dimVerts);
        FlushBatch(nullptr, dimVerts);

        // 2. Responsive Central Dialog (560px x 420px at 1280x720, proportionally scaling)
        float cardHalfW = 280.0f * pixelW;
        float cardHalfH = 210.0f * pixelH;
        float cardCenterY = 0.02f;
        float cardX0 = -cardHalfW;
        float cardX1 =  cardHalfW;
        float cardY0 = cardCenterY - cardHalfH;
        float cardY1 = cardCenterY + cardHalfH;

        // Dialog shadow
        std::vector<float> shadowVerts;
        AddColoredQuad(cardX0 + 10.0f * pixelW, cardY0 - 10.0f * pixelH,
                       cardX1 + 10.0f * pixelW, cardY1 - 10.0f * pixelH,
                       Aion::Vector4(0.02f, 0.03f, 0.05f, 0.55f), shadowVerts);
        FlushBatch(nullptr, shadowVerts);

        // Dialog panel body (9-slice Kenney Card panel)
        std::vector<float> cardVerts;
        Add9Slice(cardX0, cardY0, cardX1, cardY1, 28.0f, 576.0f, 384.0f,
                  Aion::Vector4(0.14f, 0.18f, 0.24f, 0.98f), cardVerts,
                  screenWidth, screenHeight);
        FlushBatch(m_texCardPanel ? m_texCardPanel : m_texWhiteFallback, cardVerts);

        // 3. Header Title & Accent Divider
        std::vector<float> divVerts;
        float divY = cardY1 - 70.0f * pixelH;
        AddTexturedQuad(cardX0 + 28.0f * pixelW, divY - 4.0f * pixelH,
                        cardX1 - 28.0f * pixelW, divY + 4.0f * pixelH,
                        0.0f, 0.0f, 1.0f, 1.0f,
                        Aion::Vector4(0.95f, 0.75f, 0.25f, 0.85f), divVerts);
        FlushBatch(m_texDivider, divVerts);

        std::vector<float> textVerts;
        std::string titleText = "OPTIONS";
        float titleScale = 0.80f;
        float titleW = MeasureKenneyString(titleText, titleScale, screenWidth, 2.5f * pixelW);
        float titleX = -titleW * 0.5f;
        float titleY = cardY1 - 32.0f * pixelH;

        // Shadow behind title
        DrawKenneyString(titleText, titleX + 3.0f * pixelW, titleY - 3.0f * pixelH,
                         titleScale, Aion::Vector4(0.04f, 0.05f, 0.08f, 0.90f),
                         textVerts, screenWidth, screenHeight, 2.5f * pixelW);
        // Gold title text
        DrawKenneyString(titleText, titleX, titleY,
                         titleScale, Aion::Vector4(1.0f, 0.82f, 0.25f, 1.0f),
                         textVerts, screenWidth, screenHeight, 2.5f * pixelW);

        // 4. Sliders Configuration
        float sliderMarginX = 36.0f * pixelW;
        float trackLeftX = cardX0 + sliderMarginX;
        float trackRightX = cardX1 - sliderMarginX;
        float trackSpan = trackRightX - trackLeftX;
        float trackH = 16.0f * pixelH;
        float handleW = 20.0f * pixelW;
        float handleH = 28.0f * pixelH;

        struct SliderRowConfig
        {
            const char* label;
            float volume;
            float rowCenterY;
            AudioSliderCategory category;
            SliderNDC* outBounds;
        };

        float row1Y = cardY1 - 110.0f * pixelH;
        float row2Y = cardY1 - 180.0f * pixelH;
        float row3Y = cardY1 - 250.0f * pixelH;

        SliderRowConfig rows[3] = {
            {"MASTER VOLUME", m_masterVolume, row1Y, AudioSliderCategory::Master, &m_sliderMasterNDC},
            {"MUSIC VOLUME",  m_musicVolume,  row2Y, AudioSliderCategory::Music,  &m_sliderMusicNDC},
            {"SFX VOLUME",    m_sfxVolume,    row3Y, AudioSliderCategory::SFX,    &m_sliderSFXNDC}
        };

        std::vector<float> trackVerts;
        std::vector<float> fillVerts;
        std::vector<float> handleVerts;

        for (const auto& row : rows)
        {
            // Record interaction bounding box
            row.outBounds->trackX0 = trackLeftX;
            row.outBounds->trackX1 = trackRightX;
            row.outBounds->trackY0 = row.rowCenterY - trackH * 0.5f;
            row.outBounds->trackY1 = row.rowCenterY + trackH * 0.5f;

            // Row Label Text
            float labelScale = 0.44f;
            float labelY = row.rowCenterY + 28.0f * pixelH;
            DrawKenneyString(row.label, trackLeftX + 2.0f * pixelW, labelY - 2.0f * pixelH,
                             labelScale, Aion::Vector4(0.02f, 0.03f, 0.05f, 0.85f),
                             textVerts, screenWidth, screenHeight, 1.5f * pixelW);
            DrawKenneyString(row.label, trackLeftX, labelY,
                             labelScale, Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f),
                             textVerts, screenWidth, screenHeight, 1.5f * pixelW);

            // Percentage Label (e.g. "100%", "50%", "0%")
            int pct = static_cast<int>(std::round(row.volume * 100.0f));
            std::string pctStr = std::to_string(pct) + "%";
            float pctW = MeasureKenneyString(pctStr, labelScale, screenWidth, 1.5f * pixelW);
            float pctX = trackRightX - pctW;

            DrawKenneyString(pctStr, pctX + 2.0f * pixelW, labelY - 2.0f * pixelH,
                             labelScale, Aion::Vector4(0.02f, 0.03f, 0.05f, 0.85f),
                             textVerts, screenWidth, screenHeight, 1.5f * pixelW);
            DrawKenneyString(pctStr, pctX, labelY,
                             labelScale, Aion::Vector4(0.35f, 0.90f, 0.65f, 1.0f),
                             textVerts, screenWidth, screenHeight, 1.5f * pixelW);

            // Slider Track Background
            float tY0 = row.outBounds->trackY0;
            float tY1 = row.outBounds->trackY1;
            AddTexturedQuad(trackLeftX, tY0, trackRightX, tY1,
                            0.0f, 0.0f, 1.0f, 1.0f,
                            Aion::Vector4(0.9f, 0.9f, 0.95f, 0.85f), trackVerts);

            // Slider Fill Bar (proportional to volume)
            float clampedVol = std::clamp(row.volume, 0.0f, 1.0f);
            if (clampedVol > 0.001f)
            {
                float fillRightX = trackLeftX + trackSpan * clampedVol;
                AddTexturedQuad(trackLeftX, tY0, fillRightX, tY1,
                                0.0f, 0.0f, clampedVol, 1.0f,
                                Aion::Vector4(0.20f, 0.85f, 0.50f, 0.95f), fillVerts);
            }

            // Slider Knob Handle (centered at current volume level)
            float handleCenterX = trackLeftX + trackSpan * clampedVol;
            float hX0 = handleCenterX - handleW * 0.5f;
            float hX1 = handleCenterX + handleW * 0.5f;
            float hY0 = row.rowCenterY - handleH * 0.5f;
            float hY1 = row.rowCenterY + handleH * 0.5f;

            AddTexturedQuad(hX0, hY0, hX1, hY1,
                            0.0f, 0.0f, 1.0f, 1.0f,
                            Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f), handleVerts);
        }

        // Flush slider graphics batches
        FlushBatch(m_texSliderTrack ? m_texSliderTrack : m_texWhiteFallback, trackVerts);
        FlushBatch(m_texSliderFill ? m_texSliderFill : m_texWhiteFallback, fillVerts);
        FlushBatch(m_texSliderHandle ? m_texSliderHandle : m_texWhiteFallback, handleVerts);

        // 5. Close / Back Button at Bottom
        float btnHalfW = 110.0f * pixelW;
        float btnHalfH = 24.0f * pixelH;
        float btnCenterY = cardY0 + 44.0f * pixelH;

        m_closeBtnMinX = -btnHalfW;
        m_closeBtnMaxX =  btnHalfW;
        m_closeBtnMinY = btnCenterY - btnHalfH;
        m_closeBtnMaxY = btnCenterY + btnHalfH;

        float mx = Aion::Input::GetMouseX();
        float my = Aion::Input::GetMouseY();
        float ndcX = (mx / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
        float ndcY = 1.0f - (my / static_cast<float>(screenHeight)) * 2.0f;

        bool hovered = IsMouseOverCloseButton(ndcX, ndcY);
        bool pressed = m_closeButtonPressed || (hovered && Aion::Input::GetMouseButton(Aion::Mouse::ButtonLeft));

        if (m_closeBtnVisualOverride == 0) // Force Normal
        {
            hovered = false;
            pressed = false;
        }
        else if (m_closeBtnVisualOverride == 1) // Force Hover
        {
            hovered = true;
            pressed = false;
        }
        else if (m_closeBtnVisualOverride == 2) // Force Pressed
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

        float pressYOffset = pressed ? (-2.0f * pixelH) : 0.0f;

        // Button drop shadow
        std::vector<float> btnShadowVerts;
        AddColoredQuad(m_closeBtnMinX + 4.0f * pixelW, m_closeBtnMinY - 4.0f * pixelH,
                       m_closeBtnMaxX + 4.0f * pixelW, m_closeBtnMaxY - 4.0f * pixelH,
                       Aion::Vector4(0.02f, 0.03f, 0.05f, 0.40f), btnShadowVerts);
        FlushBatch(nullptr, btnShadowVerts);

        // 9-slice button body
        std::vector<float> btnVerts;
        Add9Slice(m_closeBtnMinX, m_closeBtnMinY + pressYOffset,
                  m_closeBtnMaxX, m_closeBtnMaxY + pressYOffset,
                  18.0f, 384.0f, 128.0f,
                  Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f), btnVerts,
                  screenWidth, screenHeight);
        FlushBatch(activeBtnTex ? activeBtnTex : m_texWhiteFallback, btnVerts);

        // Close button text
        std::string btnText = "BACK [ESC]";
        float btnTextScale = 0.46f;
        float btnTextW = MeasureKenneyString(btnText, btnTextScale, screenWidth, 1.5f * pixelW);
        float btnTextX = -btnTextW * 0.5f;
        float btnTextY = btnCenterY + 8.0f * pixelH + pressYOffset;

        DrawKenneyString(btnText, btnTextX + 2.0f * pixelW, btnTextY - 2.0f * pixelH,
                         btnTextScale, Aion::Vector4(0.04f, 0.15f, 0.08f, 0.90f),
                         textVerts, screenWidth, screenHeight, 1.5f * pixelW);
        DrawKenneyString(btnText, btnTextX, btnTextY,
                         btnTextScale, Aion::Vector4(1.0f, 1.0f, 1.0f, 1.0f),
                         textVerts, screenWidth, screenHeight, 1.5f * pixelW);

        // Flush typography
        FlushBatch(m_texFontAtlas, textVerts);

        Aion::RenderCommand::ApplyState(prevState);
    }
} // namespace BlockJumper
