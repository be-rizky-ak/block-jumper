#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Aion/Math/Vector4.h"

namespace BlockJumper
{
    /**
     * @brief Simple, readable Game Over UI with an interactive Retry action.
     * Renders directly via modern OpenGL 3.3 Core Profile in the aesthetic
     * of Block Jumper (crisp 3D/2D block geometry, high contrast PBR colors).
     */
    class GameOverUI
    {
    public:
        GameOverUI() = default;
        ~GameOverUI();

        /**
         * @brief Initializes OpenGL shaders, VAO, and VBO for UI rendering.
         */
        void Initialize();

        /**
         * @brief Cleans up GPU resources.
         */
        void Shutdown();

        /**
         * @brief Checks if the Retry action was requested via keyboard (R, Space, Enter)
         * or mouse click on the Retry button.
         *
         * @param screenWidth Current viewport width.
         * @param screenHeight Current viewport height.
         * @return true if Retry action was triggered.
         */
        bool CheckRetryAction(int screenWidth, int screenHeight) const;

        /**
         * @brief Renders the in-game HUD at top-left during active gameplay.
         * Displays current run distance, best distance, and difficulty stage.
         */
        void RenderHUD(
            int screenWidth,
            int screenHeight,
            float currentDistance,
            int difficultyStage,
            const char* stageName,
            float bestDistance);

        /**
         * @brief Renders the enhanced Game Over overlay presenting run distance,
         * best distance record, new-record notification, cleared segments, and Retry action.
         */
        void RenderGameOver(
            int screenWidth,
            int screenHeight,
            float finalDistance,
            float bestDistance,
            bool isNewRecord,
            int completedSegments = 0,
            const char* difficultyStage = "Easy");

        /**
         * @brief Renders the Game Over overlay (backward-compatible signature).
         *
         * @param screenWidth Viewport width.
         * @param screenHeight Viewport height.
         * @param completedSegments Segments cleared in the run.
         * @param difficultyStage Current difficulty stage name.
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

    private:
        void EnsureInitialized();
        void AddQuad(
            float x0, float y0, float x1, float y1,
            const Aion::Vector4& color,
            std::vector<float>& vertexBuffer);
        void DrawGlyph(
            char c,
            float x, float y,
            float pixelW, float pixelH,
            const Aion::Vector4& color,
            std::vector<float>& vertexBuffer);
        void DrawString(
            const std::string& text,
            float startX, float startY,
            float pixelW, float pixelH,
            float letterSpacing,
            const Aion::Vector4& color,
            std::vector<float>& vertexBuffer);

    private:
        bool m_initialized = false;
        uint32_t m_shaderProgram = 0;
        uint32_t m_vao = 0;
        uint32_t m_vbo = 0;

        // Button bounds in Normalized Device Coordinates [-1.0, 1.0]
        float m_btnMinX = -0.26f;
        float m_btnMaxX =  0.26f;
        float m_btnMinY = -0.22f;
        float m_btnMaxY = -0.06f;
    };
} // namespace BlockJumper
