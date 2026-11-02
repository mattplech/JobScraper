#include "jobscraper/Adapters/Adapter.hpp"

// std
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// Job Scraper
#include "jobscraper/Log/Log.hpp"

namespace jobscraper
{
    namespace
    {
        std::optional<std::chrono::sys_days> ParseDate(const std::string &date)
        {
            std::tm parsed{};
            std::istringstream input(date);
            input >> std::get_time(&parsed, "%Y-%m-%d");

            if (input.fail())
                return std::nullopt;

            const std::chrono::year_month_day day{
                std::chrono::year{parsed.tm_year + 1900},
                std::chrono::month{static_cast<unsigned>(parsed.tm_mon + 1)},
                std::chrono::day{static_cast<unsigned>(parsed.tm_mday)}
            };

            if (!day.ok())
                return std::nullopt;

            return std::chrono::sys_days{day};
        }

        bool IsOfferWithinMaxAge(const JobOffer &offer, const SearchFilters &filters)
        {
            if (filters.max_age_days == 0)
                return true;

            if (!offer.created_at.has_value())
                return true;

            const auto created = ParseDate(offer.created_at.value());
            if (!created.has_value())
                return false;

            const auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());

            return created.value() >= (today - std::chrono::days{filters.max_age_days});
        }
    }

    void Adapter::Run(Database *database, const SearchFilters &filters)
    {
        std::vector<JobOffer> offers;
        Scrape(filters, offers);

        auto offer_it = offers.begin();
        while (offer_it != offers.end())
        {
            if (!IsOfferWithinMaxAge(*offer_it, filters))
            {
                JOBSCRAPER_INFO("Offer '{}' is older than {} days, skipping.", offer_it->job_url, filters.max_age_days);

                offers.erase(offer_it);

                continue;
            }

            if (database->HasOffer(offer_it->job_url))
            {
                offers.erase(offer_it);

                JOBSCRAPER_INFO("Offer '{}' already exists in the database, skipping.", offer_it->job_url);

                continue;
            }

            offer_it++;
        }

        database->StoreOffers(offers);

        JOBSCRAPER_INFO("{} adapter stored {} new job offers.", Name(), offers.size());
    }
} // namespace jobscraper