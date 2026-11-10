#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Aion/Audio/AudioCategory.h"
#include "Aion/Math/Vector4.h"

namespace Aion
{
    class AudioClip;
    class Shader;
    class Texture;
    class VertexArray;
}

namespace BlockJumper
{
    class SaveManager;

    enum class AudioSliderCategory
    {
        None,
        Master,
        Music,
        SFX
    };

    /**
     * @brief Responsive Options UI window for audio configuration in Block Jumper.
     * Accessible from in-game HUD and Game Over screen.
     * Manages Master, Music, and SFX volume sliders (0–100%),
     * connects directly to Aion::AudioSystem::SetCategoryVolume,
     * provides immediate audio feedback, and persists settings via SaveManager.
     */
    class OptionsUI
    {
    public:
        OptionsUI();
        ~OptionsUI();

        /**
         * @brief Initializes shaders, VAO/VBO, Kenney SVG vector UI textures, and font atlas.
         */
        void Initialize();

        /**
         * @brief Cleans up GPU textures, shaders, and buffers.
         */
        void Shutdown();

        /**
         * @brief Opens the Options window, syncing slider values with current AudioSystem settings.
         */
        void Open(SaveManager* saveManager = nullptr);

        /**
         * @brief Closes the Options window, committing changes to SaveManager disk persistence.
         */
        void Close();

        bool IsOpen() const { return m_isOpen; }

        /**
         * @brief Handles mouse interaction (slider dragging, click-to-position, close button)
         * and keyboard shortcuts (ESC).
         * @return true if Options is open and consuming input (blocks gameplay input).
         */
        bool Update(float deltaTime, int screenWidth, int screenHeight);

        /**
         * @brief Renders the responsive Options dialog at the given resolution (e.g. 1280x720, 960x540).
         */
        void Render(int screenWidth, int screenHeight);

        // Volume accessors (normalized 0.0f - 1.0f range)
        float GetMasterVolume() const { return m_masterVolume; }
        float GetMusicVolume() const { return m_musicVolume; }
        float GetSFXVolume() const { return m_sfxVolume; }

        // Direct volume setters (useful for testing and programmatic initialization)
        void SetMasterVolume(float vol);
        void SetMusicVolume(float vol);
        void SetSFXVolume(float vol);

        // Visual state override for deterministic testing and screenshot verification
        // -1: Auto (dynamic input), 0: Normal, 1: Hover, 2: Pressed
        void SetCloseButtonVisualStateOverride(int state) { m_closeBtnVisualOverride = state; }
        int GetCloseButtonVisualStateOverride() const { return m_closeBtnVisualOverride; }

        void SetFeedbackClip(std::shared_ptr<Aion::AudioClip> clip) { m_feedbackClip = std::move(clip); }

    private:
        void EnsureInitialized();
        std::shared_ptr<Aion::Texture> LoadTexture(const std::string& relativePath);
        std::shared_ptr<Aion::Texture> LoadSVGTexture(const std::string& relativePath, int targetWidth = 0, int targetHeight = 0);

        void AddColoredQuad(
            float x0, float y0, float x1, float y1,
            const Aion::Vector4& color,
            std::vector<float>& vertexBuffer);

        void AddTexturedQuad(
            float x0, float y0, float x1, float y1,
            float u0, float v0, float u1, float v1,
            const Aion::Vector4& tint,
            std::vector<float>& vertexBuffer);

        void Add9Slice(
            float x0, float y0, float x1, float y1,
            float cornerPx,
            float texW, float texH,
            const Aion::Vector4& tint,
            std::vector<float>& vertexBuffer,
            int screenW, int screenH);

        void DrawKenneyString(
            const std::string& text,
            float startX, float startY,
            float scale,
            const Aion::Vector4& color,
            std::vector<float>& vertexBuffer,
            int screenW, int screenH,
            float letterSpacing = 0.0f);

        float MeasureKenneyString(
            const std::string& text,
            float scale,
            int screenW,
            float letterSpacing = 0.0f) const;

        void FlushBatch(const std::shared_ptr<Aion::Texture>& texture, const std::vector<float>& vertexBuffer);

        void ApplyVolume(AudioSliderCategory category, float value);
        void TriggerSFXFeedback(bool force = false);

        bool IsMouseOverCloseButton(float ndcX, float ndcY) const;

    private:
        bool m_initialized = false;
        bool m_isOpen = false;
        SaveManager* m_saveManager = nullptr;

        std::unique_ptr<Aion::Shader> m_shader;
        std::unique_ptr<Aion::VertexArray> m_vertexArray;

        // Kenney UI Textures
        std::shared_ptr<Aion::Texture> m_texFontAtlas;
        std::shared_ptr<Aion::Texture> m_texCardPanel;
        std::shared_ptr<Aion::Texture> m_texSliderTrack;
        std::shared_ptr<Aion::Texture> m_texSliderFill;
        std::shared_ptr<Aion::Texture> m_texSliderHandle;
        std::shared_ptr<Aion::Texture> m_texBtnNormal;
        std::shared_ptr<Aion::Texture> m_texBtnHover;
        std::shared_ptr<Aion::Texture> m_texBtnPressed;
        std::shared_ptr<Aion::Texture> m_texDivider;
        std::shared_ptr<Aion::Texture> m_texWhiteFallback;

        // Feedback sound effect
        std::shared_ptr<Aion::AudioClip> m_feedbackClip;

        // Current volume settings (0.0f - 1.0f)
        float m_masterVolume = 1.0f;
        float m_musicVolume = 0.5f;
        float m_sfxVolume = 0.8f;
        bool m_dirty = false;

        // Dragging & input tracking
        AudioSliderCategory m_activeDraggingSlider = AudioSliderCategory::None;
        float m_lastSFXFeedbackTime = 0.0f;
        float m_lastSFXFeedbackVol = -1.0f;

        // Interactive bounding boxes in NDC
        struct SliderNDC
        {
            float trackX0 = 0.0f;
            float trackX1 = 0.0f;
            float trackY0 = 0.0f;
            float trackY1 = 0.0f;
        };
        SliderNDC m_sliderMasterNDC;
        SliderNDC m_sliderMusicNDC;
        SliderNDC m_sliderSFXNDC;

        mutable float m_closeBtnMinX = -0.15f;
        mutable float m_closeBtnMaxX =  0.15f;
        mutable float m_closeBtnMinY = -0.28f;
        mutable float m_closeBtnMaxY = -0.18f;

        bool m_closeButtonPressed = false;
        int m_closeBtnVisualOverride = -1;

        bool m_escPrev = true;
        bool m_mousePrev = true;
    };
} // namespace BlockJumper
