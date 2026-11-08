#include "jobscraper/JobScraper.hpp"

// std
#include <cstdlib>
#include <unordered_map>

// Job Scraper
#include "jobscraper/Log/Log.hpp"
#include "jobscraper/Config.hpp"
#include "jobscraper/Adapters/AlmaAdapter.hpp"
#include "jobscraper/Adapters/JobsFetcher.hpp"
#include "jobscraper/Adapters/PraceFetcher.hpp"

namespace jobscraper
{
    JobScraper::JobScraper()
    {
        JOBSCRAPER_INFO("Initializing JobScraper...");

        DatabaseConfig config;
        config.host = std::getenv("JOBSCRAPER_HOST") ? std::getenv("JOBSCRAPER_HOST") : config.host;
        try
        {
            config.port = getenv("JOBSCRAPER_PORT") ? std::stoi(std::getenv("JOBSCRAPER_PORT")) : config.port;
        }
        catch(const std::exception &e)
        {
            JOBSCRAPER_ERROR("Error occurred while parsing JOBSCRAPER_PORT: {}", e.what());
            JOBSCRAPER_WARN("Using default JOBSCRAPER_PORT: {}", config.port);
        }
        config.db_name = std::getenv("JOBSCRAPER_NAME") ? std::getenv("JOBSCRAPER_NAME") : config.db_name;
        config.user = std::getenv("JOBSCRAPER_USER") ? std::getenv("JOBSCRAPER_USER") : config.user;
        config.password = std::getenv("JOBSCRAPER_PASSWORD") ? std::getenv("JOBSCRAPER_PASSWORD") : config.password;
        config.password_file = std::getenv("JOBSCRAPER_PASSWORD_FILE") ? std::getenv("JOBSCRAPER_PASSWORD_FILE") : config.password_file;
        config.ssl_mode = std::getenv("JOBSCRAPER_SSL_MODE") ? std::getenv("JOBSCRAPER_SSL_MODE") : config.ssl_mode;

        database_ = std::make_unique<Database>(config);

        JOBSCRAPER_INFO("JobScraper initialized.");
    }

    void JobScraper::Run()
    {
        const char *config_path = std::getenv("JOBSCRAPER_CONFIG");
        const JobscraperConfig config = JobscraperConfig::Load(config_path ? config_path : "config.json");

        std::unordered_map<std::string, std::unique_ptr<Adapter>> adapters;
        adapters.emplace("jobs.cz", std::make_unique<AlmaAdapter>(new JobsFetcher()));
        adapters.emplace("prace.cz", std::make_unique<AlmaAdapter>(new PraceFetcher()));

        for (const SiteConfig &site : config.sites)
        {
            if (!site.enabled)
                continue;

            const auto adapter = adapters.find(site.name);
            if (adapter == adapters.end())
            {
                JOBSCRAPER_WARN("No adapter registered for site \"{}\".", site.name);

                continue;
            }

            try
            {
                adapter->second->Run(database_.get(), site.filters);
            }
            catch (const std::exception &error)
            {
                JOBSCRAPER_ERROR("Scraping \"{}\" failed: {}", site.name, error.what());
            }
        }
    }
} // namespace jobscraper