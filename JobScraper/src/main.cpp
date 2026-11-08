// std
#include <memory>

// Job Scraper
#include "jobscraper/Log/Log.hpp"
#include "jobscraper/JobScraper.hpp"

int main(int argc, char** argv)
{
    JOBSCRAPER_LOGGER_INIT();

    try
    {
        std::unique_ptr<jobscraper::JobScraper> job_scraper = std::make_unique<jobscraper::JobScraper>();

        job_scraper->Run();
    } catch (std::exception const &e)
    {
        JOBSCRAPER_CRITICAL(e.what());

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}