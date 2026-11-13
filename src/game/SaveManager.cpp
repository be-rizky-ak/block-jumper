#include "SaveManager.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace BlockJumper
{
    SaveManager::SaveManager()
        : m_filepath("block_jumper_save.json")
    {
        Load();
    }

    SaveManager::SaveManager(std::string filepath)
        : m_filepath(std::move(filepath))
    {
        Load();
    }

    bool SaveManager::Load()
    {
        // Safe defaults
        m_bestDistance = 0.0f;
        m_masterVolume = 1.0f;
        m_musicVolume = 0.5f;
        m_sfxVolume = 0.8f;
        m_checkpointLevel = 1;

        std::ifstream file(m_filepath);
        if (!file.is_open())
        {
            // Missing file is an expected initial condition: start fresh with defaults
            return false;
        }

        try
        {
            nlohmann::json j;
            file >> j;

            bool validDistance = false;
            if (j.contains("best_distance") && j["best_distance"].is_number())
            {
                float dist = j["best_distance"].get<float>();
                if (std::isfinite(dist) && dist >= 0.0f)
                {
                    m_bestDistance = dist;
                    validDistance = true;
                }
            }

            if (!validDistance && j.contains("best_distance"))
            {
                std::cerr << "[SaveManager] Warning: Invalid or corrupted best_distance in " << m_filepath
                          << ". Resetting best distance to 0.0." << std::endl;
            }

            // Volume configuration: support flat keys or nested "audio" object
            auto loadVol = [&j](const std::string& key, const std::string& nestedKey, float defaultVal) -> float {
                if (j.contains(key) && j[key].is_number())
                {
                    float v = j[key].get<float>();
                    if (std::isfinite(v))
                    {
                        return std::clamp(v, 0.0f, 1.0f);
                    }
                }
                if (j.contains("audio") && j["audio"].is_object() && j["audio"].contains(nestedKey) && j["audio"][nestedKey].is_number())
                {
                    float v = j["audio"][nestedKey].get<float>();
                    if (std::isfinite(v))
                    {
                        return std::clamp(v, 0.0f, 1.0f);
                    }
                }
                return defaultVal;
            };

            m_masterVolume = loadVol("master_volume", "master", 1.0f);
            m_musicVolume = loadVol("music_volume", "music", 0.5f);
            m_sfxVolume = loadVol("sfx_volume", "sfx", 0.8f);

            if (j.contains("checkpoint_level") && j["checkpoint_level"].is_number())
            {
                int lvl = j["checkpoint_level"].get<int>();
                if (lvl >= 1 && lvl <= 10)
                {
                    m_checkpointLevel = lvl;
                }
            }

            return validDistance;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[SaveManager] Warning: Failed to parse save file " << m_filepath
                      << " (" << e.what() << "). Safely resetting to defaults." << std::endl;
            m_bestDistance = 0.0f;
            m_masterVolume = 1.0f;
            m_musicVolume = 0.5f;
            m_sfxVolume = 0.8f;
            m_checkpointLevel = 1;
            return false;
        }
    }

    void SaveManager::SetCheckpointLevel(int level)
    {
        m_checkpointLevel = std::clamp(level, 1, 10);
    }

    bool SaveManager::Save()
    {
        try
        {
            nlohmann::json j = nlohmann::json::object();
            // Preserve existing unrelated fields if file already exists
            {
                std::ifstream existingFile(m_filepath);
                if (existingFile.is_open())
                {
                    try
                    {
                        existingFile >> j;
                    }
                    catch (...)
                    {
                        j = nlohmann::json::object();
                    }
                }
            }

            j["best_distance"] = m_bestDistance;
            j["master_volume"] = m_masterVolume;
            j["music_volume"] = m_musicVolume;
            j["sfx_volume"] = m_sfxVolume;
            j["checkpoint_level"] = m_checkpointLevel;

            std::ofstream file(m_filepath, std::ios::trunc);
            if (!file.is_open())
            {
                std::cerr << "[SaveManager] Warning: Unable to open save file for writing: "
                          << m_filepath << std::endl;
                return false;
            }

            file << j.dump(4) << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[SaveManager] Error: Exception during save: " << e.what() << std::endl;
            return false;
        }
    }

    bool SaveManager::TryUpdateBestDistance(float runDistance)
    {
        if (!std::isfinite(runDistance) || runDistance <= 0.0f)
        {
            return false;
        }

        // New record if current run strictly exceeds previous best
        if (runDistance > m_bestDistance + 0.01f)
        {
            m_bestDistance = runDistance;
            Save();
            return true;
        }

        return false;
    }
} // namespace BlockJumper
