#pragma once

// std
#include <string>

// Job Scraper
#include "jobscraper/Adapters/FetchAdapter.hpp"
#include "jobscraper/Adapters/Adapter.hpp"

namespace jobscraper
{
    class JobsFetcher : public FetchAdapter
    {
    public:
        std::string Name() const override;

    private:
        std::string BuildSearchUrl(const SearchFilters &filters, int page) override;
        std::string Fetch(const std::string &url) override;
    };
} // namespace jobscraper