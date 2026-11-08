#pragma once

// std
#include <string>
#include <vector>
#include <memory>

// Job Scraper
#include "jobscraper/Adapters/Adapter.hpp"
#include "jobscraper/Adapters/FetchAdapter.hpp"

namespace jobscraper
{
    class AlmaAdapter final : public Adapter
    {
    public:
        AlmaAdapter(FetchAdapter *fetcher);

        std::string Name() const override;
        void Scrape(const SearchFilters &filters, std::vector<JobOffer> &offers) override;

    private:
        std::unique_ptr<FetchAdapter> fetcher_;
    };
} // namespace jobscraper