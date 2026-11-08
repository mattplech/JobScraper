#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include <jobscraper/Adapters/AlmaAdapter.hpp>
#include <jobscraper/Log/Log.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using jobscraper::AlmaAdapter;
    using jobscraper::FetchAdapter;
    using jobscraper::JobOffer;
    using jobscraper::SearchFilters;

    constexpr const char *SearchUrl = "https://example.test/search";
    constexpr const char *DetailUrlPrefix = "https://example.test/nabidka/fixture-job-";

    std::string ReadFixture(const char *name)
    {
        const std::filesystem::path fixture_path = std::filesystem::path(std::getenv("TEST_FIXTURE_DIR")) / name;

        std::ifstream input(fixture_path);
        if (!input)
            throw std::runtime_error("Failed to open fixture: " + fixture_path.string());

        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    void ReplaceAll(std::string &text, const std::string &marker, const std::string &value)
    {
        size_t position = 0;
        while ((position = text.find(marker, position)) != std::string::npos)
        {
            text.replace(position, marker.size(), value);
            position += value.size();
        }
    }

    class FixtureFetcher final : public FetchAdapter
    {
    public:
        FixtureFetcher(std::string listing, std::string detail)
            : listing_(std::move(listing)), detail_(std::move(detail))
        {
        }

        std::string Name() const override
        {
            return "AlmaAdapter Integration Test";
        }

        int SearchFetchCount() const
        {
            return search_fetch_count_;
        }

        int DetailFetchCount() const
        {
            return detail_fetch_count_;
        }

    protected:
        std::string BuildSearchUrl(const SearchFilters &, int page) override
        {
            return std::string(SearchUrl) + "?page=" + std::to_string(page);
        }

        std::string Fetch(const std::string &url) override
        {
            if (url.starts_with(SearchUrl))
            {
                ++search_fetch_count_;
                return listing_;
            }

            if (url.starts_with(DetailUrlPrefix))
            {
                ++detail_fetch_count_;
                return detail_;
            }

            throw std::runtime_error("Unexpected fixture URL: " + url);
        }

    private:
        std::string listing_;
        std::string detail_;
        int search_fetch_count_ = 0;
        int detail_fetch_count_ = 0;
    };
} // namespace

int main(int argc, char *argv[])
{
    JOBSCRAPER_LOGGER_INIT();

    return Catch::Session().run(argc, argv);
}

TEST_CASE("AlmaAdapter scrapes listing and detail fixtures", "[integration][adapter]")
{
    std::string detail = ReadFixture("alma_detail.html");
    ReplaceAll(detail, "{{JOB_TITLE}}", "Fixture Engineer");
    ReplaceAll(detail, "{{JOB_DESCRIPTION}}", "Fixture job description");
    ReplaceAll(detail, "{{COMPANY_NAME}}", "Fixture Employer");
    ReplaceAll(detail, "{{COMPANY_ADDRESS}}", "1 Fixture Road");
    ReplaceAll(detail, "{{DATE_POSTED}}", "2025-11-07");
    ReplaceAll(detail, "{{SALARY_MIN}}", "51000");
    ReplaceAll(detail, "{{SALARY_MAX}}", "71000");

    FixtureFetcher *fetcher = new FixtureFetcher(ReadFixture("alma_search.html"), std::move(detail));
    AlmaAdapter adapter(fetcher);

    SearchFilters filters;
    filters.max_pages = 1;
    std::vector<JobOffer> offers;
    adapter.Scrape(filters, offers);

    REQUIRE(offers.size() == 5);
    CHECK(fetcher->SearchFetchCount() == 1);
    CHECK(fetcher->DetailFetchCount() == 5);

    for (size_t index = 0; index < offers.size(); ++index)
    {
        JobOffer &offer = offers[index];
        const std::string suffix = std::to_string(index + 1);

        CHECK(offer.title == "Fixture Engineer");
        CHECK(offer.job_description == "Fixture job description");
        CHECK(offer.company_name == "Fixture Employer");
        REQUIRE(offer.company_address.has_value());
        CHECK(*offer.company_address == "1 Fixture Road, Fixture City");
        CHECK(offer.job_url == std::string(DetailUrlPrefix) + suffix);
        REQUIRE(offer.salary.has_value());
        CHECK(*offer.salary == "51000 - 71000");
        REQUIRE(offer.created_at.has_value());
        CHECK(*offer.created_at == "2025-11-07");
        CHECK(offer.IsFilled());
    }
}