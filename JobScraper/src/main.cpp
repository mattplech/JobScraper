// std
#include <memory>

// Job Scraper
#include "jobscraper/JobScraper.hpp"

int main(int argc, char** argv) {
    std::unique_ptr<jobscraper::JobScraper> jobScraper = std::make_unique<jobscraper::JobScraper>();
    
    return 0;
}