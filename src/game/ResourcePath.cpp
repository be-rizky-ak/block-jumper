#include "ResourcePath.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

#include "Aion/Core/FileSystem.h"

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif

namespace BlockJumper
{
    namespace
    {
        std::string s_AssetDirectory;
        std::string s_ExecutableDirectory;
        bool s_Initialized = false;

        std::string DiscoverExecutableDir(const char* argv0)
        {
            std::filesystem::path exePath;

#if defined(_WIN32)
            wchar_t buffer[MAX_PATH];
            DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
            if (len > 0 && len < MAX_PATH)
            {
                exePath = std::filesystem::path(buffer);
            }
#elif defined(__linux__)
            try
            {
                exePath = std::filesystem::canonical("/proc/self/exe");
            }
            catch (...)
            {
            }
#endif

            if (exePath.empty() && argv0 && argv0[0] != '\0')
            {
                try
                {
                    std::filesystem::path candidate(argv0);
                    if (candidate.is_absolute())
                    {
                        exePath = candidate;
                    }
                    else
                    {
                        exePath = std::filesystem::absolute(candidate);
                    }
                }
                catch (...)
                {
                }
            }

            if (!exePath.empty())
            {
                try
                {
                    if (std::filesystem::is_regular_file(exePath))
                    {
                        return Aion::FileSystem::NormalizePath(exePath.parent_path().string());
                    }
                }
                catch (...)
                {
                }
            }

            try
            {
                return Aion::FileSystem::NormalizePath(std::filesystem::current_path().string());
            }
            catch (...)
            {
                return ".";
            }
        }
    } // namespace

    void ResourcePath::Initialize(const std::string& explicitAssetDir, const char* argv0)
    {
        s_ExecutableDirectory = DiscoverExecutableDir(argv0);

        std::vector<std::pair<std::string, std::string>> candidates;

        // 1. Explicitly configured asset directory
        if (!explicitAssetDir.empty())
        {
            candidates.push_back({"Explicit configuration", explicitAssetDir});
        }

        // 2. Environment variable overrides
        const char* envCustom = std::getenv("BLOCK_JUMPER_ASSETS_DIR");
        if (envCustom && envCustom[0] != '\0')
        {
            candidates.push_back({"BLOCK_JUMPER_ASSETS_DIR environment variable", std::string(envCustom)});
        }

        const char* envAion = std::getenv("AION_ASSETS_DIR");
        if (envAion && envAion[0] != '\0')
        {
            candidates.push_back({"AION_ASSETS_DIR environment variable", std::string(envAion)});
        }

        // 3. Executable-relative packaged asset directory
        std::filesystem::path exeP(s_ExecutableDirectory);
        candidates.push_back({"Packaged assets relative to executable", (exeP / "assets").string()});
        candidates.push_back({"Parent assets relative to executable", (exeP / ".." / "assets").string()});

        // 4. Well-defined development repository layout
        candidates.push_back({"Workspace game layout", (exeP / ".." / ".." / "games" / "BlockJumper" / "assets").string()});
        candidates.push_back({"Workspace parent game layout", (exeP / ".." / "games" / "BlockJumper" / "assets").string()});
        candidates.push_back({"Deep workspace game layout", (exeP / ".." / ".." / ".." / "games" / "BlockJumper" / "assets").string()});

        // 5. Working directory fallback
        try
        {
            std::filesystem::path cwd = std::filesystem::current_path();
            candidates.push_back({"Working directory assets", (cwd / "assets").string()});
            candidates.push_back({"Working directory game assets", (cwd / "games" / "BlockJumper" / "assets").string()});
        }
        catch (...)
        {
        }

        std::string chosenSource;
        std::string resolvedDir;

        for (const auto& [desc, pathStr] : candidates)
        {
            try
            {
                if (!pathStr.empty() && std::filesystem::exists(pathStr) && std::filesystem::is_directory(pathStr))
                {
                    resolvedDir = Aion::FileSystem::NormalizePath(std::filesystem::canonical(pathStr).string());
                    chosenSource = desc;
                    break;
                }
            }
            catch (...)
            {
                // Continue searching candidates
            }
        }

        if (resolvedDir.empty())
        {
            resolvedDir = Aion::FileSystem::NormalizePath((exeP / "assets").string());
            std::cerr << "[ResourcePath] Warning: Could not locate a verified asset directory. Defaulting to: "
                      << resolvedDir << std::endl;
        }
        else
        {
            std::cout << "[ResourcePath] Asset directory discovered via " << chosenSource << ": "
                      << resolvedDir << std::endl;
        }

        s_AssetDirectory = resolvedDir;
        s_Initialized = true;

        // Configure Aion engine FileSystem
        Aion::FileSystem::Init();
        Aion::FileSystem::SetBaseDirectory(s_ExecutableDirectory);
        Aion::FileSystem::SetAssetDirectory(s_AssetDirectory);
    }

