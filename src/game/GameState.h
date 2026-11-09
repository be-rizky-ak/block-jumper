#pragma once

namespace BlockJumper
{
    /**
     * @brief Clear gameplay state representation for the fall detection, game over,
     * and retry lifecycle.
     */
    enum class GameState
    {
        Playing,
        Falling,
        GameOver
    };

    inline const char* GetGameStateName(GameState state)
    {
        switch (state)
        {
            case GameState::Playing:  return "Playing";
            case GameState::Falling:  return "Falling";
            case GameState::GameOver: return "GameOver";
            default:                  return "Unknown";
        }
    }
} // namespace BlockJumper
