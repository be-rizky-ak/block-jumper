#pragma once

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
     * @brief Dedicated full-screen startup loading screen for Block Jumper.
     * Completely obscures the viewport on launch with an on-theme background,
     * prominent "Block Jumper" title, loading progress bar, status text,
     * smooth fade-out transition, and loading error handling.
     */
    class LoadingScreen
    {
    public:
        LoadingScreen();
        ~LoadingScreen();

        void Initialize();
        void Shutdown();
        bool IsInitialized() const { return m_initialized; }

        /**
         * @brief Renders the dedicated full-screen loading screen.
         * @param screenWidth Viewport width in pixels.
         * @param screenHeight Viewport height in pixels.
         * @param alpha Overall opacity (1.0 = fully opaque, fading down to 0.0).
         * @param progress Progress fraction [0.0, 1.0], or negative for indeterminate pulse.
         * @param statusText Supporting status text (e.g. "PREPARING GAME ASSETS...").
         * @param errorMessage Optional error message if loading failed.
         * @param deltaTime Elapsed frame time for indeterminate pulse animations.
         */
        void Render(
            int screenWidth,
            int screenHeight,
            float alpha = 1.0f,
            float progress = 0.0f,
            const std::string& statusText = "PREPARING GAME ASSETS...",
            const std::string& errorMessage = "",
            float deltaTime = 0.016f);

    private:
        void EnsureInitialized();
        void AddColoredQuad(float x0, float y0, float x1, float y1,
                            const Aion::Vector4& color, std::vector<float>& buffer);
        void AddTexturedQuad(float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1,
                             const Aion::Vector4& color, std::vector<float>& buffer);
        void DrawKenneyString(const std::string& text, float x, float y, float scale,
                              const Aion::Vector4& color, std::vector<float>& vertexBuffer,
                              int screenW, int screenH, float letterSpacing = 0.0f);
        float MeasureKenneyString(const std::string& text, float scale, int screenW, float letterSpacing = 0.0f) const;
        void FlushBatch(const std::shared_ptr<Aion::Texture>& texture, const std::vector<float>& vertexBuffer);

    private:
        bool m_initialized = false;
        std::unique_ptr<Aion::Shader> m_shader;
        std::unique_ptr<Aion::VertexArray> m_vertexArray;
        std::shared_ptr<Aion::Texture> m_texFontAtlas;
        std::shared_ptr<Aion::Texture> m_texWhiteFallback;
        float m_indeterminateTime = 0.0f;
    };
} // namespace BlockJumper
