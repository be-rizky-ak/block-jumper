#pragma once

#include <string>

namespace BlockJumper
{
    /**
     * @brief Manages local persistence of player record telemetry (Best Distance).
     * Resilient against missing, corrupted, or invalid saved data.
     */
    class SaveManager
    {
    public:
        SaveManager();
        explicit SaveManager(std::string filepath);
        ~SaveManager() = default;

        /**
         * @brief Loads the best distance from disk.
         * Falls back safely to 0.0f on missing or corrupted file.
         */
        bool Load();

        /**
         * @brief Persists current best distance to disk.
         */
        bool Save();

        /**
         * @brief Evaluates whether current run distance exceeds the best record.
         * If it does, updates the best distance and persists to disk.
         *
         * @param runDistance Final distance of the current run.
         * @return true if a new record was achieved and updated.
         */
        bool TryUpdateBestDistance(float runDistance);

        float GetBestDistance() const { return m_bestDistance; }

        /**
         * @brief Directly sets the best distance in memory (useful for testing).
         */
        void SetBestDistance(float distance) { m_bestDistance = distance; }

        // Volume configuration accessors (0.0f to 1.0f range)
        float GetMasterVolume() const { return m_masterVolume; }
        float GetMusicVolume() const { return m_musicVolume; }
        float GetSFXVolume() const { return m_sfxVolume; }

        void SetMasterVolume(float volume) { m_masterVolume = volume; }
        void SetMusicVolume(float volume) { m_musicVolume = volume; }
        void SetSFXVolume(float volume) { m_sfxVolume = volume; }
        void SetVolumeSettings(float master, float music, float sfx)
        {
            m_masterVolume = master;
            m_musicVolume = music;
            m_sfxVolume = sfx;
        }

        int GetCheckpointLevel() const { return m_checkpointLevel; }
        void SetCheckpointLevel(int level);

        const std::string& GetFilePath() const { return m_filepath; }

    private:
        std::string m_filepath = "block_jumper_save.json";
        float m_bestDistance = 0.0f;
        float m_masterVolume = 1.0f;
        float m_musicVolume = 0.5f;
        float m_sfxVolume = 0.8f;
        int m_checkpointLevel = 1;
    };
} // namespace BlockJumper
