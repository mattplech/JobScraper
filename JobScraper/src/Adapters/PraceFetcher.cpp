#include "jobscraper/Adapters/PraceFetcher.hpp"

// CPR
#include <cpr/cpr.h>

namespace jobscraper
{
    namespace
    {
        std::string EncodeQueryValue(const std::string &value)
        {
            static constexpr char hex[] = "0123456789ABCDEF";
            std::string encoded;
            for (const unsigned char character : value)
            {
                // Keep URI-safe bytes and percent-encode everything else.
                if (std::isalnum(character) || character == '-' || character == '_' || character == '.' || character == '~')
                {
                    encoded.push_back(static_cast<char>(character));
                }
                else
                {
                    encoded.push_back('%');
                    encoded.push_back(hex[character >> 4]);
                    encoded.push_back(hex[character & 0x0F]);
                }
            }

            return encoded;
        }
    }

    std::string PraceFetcher::Name() const
    {
        return "jobs.cz";
    }

    std::string PraceFetcher::BuildSearchUrl(const SearchFilters &filters, int page)
    {
        std::string url = "https://prace.cz/hledat/?";

        for (const std::string &keyword : filters.keywords)
            url += "keywords%5B%5D=" + EncodeQueryValue(keyword) + "&";

        for (const std::string &location_id : filters.location_ids)
            url += "workLocationIds%5B%5D=" + EncodeQueryValue(location_id) + "&";

        if (filters.work_location_type == "ON_SITE" || 
            filters.work_location_type == "PARTIALLY_REMOTE" || 
            filters.work_location_type == "FULL_REMOTE")
        {
            url += "workLocationTypes%5B%5D=" + EncodeQueryValue(filters.work_location_type) + "&";
        }

        if (page > 1)
            url += "page=" + std::to_string(page);

        if (url.back() == '?')
        {
            url.pop_back();
        } else if (url.back() == '&')
        {
            url.pop_back();
        }

        return url;
    }

    std::string PraceFetcher::Fetch(const std::string &url)
    {
        // Follow redirects because /pd/ links often lead to a site's final detail-page URL.
        const cpr::Response response = cpr::Get(cpr::Url{url},
            cpr::Header{{"User-Agent", "JobScraper/1.0 (+https://prace.cz/)"}},
            cpr::Timeout{12000}, cpr::Redirect{true});
            
        if (response.error || response.status_code < 200 || response.status_code >= 400)
            throw std::runtime_error("HTTP request failed for " + url + ": " + 
                (response.error ? response.error.message : std::to_string(response.status_code)));

        return response.text;
    }
} // namespace jobscraper
