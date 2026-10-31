#include "jobscraper/JobScraper.hpp"

// std
#include <cstdlib>
#include <unordered_map>

// Job Scraper
#include "jobscraper/Log/Log.hpp"
#include "jobscraper/Config.hpp"
#include "jobscraper/Adapters/JobsAdapter.hpp"

namespace jobscraper
{
    JobScraper::JobScraper()
    {
        JOBSCRAPER_LOGGER_INIT();       
        
        JOBSCRAPER_INFO("Initializing JobScraper...");

        DatabaseConfig config;
        config.host = std::getenv("DB_HOST") ? std::getenv("DB_HOST") : config.host;
        try
        {
            config.port = getenv("DB_PORT") ? std::stoi(std::getenv("DB_PORT")) : config.port;
        }
        catch(const std::exception& e)
        {
            JOBSCRAPER_ERROR("Error occurred while parsing DB_PORT: {}", e.what());
            JOBSCRAPER_WARN("Using default DB_PORT: {}", config.port);
        }
        config.db_name = std::getenv("DB_NAME") ? std::getenv("DB_NAME") : config.db_name;
        config.user = std::getenv("DB_USER") ? std::getenv("DB_USER") : config.user;
        config.password = std::getenv("DB_PASSWORD") ? std::getenv("DB_PASSWORD") : config.password;
        config.password_file = std::getenv("DB_PASSWORD_FILE") ? std::getenv("DB_PASSWORD_FILE") : config.password_file;
        config.ssl_mode = std::getenv("DB_SSL_MODE") ? std::getenv("DB_SSL_MODE") : config.ssl_mode;

        database_ = std::make_unique<Database>(config);

        JOBSCRAPER_INFO("JobScraper initialized.");
    }

    void JobScraper::Run()
    {
        const char *config_path = std::getenv("JOBSCRAPER_CONFIG");
        const JobscraperConfig config = JobscraperConfig::Load(config_path ? config_path : "config.json");

        std::unordered_map<std::string, std::unique_ptr<Adapter>> adapters;
        adapters.emplace("jobs.cz", std::make_unique<JobsAdapter>());

        for (const SiteConfig &site : config.sites)
        {
            if (!site.enabled)
                continue;

            const auto adapter = adapters.find(site.name);
            if (adapter == adapters.end())
            {
                JOBSCRAPER_WARN("No adapter registered for site '{}'.", site.name);

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