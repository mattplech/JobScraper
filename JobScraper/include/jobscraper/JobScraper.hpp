#pragma once

// std
#include <memory>

// Job Scraper
#include "jobscraper/Database.hpp"

namespace jobscraper
{
    class JobScraper
    {
    public:
        JobScraper();

        JobScraper(const Database &) = delete;
        JobScraper(const JobScraper &&) noexcept = delete;
        JobScraper& operator=(const JobScraper &) = delete;
        JobScraper& operator=(const JobScraper &&) noexcept = delete;

        void Run();
    
    private:
        std::unique_ptr<Database> database_;
    };
    
} // namespace jobscraper