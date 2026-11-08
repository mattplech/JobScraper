#pragma once

// std
#include <string>
#include <vector>

// Job Scraper
#include "jobscraper/Database.hpp"

namespace jobscraper
{
    struct SearchFilters
    {
        std::vector<std::string> keywords;
        std::vector<std::string> location_ids;
        std::string work_location_type;
        int max_pages = 5;
        int max_age_days = 30;
    };

    class Adapter
    {
    public:
        Adapter() = default;
        virtual ~Adapter() = default;

        Adapter(const Adapter &) = delete;
        Adapter(const Adapter &&) noexcept = delete;
        Adapter& operator=(const Adapter &) = delete;
        Adapter& operator=(const Adapter &&) noexcept = delete;

        virtual std::string Name() const = 0;
        virtual void Scrape(const SearchFilters &filters, std::vector<JobOffer> &offers) = 0;

        void Run(Database *database, const SearchFilters &filters);
    };
} // namespace jobscraper