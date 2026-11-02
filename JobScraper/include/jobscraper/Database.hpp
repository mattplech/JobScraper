#pragma once

// std
#include <string>
#include <optional>
#include <vector>
#include <memory>

// libpqxx
#include <pqxx/pqxx>

namespace jobscraper
{
    struct DatabaseConfig
    {
        std::string host = "localhost";
        int port = 5432;

        std::string db_name = "jobscraper";
        std::string user = "jobscraper";
        std::string password;
        std::string password_file;
        std::optional<std::string> schema;

        std::string ssl_mode = "prefer";
    };

    struct JobOffer
    {
        std::string title;
        std::string job_description;
        std::string company_name;
        std::optional<std::string> company_address;

        std::string job_url;

        std::optional<std::string> salary;

        std::optional<std::string> created_at; // When the job offer was created

        bool IsFilled()
        {
            return !title.empty() && !job_description.empty() &&
                !company_name.empty() && company_address.has_value() && 
                !job_url.empty() && salary.has_value() && created_at.has_value();
        }
    };

    struct StoredJobOffer
    {
        int id = 0;

        JobOffer job_offer;

        std::optional<std::string> notes; // Personal notes on the job offer

        std::string registered_at; // When the job offer was stored in database
    };

    class Database
    {
    public:
        Database(const DatabaseConfig &config);
        ~Database();

        Database(const Database &) = delete;
        Database(const Database &&) noexcept = delete;
        Database& operator=(const Database &) = delete;
        Database& operator=(const Database &&) noexcept = delete;

        int StoreOffer(const JobOffer &jobOffer);
        void StoreOffers(const std::vector<JobOffer> &jobOffers);

        std::optional<StoredJobOffer> GetOffer(const int id) const;
        std::vector<StoredJobOffer> GetOffers() const;

        bool DeleteOffer(const int id);

        bool HasOffer(const int id) const;
        bool HasOffer(const std::string &jobUrl) const;

        bool IsConnected() const noexcept;

    private:
        void Init();

        std::unique_ptr<pqxx::connection> connection_;
    };
} // namespace jobscraper
