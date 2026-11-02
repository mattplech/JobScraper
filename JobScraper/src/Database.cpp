#include <jobscraper/Database.hpp>

// std
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <iterator>

// Job Scraper
#include "jobscraper/Log/Log.hpp"

namespace jobscraper
{
    Database::Database(const DatabaseConfig &config)
    {
        if (config.port < 1 || config.port > 65535)
            throw std::invalid_argument("Invalid DB_PORT");

        if (config.schema.has_value() && config.schema->empty())
            throw std::invalid_argument("DB_SCHEMA must not be empty");

        JOBSCRAPER_INFO("Connecting to database...");

        std::map<std::string, std::string> options = {
            {"host", config.host},
            {"port", std::to_string(config.port)},
            {"dbname", config.db_name},
            {"user", config.user},
            {"sslmode", config.ssl_mode}
        };

        if (!config.password.empty())
        {
            options.emplace("password", config.password);
        } else if (!config.password_file.empty())
        {
            if (!std::filesystem::exists(config.password_file))
                throw std::runtime_error("DB_PASSWORD_FILE does not exist: " + config.password_file);

            std::ifstream password_file(config.password_file);
            if (!password_file.is_open())
                throw std::runtime_error("Failed to open DB_PASSWORD_FILE: " + config.password_file);

            std::string password;
            if (!std::getline(password_file, password))
                throw std::runtime_error("Failed to read DB_PASSWORD_FILE: " + config.password_file);

            if (password.empty())
                throw std::runtime_error("DB_PASSWORD_FILE is empty: " + config.password_file);

            options.emplace("password", password);
        } else
        {
            throw std::invalid_argument("PostgreSQL password or password file must be provided");
        }

        // Create the connection
        connection_ = std::make_unique<pqxx::connection>(options);

        if (config.schema.has_value())
        {
            pqxx::work schema_txn(*connection_);
            schema_txn.exec("SET search_path TO " + schema_txn.quote_name(config.schema.value()));
            schema_txn.commit();
        }

        JOBSCRAPER_INFO("Database connected.");

        Init();
    }

    Database::~Database()
    {
        connection_->close();
        
        JOBSCRAPER_INFO("Database disconnected.");
    }

    int Database::StoreOffer(const JobOffer &jobOffer)
    {
        if (!IsConnected())
            throw std::runtime_error("Database is not connected");

        pqxx::work txn(*connection_);

        const pqxx::result result;
        try
        {
            const pqxx::result result = txn.exec(R"(
                INSERT INTO job_offers (
                    name,
                    job_description,
                    company_name,
                    company_address,

                    job_url,

                    salary,

                    created_at
                )
                VALUES ($1, $2, $3, $4, $5, $6, $7)
                RETURNING id
            )", pqxx::params{
                    jobOffer.title,
                    jobOffer.job_description,
                    jobOffer.company_name,
                    jobOffer.company_address,
                    jobOffer.job_url,
                    jobOffer.salary,
                    jobOffer.created_at
                }
            );

            txn.commit();

