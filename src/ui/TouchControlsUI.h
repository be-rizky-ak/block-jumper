#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Aion/Math/Vector2.h"
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
     * @brief Responsive On-Screen Touch Controls System for Block Jumper.
     * Provides:
     * - Left side: Virtual analog joystick for camera-relative omnidirectional player movement.
     * - Right side: Jump button with edge-triggered press detection.
     * - Top-right HUD: Toggle button to show or hide controls.
     *
     * Supports simultaneous multi-touch via browser touch events in WebGL2,
     * auto-detects touch capability, and supports mouse dragging for desktop simulation/testing.
     */
    class TouchControlsUI
    {
    public:
        TouchControlsUI();
        ~TouchControlsUI();

        void Initialize();
        void Shutdown();
        void Reset();

        /**
         * @brief Updates layout, scales controls to viewport, and handles mouse simulation.
         */
        void Update(float deltaTime, int screenWidth, int screenHeight, bool blockInput = false);

        /**
         * @brief Renders the touch controls overlay using Kenney UI vector assets.
         */
        void Render(int screenWidth, int screenHeight);

        // Visibility & Capability
        bool IsVisible() const { return m_visible; }
        void SetVisible(bool visible);
        void ToggleVisible() { SetVisible(!m_visible); }

        bool IsTouchCapable() const { return m_touchCapable; }
        void SetTouchCapable(bool capable);

        // Player input queries
        float GetForwardInput() const { return m_forwardInput; }
        float GetRightInput() const { return m_rightInput; }
        bool ConsumeJumpEdge();
        bool IsJumpHeld() const { return m_jumpHeld; }

        /**
         * @brief Dispatches touch events from the browser or test harness.
         * @param id Touch / Pointer ID (integer).
         * @param phase 0 = Down, 1 = Move, 2 = Up, 3 = Cancel.
         * @param normX Normalized canvas X [0.0 (left) .. 1.0 (right)].
         * @param normY Normalized canvas Y [0.0 (top) .. 1.0 (bottom)].
         */
        void OnTouchEvent(int id, int phase, float normX, float normY);

        /**
         * @brief Clears all active touch states (on blur, visibilitychange, pause, or disable).
         */
        void ClearAllTouches();

        // Testing & inspection queries
        bool IsTouchInJoystick(float screenX, float screenY) const;
        bool IsTouchInJump(float screenX, float screenY) const;
        bool IsTouchInToggle(float screenX, float screenY) const;

        float GetJoystickCenterX() const { return m_joyCenterX; }
        float GetJoystickCenterY() const { return m_joyCenterY; }
        float GetJoystickBaseRadius() const { return m_joyBaseRadius; }
        float GetJoystickKnobX() const { return m_joyKnobX; }
        float GetJoystickKnobY() const { return m_joyKnobY; }
        float GetJumpCenterX() const { return m_jumpCenterX; }
        float GetJumpCenterY() const { return m_jumpCenterY; }
        float GetJumpRadius() const { return m_jumpRadius; }

        static TouchControlsUI* GetActiveInstance() { return s_activeInstance; }

    private:
        void EnsureInitialized();
        std::shared_ptr<Aion::Texture> LoadTexture(const std::string& relativePath);
        std::shared_ptr<Aion::Texture> LoadSVGTexture(const std::string& relativePath, int targetWidth = 0, int targetHeight = 0);

        void AddTexturedQuad(
            float x0, float y0, float x1, float y1,
            float u0, float v0, float u1, float v1,
            const Aion::Vector4& tint,
            std::vector<float>& vertexBuffer);

        void Add9Slice(
            float x0, float y0, float x1, float y1,
            float cornerPx, float texW, float texH,
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

        void UpdateLayout(int screenWidth, int screenHeight);

    private:
        static TouchControlsUI* s_activeInstance;

        bool m_initialized = false;
        bool m_visible = false;
        bool m_touchCapable = false;
        int m_lastScreenWidth = 1280;
        int m_lastScreenHeight = 720;

        // Joystick layout & state (in screen pixels)
        float m_joyCenterX = 0.0f;
        float m_joyCenterY = 0.0f;
        float m_joyBaseRadius = 75.0f;
        float m_joyKnobRadius = 34.0f;
        float m_joyKnobX = 0.0f;
        float m_joyKnobY = 0.0f;
        int m_joyTouchId = -1;

        // Jump button layout & state (in screen pixels)
        float m_jumpCenterX = 0.0f;
        float m_jumpCenterY = 0.0f;
        float m_jumpRadius = 55.0f;
        int m_jumpTouchId = -1;
        bool m_jumpHeld = false;
        bool m_jumpEdge = false;

        // Toggle button layout & state (in screen pixels)
        float m_toggleX0 = 0.0f;
        float m_toggleY0 = 0.0f;
        float m_toggleX1 = 0.0f;
        float m_toggleY1 = 0.0f;
        int m_toggleTouchId = -1;
        bool m_toggleHovered = false;

        // Computed analog movement values
        float m_forwardInput = 0.0f;
        float m_rightInput = 0.0f;

        // Mouse simulation state for desktop testing
        bool m_prevMouseLeft = false;

        // Rendering resources
        std::unique_ptr<Aion::Shader> m_shader;
        std::unique_ptr<Aion::VertexArray> m_vertexArray;

        std::shared_ptr<Aion::Texture> m_texJoyBase;
        std::shared_ptr<Aion::Texture> m_texJoyKnob;
        std::shared_ptr<Aion::Texture> m_texJumpBtn;
        std::shared_ptr<Aion::Texture> m_texJumpIcon;
        std::shared_ptr<Aion::Texture> m_texToggleBtn;
        std::shared_ptr<Aion::Texture> m_texToggleBtnActive;
        std::shared_ptr<Aion::Texture> m_texFontAtlas;
        std::shared_ptr<Aion::Texture> m_texWhiteFallback;
    };
} // namespace BlockJumper