    std::string ResourcePath::GetAssetDirectory()
    {
        if (!s_Initialized)
        {
            Initialize();
        }
        return s_AssetDirectory;
    }

    std::string ResourcePath::GetExecutableDirectory()
    {
        if (!s_Initialized)
        {
            Initialize();
        }
        return s_ExecutableDirectory;
    }

    std::string ResourcePath::Resolve(const std::string& relativePath)
    {
        if (relativePath.empty())
        {
            return "";
        }

        if (!s_Initialized)
        {
            Initialize();
        }

        try
        {
            // If already an existing absolute path, return it directly
            if (std::filesystem::path(relativePath).is_absolute() && std::filesystem::exists(relativePath))
            {
                return Aion::FileSystem::NormalizePath(relativePath);
            }
        }
        catch (...)
        {
        }

        // Handle virtual scheme "assets://"
        const std::string virtualPrefix = "assets://";
        if (relativePath.rfind(virtualPrefix, 0) == 0)
        {
            std::string sub = relativePath.substr(virtualPrefix.length());
            std::filesystem::path direct = std::filesystem::path(s_AssetDirectory) / sub;
            if (std::filesystem::exists(direct))
            {
                return Aion::FileSystem::NormalizePath(direct.string());
            }
            return Aion::FileSystem::ResolvePath(relativePath);
        }

        // Strip leading "assets/" or "assets\" if present
        std::string stripped = relativePath;
        if (stripped.rfind("assets/", 0) == 0)
        {
            stripped = stripped.substr(7);
        }
        else if (stripped.rfind("assets\\", 0) == 0)
        {
            stripped = stripped.substr(7);
        }

        // Primary: resolved against s_AssetDirectory
        try
        {
            std::filesystem::path primary = std::filesystem::path(s_AssetDirectory) / stripped;
            if (std::filesystem::exists(primary))
            {
                return Aion::FileSystem::NormalizePath(primary.string());
            }
        }
        catch (...)
        {
        }

        // Secondary: check relative to executable directory
        try
        {
            std::filesystem::path exeRel = std::filesystem::path(s_ExecutableDirectory) / relativePath;
            if (std::filesystem::exists(exeRel))
            {
                return Aion::FileSystem::NormalizePath(exeRel.string());
            }
        }
        catch (...)
        {
        }

        // Tertiary: delegate to Aion FileSystem
        std::string engineResolved = Aion::FileSystem::ResolvePath(relativePath);
        if (!engineResolved.empty() && std::filesystem::exists(engineResolved))
        {
            return engineResolved;
        }

        // Best effort: asset directory + stripped
        return Aion::FileSystem::NormalizePath((std::filesystem::path(s_AssetDirectory) / stripped).string());
    }

    bool ResourcePath::Exists(const std::string& relativePath)
    {
        std::string resolved = Resolve(relativePath);
        if (resolved.empty())
        {
            return false;
        }
        try
        {
            return std::filesystem::exists(resolved);
        }
        catch (...)
        {
            return false;
        }
    }
} // namespace BlockJumper
