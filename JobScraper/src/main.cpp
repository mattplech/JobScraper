// std
#include <memory>

// Job Scraper
#include "jobscraper/Log/Log.hpp"
#include "jobscraper/JobScraper.hpp"

int main(int argc, char** argv)
{
    try
    {
        std::unique_ptr<jobscraper::JobScraper> jobScraper = std::make_unique<jobscraper::JobScraper>();

        jobScraper->Run();
    } catch (std::exception const &e)
    {
        JOBSCRAPER_CRITICAL(e.what());

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}