            return result[0]["id"].as<int>();
        }
        catch(const std::exception& e)
        {
            JOBSCRAPER_ERROR(e.what());
        }

        return -1;
    }

    void Database::StoreOffers(const std::vector<JobOffer> &jobOffers)
    {
        if (!IsConnected())
            throw std::runtime_error("Database is not connected");

        pqxx::work txn(*connection_);

        for (const auto &job_offer : jobOffers)
        {
            try
            {
                const pqxx::result result = txn.exec(R"(
                    INSERT INTO job_offers (
                        name,
                        job_description,
                        company_name,
                        company_address,

                        job_url,

                        salary,

                        created_at
                    )
                    VALUES ($1, $2, $3, $4, $5, $6, $7)
                    RETURNING id
                )", pqxx::params{
                        job_offer.title,
                        job_offer.job_description,
                        job_offer.company_name,
                        job_offer.company_address,
                        job_offer.job_url,
                        job_offer.salary,
                        job_offer.created_at
                    }
                );
            }
            catch(const std::exception& e)
            {
                JOBSCRAPER_ERROR(e.what());
            }
        }

        txn.commit();
    }   

    std::optional<StoredJobOffer> Database::GetOffer(const int id) const
    {
        if (!IsConnected())
            throw std::runtime_error("Database is not connected");

        pqxx::read_transaction txn(*connection_);

        const pqxx::result result = txn.exec(R"(
            SELECT 
                id,
                name,
                job_description,
                company_name,
                company_address,

                job_url,

                salary,

                notes,

                created_at,
                registered_at

            FROM job_offers
            WHERE id = $1
        )", pqxx::params{id});

        if (result.empty())
            return std::nullopt;

        const pqxx::row_ref &row = result[0];

        // Populate the StoredJobOffer fields
        StoredJobOffer stored_offer;
        stored_offer.id = id;
        stored_offer.registered_at = row["registered_at"].as<std::string>();

        if (!row["notes"].is_null()) {
            stored_offer.notes = row["notes"].as<std::string>();
        }

        // Populate the JobOffer fields
        JobOffer &job_offer = stored_offer.job_offer;
        job_offer.title = row["name"].as<std::string>();
        job_offer.job_description = row["job_description"].as<std::string>();
        job_offer.company_name = row["company_name"].as<std::string>();

        if (!row["company_address"].is_null()) {
            job_offer.company_address = row["company_address"].as<std::string>();
        }

        job_offer.job_url = row["job_url"].as<std::string>();

        if (!row["salary"].is_null()) {
            job_offer.salary = row["salary"].as<std::string>();
        }

        if (!row["created_at"].is_null()) {
            job_offer.created_at = row["created_at"].as<std::string>();
        }

        return stored_offer;
    }

    std::vector<StoredJobOffer> Database::GetOffers() const
    {
        if (!IsConnected())
            throw std::runtime_error("Database is not connected");

        pqxx::read_transaction txn(*connection_);

        const pqxx::result result = txn.exec(R"(
            SELECT 
                id,
                name,
                job_description,
                company_name,
                company_address,

                job_url,

                salary,

                notes,

                created_at,
                registered_at

            FROM job_offers
            ORDER BY registered_at DESC
        )");

        std::vector<StoredJobOffer> offers;
        offers.reserve(result.size());

        for (const auto &row : result)
        {
            // Populate the StoredJobOffer fields
            StoredJobOffer stored_offer;
            stored_offer.id = row["id"].as<int>();
            stored_offer.registered_at = row["registered_at"].as<std::string>();

            if (!row["notes"].is_null()) {
                stored_offer.notes = row["notes"].as<std::string>();
            }

            // Populate the JobOffer fields
            JobOffer &job_offer = stored_offer.job_offer;
            job_offer.title = row["name"].as<std::string>();
            job_offer.job_description = row["job_description"].as<std::string>();
            job_offer.company_name = row["company_name"].as<std::string>();

            if (!row["company_address"].is_null()) {
                job_offer.company_address = row["company_address"].as<std::string>();
            }

            job_offer.job_url = row["job_url"].as<std::string>();

            if (!row["salary"].is_null()) {
                job_offer.salary = row["salary"].as<std::string>();
            }

            if (!row["created_at"].is_null()) {
                job_offer.created_at = row["created_at"].as<std::string>();
            }

            offers.push_back(std::move(stored_offer));
        }

        return offers;
    }

    bool Database::DeleteOffer(const int id)
    {
        if (!IsConnected())
            throw std::runtime_error("Database is not connected");

        pqxx::work txn(*connection_);

        const pqxx::result result = txn.exec(R"(
            DELETE FROM job_offers
            WHERE id = $1
        )", pqxx::params{id});

        txn.commit();

        return result.affected_rows() > 0;
    }

    bool Database::HasOffer(const int id) const
    {
        if (!IsConnected())
            throw std::runtime_error("Database is not connected");

        pqxx::read_transaction txn(*connection_);

        const pqxx::result result = txn.exec(R"(
            SELECT 1
            FROM job_offers
            WHERE id = $1
        )", pqxx::params{id});

        return !result.empty();
    }

    bool Database::HasOffer(const std::string &jobUrl) const
    {
        if (!IsConnected())
            throw std::runtime_error("Database is not connected");

        pqxx::read_transaction txn(*connection_);

        const pqxx::result result = txn.exec(R"(
            SELECT 1
            FROM job_offers
            WHERE job_url = $1
            LIMIT 1
        )", pqxx::params{jobUrl});

        return !result.empty();
    }

    bool Database::IsConnected() const noexcept
    {
        return connection_->is_open();
    }

    void Database::Init()
    {
        pqxx::work txn(*connection_);

        // Create the job_offers table if it doesn't exist
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS job_offers(
                id INT PRIMARY KEY GENERATED ALWAYS AS IDENTITY,

                name TEXT NOT NULL,
                job_description TEXT NOT NULL,
                company_name TEXT NOT NULL,
                company_address TEXT,

                job_url TEXT NOT NULL,

                salary TEXT,

                notes TEXT,

                created_at TIMESTAMPTZ,
                registered_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
            )
        )");

        // Create an index on the registered_at column for faster queries
        txn.exec(R"(
            CREATE INDEX IF NOT EXISTS
            idx_job_offers_registered_at
            ON job_offers (registered_at DESC)
        )");

        txn.exec(R"(
            CREATE INDEX IF NOT EXISTS
            idx_job_offers_job_url
            ON job_offers (job_url)
        )");

        txn.commit();

        JOBSCRAPER_INFO("Database initialized.");
    }
} // namespace jobscraper