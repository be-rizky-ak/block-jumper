#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Aion/Math/Vector4.h"

namespace Aion
{
    class Shader;
    class Texture;
    class VertexArray;
}

namespace BlockJumper
{
    /**
     * @brief Polished Kenney UI interface system for Block Jumper.
     * Renders responsive in-game HUD and Game Over screen using Kenney UI Pack
     * 9-slice panels, buttons, icons, and the official Kenney Future typography.
     */
    class GameOverUI
    {
    public:
        GameOverUI();
        ~GameOverUI();

        /**
         * @brief Initializes OpenGL shaders, VAO, VBO, and loads Kenney UI textures & font atlas.
         */
        void Initialize();

        /**
         * @brief Cleans up GPU texture and buffer resources.
         */
        void Shutdown();

        /**
         * @brief Checks if the Retry action was requested via keyboard (R, Space, Enter)
         * or mouse click on the Retry button.
         */
        bool CheckRetryAction(int screenWidth, int screenHeight);

        /**
         * @brief Renders a clean Kenney-styled loading indicator.
         */
        void RenderLoading(
            int screenWidth,
            int screenHeight,
            float progress,
            const std::string& statusText = "LOADING...");

        /**
         * @brief Renders the redesigned in-game HUD at top-left during active gameplay.
         * Displays current run distance, best distance, difficulty stage, and milestone feedback.
         */
        void RenderHUD(
            int screenWidth,
            int screenHeight,
            float currentDistance,
            int difficultyStage,
            const char* stageName,
            float bestDistance,
            float stageMilestoneTimer = 0.0f);

        /**
         * @brief Renders the enhanced Game Over overlay presenting run distance,
         * best distance record, new-record notification, cleared segments, and interactive Retry button.
         */
        void RenderGameOver(
            int screenWidth,
            int screenHeight,
            float finalDistance,
            float bestDistance,
            bool isNewRecord,
            int completedSegments = 0,
            const char* difficultyStage = "Easy",
            float gameOverDuration = 1.0f);

        /**
         * @brief Backward-compatible Render() signature for existing tests and call sites.
         */
        void Render(
            int screenWidth,
            int screenHeight,
            int completedSegments = 0,
            const char* difficultyStage = "Easy");

        /**
         * @brief Tests if given screen coordinates fall inside the Retry button bounds.
         */
        bool IsMouseOverButton(float mouseX, float mouseY, int screenWidth, int screenHeight) const;

        /**
         * @brief Sets button visual state override for deterministic testing and rendering.
         * -1: Auto (dynamic input), 0: Normal, 1: Hover, 2: Pressed.
         */
        void SetButtonVisualStateOverride(int state) { m_buttonVisualStateOverride = state; }
        int GetButtonVisualStateOverride() const { return m_buttonVisualStateOverride; }

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

    private:
        bool m_initialized = false;
        std::unique_ptr<Aion::Shader> m_shader;
        std::unique_ptr<Aion::VertexArray> m_vertexArray;

        // Kenney UI Textures
        std::shared_ptr<Aion::Texture> m_texFontAtlas;
        std::shared_ptr<Aion::Texture> m_texHudPanel;
        std::shared_ptr<Aion::Texture> m_texCardPanel;
        std::shared_ptr<Aion::Texture> m_texBtnNormal;
        std::shared_ptr<Aion::Texture> m_texBtnHover;
        std::shared_ptr<Aion::Texture> m_texBtnPressed;
        std::shared_ptr<Aion::Texture> m_texStarGold;
        std::shared_ptr<Aion::Texture> m_texIconRepeat;
        std::shared_ptr<Aion::Texture> m_texStageIndicator;
        std::shared_ptr<Aion::Texture> m_texDivider;
        std::shared_ptr<Aion::Texture> m_texWhiteFallback;

        // Dynamic responsive button bounds in NDC
        mutable float m_btnMinX = -0.25f;
        mutable float m_btnMaxX =  0.25f;
        mutable float m_btnMinY = -0.22f;
        mutable float m_btnMaxY = -0.08f;

        bool m_buttonPressed = false;
        int m_buttonVisualStateOverride = -1;
        float m_gameOverAnimTime = 0.0f;
    };
} // namespace BlockJumper
