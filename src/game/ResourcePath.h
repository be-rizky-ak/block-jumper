#pragma once

#include <string>

namespace BlockJumper
{
    /**
     * @brief Portable centralized resource discovery for Block Jumper.
     *
     * Resolves assets independently of the current working directory and
     * original repository checkout layout using a deterministic resolution order:
     *   1. Explicitly configured asset directory (CLI --assets or parameter)
     *   2. Environment-variable overrides (BLOCK_JUMPER_ASSETS_DIR, AION_ASSETS_DIR)
     *   3. Packaged asset directory relative to the executable (<exe_dir>/assets)
     *   4. Development layout relative to the executable
     *   5. Current working directory fallback
     */
    class ResourcePath
    {
    public:
        /**
         * @brief Initialize resource discovery and configure Aion::FileSystem asset directory.
         * @param explicitAssetDir Optional explicitly configured asset directory path.
         * @param argv0 Optional argv[0] passed from main() to assist in executable path discovery.
         */
        static void Initialize(const std::string& explicitAssetDir = "", const char* argv0 = nullptr);

        /**
         * @brief Returns the determined root asset directory.
         */
        static std::string GetAssetDirectory();

        /**
         * @brief Returns the determined executable directory.
         */
        static std::string GetExecutableDirectory();

        /**
         * @brief Resolves an asset path to an absolute, normalized filesystem path.
         * @param relativePath Relative asset path (e.g. "assets/models/...", "models/...", or "assets://...").
         * @return Absolute normalized path if found, or best-effort resolved path.
         */
        static std::string Resolve(const std::string& relativePath);

        /**
         * @brief Checks if a given asset path exists on disk.
         */
        static bool Exists(const std::string& relativePath);
    };

    /**
     * @brief Convenience helper function to resolve an asset path.
     */
    inline std::string ResolveAsset(const std::string& relativePath)
    {
        return ResourcePath::Resolve(relativePath);
    }
} // namespace BlockJumper
