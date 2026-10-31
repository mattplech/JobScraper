#pragma once

// std
#include <string>
#include <vector>
#include <filesystem>

// Job Scraper
#include "jobscraper/Adapters/Adapter.hpp"

namespace jobscraper
{
    struct SiteConfig
    {
        std::string name;
        bool enabled = true;
        SearchFilters filters;
    };

    struct JobscraperConfig
    {
        std::vector<SiteConfig> sites;

        static JobscraperConfig Load(const std::filesystem::path &path);
    };
} // namespace jobscraper