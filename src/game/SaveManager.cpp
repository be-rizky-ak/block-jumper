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
        std::ifstream file(m_filepath);
        if (!file.is_open())
        {
            // Missing file is an expected initial condition: start fresh with 0.0f
            m_bestDistance = 0.0f;
            return false;
        }

        try
        {
            nlohmann::json j;
            file >> j;

            if (j.contains("best_distance"))
            {
                if (j["best_distance"].is_number())
                {
                    float dist = j["best_distance"].get<float>();
                    if (std::isfinite(dist) && dist >= 0.0f)
                    {
                        m_bestDistance = dist;
                        return true;
                    }
                }
            }

            // Invalid or non-positive value
            std::cerr << "[SaveManager] Warning: Invalid or corrupted value in " << m_filepath
                      << ". Resetting best distance to 0.0." << std::endl;
            m_bestDistance = 0.0f;
            return false;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[SaveManager] Warning: Failed to parse save file " << m_filepath
                      << " (" << e.what() << "). Safely resetting to 0.0." << std::endl;
            m_bestDistance = 0.0f;
            return false;
        }
    }

    bool SaveManager::Save()
    {
        try
        {
            nlohmann::json j;
            j["best_distance"] = m_bestDistance;

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
