#include "jobscraper/JobScraper.hpp"

// std
#include <cstdlib>

// Job Scraper
#include "jobscraper/Log/Log.hpp"

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
        // TODO: Create adapters
    }
} // namespace jobscraper