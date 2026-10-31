#pragma once

// Job Scraper
#include "jobscraper/Adapters/Adapter.hpp"

namespace jobscraper
{
    class JobsAdapter final : public Adapter
    {
    public:
        std::string Name() const override;
        void Scrape(const SearchFilters &filters, std::vector<JobOffer> &offers) override;
    };
} // namespace jobscraper