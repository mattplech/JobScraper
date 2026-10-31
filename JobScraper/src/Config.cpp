#include "jobscraper/Config.hpp"

// std
#include <fstream>
#include <stdexcept>

// nlohmann-json
#include <nlohmann/json.hpp>

// Job Scraper
#include "jobscraper/Log/Log.hpp"

namespace jobscraper
{
    JobscraperConfig JobscraperConfig::Load(const std::filesystem::path &path)
    {
        if (!std::filesystem::exists(path))
            throw std::runtime_error("Jobscraper config file does not exist: " + path.string());

        std::ifstream file(path);
        if (!file)
            throw std::runtime_error("Could not open jobscraper config: " + path.string());

        const nlohmann::json root = nlohmann::json::parse(file);
        if (!root.contains("sites") || !root["sites"].is_array())
            throw std::runtime_error("Jobscraper config must contain a sites array");

        JobscraperConfig config;
        for (const auto &site : root["sites"])
        {
            if (!site.contains("name") || !site["name"].is_string())
            {
                JOBSCRAPER_WARN("Skipping site config without a name: {}", site.dump());

                continue;
            }

            SiteConfig site_config;
            site_config.name = site.at("name").get<std::string>();
            site_config.enabled = site.value("enabled", true);

            const auto filters = site.value("filters", nlohmann::json::object());
            site_config.filters.keywords = filters.value("keywords", std::vector<std::string>{});
            site_config.filters.location_ids = filters.value("location_ids", std::vector<std::string>{});
            site_config.filters.work_location_type = filters.value("work_location_type", std::string{});
            site_config.filters.max_pages = filters.value("max_pages", int{5});
            site_config.filters.max_age_days = filters.value("max_age_days", int{30});

            if (site_config.filters.max_pages <= 0)
            {
                JOBSCRAPER_WARN("Skipping site config with invalid max_pages: {}", site_config.filters.max_pages);

                continue;
            }

            if (site_config.filters.max_age_days <= 0)
            {
                JOBSCRAPER_WARN("Skipping site config with invalid max_age_days: {}", site_config.filters.max_age_days);

                continue;
            }

            config.sites.push_back(std::move(site_config));
        }

        return config;
    }
} // namespace jobscraper