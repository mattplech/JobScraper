#pragma once

// std
#include <string>

// Job Scraper
#include "jobscraper/Adapters/Adapter.hpp"

namespace jobscraper
{
    class FetchAdapter
    {
        friend class AlmaAdapter;
    public:
        FetchAdapter() = default;
        virtual ~FetchAdapter() = default;

        FetchAdapter(const FetchAdapter &) = delete;
        FetchAdapter(const FetchAdapter &&) noexcept = delete;
        FetchAdapter& operator=(const FetchAdapter &) = delete;
        FetchAdapter& operator=(const FetchAdapter &&) noexcept = delete;

        virtual std::string Name() const = 0;

    protected:
        virtual std::string BuildSearchUrl(const SearchFilters &filters, int page) = 0;
        virtual std::string Fetch(const std::string &url) = 0;
    };
} // namespace jobscraper