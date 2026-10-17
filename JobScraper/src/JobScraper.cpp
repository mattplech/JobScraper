#include "jobscraper/JobScraper.hpp"

// Job Scraper
#include "jobscraper/Log/Log.hpp"

namespace jobscraper
{
    JobScraper::JobScraper()
    {
        log::Log::Init();

        JOB_SCRAPER_INFO("JobScraper initialized.");
    }
} // namespace jobscraper