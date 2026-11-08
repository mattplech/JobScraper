// std
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <pqxx/pqxx>
#include <stdexcept>
#include <string>
#include <vector>

// Catch2
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

// Job Scraper
#include <jobscraper/Database.hpp>
#include <jobscraper/Log/Log.hpp>

namespace
{
    using jobscraper::Database;
    using jobscraper::DatabaseConfig;
    using jobscraper::JobOffer;
    using jobscraper::StoredJobOffer;

    inline std::string EnvironmentValue(const char *name, const std::string &fallback)
    {
        const char *value = std::getenv(name);

        return value ? value : fallback;
    }

    DatabaseConfig LoadDatabaseConfig()
    {
        DatabaseConfig config;
        config.host = EnvironmentValue("JOBSCRAPER_HOST", config.host);
        try
        {
            const std::string port = EnvironmentValue("JOBSCRAPER_PORT", std::to_string(config.port));
            config.port = std::stoi(port);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Error occurred while parsing JOBSCRAPER_PORT for Database integration tests: " +
                std::string(e.what()));
        }
        config.db_name = EnvironmentValue("JOBSCRAPER_NAME", config.db_name);
        config.user = EnvironmentValue("JOBSCRAPER_USER", config.user);
        config.password = EnvironmentValue("JOBSCRAPER_PASSWORD", config.password);
        config.password_file = EnvironmentValue("JOBSCRAPER_PASSWORD_FILE", config.password_file);
        config.ssl_mode = EnvironmentValue("JOBSCRAPER_SSL_MODE", config.ssl_mode);

        return config;
    }

    std::map<std::string, std::string> ConnectionOptions(const DatabaseConfig &config)
    {
        std::map<std::string, std::string> options{
            {"host", config.host},
            {"port", std::to_string(config.port)},
            {"dbname", config.db_name},
            {"user", config.user},
            {"sslmode", config.ssl_mode}
        };

        if (!config.password.empty())
        {
            options.emplace("password", config.password);
        }
        else if (!config.password_file.empty())
        {
            if (!std::filesystem::exists(config.password_file))
                throw std::runtime_error("JOBSCRAPER_PASSWORD_FILE does not exist: " + config.password_file);

            std::ifstream password_file(config.password_file);
            if (!password_file.is_open())
                throw std::runtime_error("Failed to open JOBSCRAPER_PASSWORD_FILE: " + config.password_file);

            std::string password;
            if (!std::getline(password_file, password))
                throw std::runtime_error("Failed to read JOBSCRAPER_PASSWORD_FILE: " + config.password_file);

            if (password.empty())
                throw std::runtime_error("JOBSCRAPER_PASSWORD_FILE is empty: " + config.password_file);

            options.emplace("password", password);
        }
        else
        {
            throw std::runtime_error(
                "Database integration tests require JOBSCRAPER_PASSWORD or JOBSCRAPER_PASSWORD_FILE");
        }

        return options;
    }

    class TestSchema
    {
    public:
        explicit TestSchema(const DatabaseConfig &config)
            : connection_(ConnectionOptions(config)), name_(NextName(connection_))
        {
            pqxx::work transaction(connection_);
            transaction.exec("CREATE SCHEMA " + transaction.quote_name(name_));
            transaction.commit();
        }

        ~TestSchema()
        {
            try
            {
                pqxx::work transaction(connection_);
                transaction.exec("DROP SCHEMA IF EXISTS " + transaction.quote_name(name_) + " CASCADE");
                transaction.commit();
            }
            catch (...)
            {
            }
        }

        TestSchema(const TestSchema &) = delete;
        TestSchema &operator=(const TestSchema &) = delete;

        const std::string &Name() const noexcept
        {
            return name_;
        }

    private:
        static std::string NextName(pqxx::connection &connection)
        {
            pqxx::read_transaction transaction(connection);
            const pqxx::result result = transaction.exec("SELECT gen_random_uuid()::text");
            transaction.commit();

            return "test_" + result[0][0].as<std::string>();
        }

        pqxx::connection connection_;
        std::string name_;
    };

    struct DatabaseFixture
    {
        DatabaseFixture()
            : config(LoadDatabaseConfig()), schema(config)
        {
            config.schema = schema.Name();
            database = std::make_unique<Database>(config);
        }

        DatabaseConfig config;
        TestSchema schema;
        std::unique_ptr<Database> database;
    };

    JobOffer MakeOffer(const std::string &url)
    {
        JobOffer offer;
        offer.title = "Database integration test";
        offer.job_description = "A deterministic test offer";
        offer.company_name = "Example Company";
        offer.company_address = "1 Test Street";
        offer.job_url = url;
        offer.salary = "1000";
        offer.created_at = "2025-01-15";

        return offer;
    }

    bool ContainsOffer(const std::vector<StoredJobOffer> &offers, const std::string &url)
    {
        for (const StoredJobOffer &offer : offers)
            if (offer.job_offer.job_url == url)
                return true;

        return false;
    }
}

int main(int argc, char *argv[])
{
    JOBSCRAPER_LOGGER_INIT();

    return Catch::Session().run(argc, argv);
}

TEST_CASE_METHOD(DatabaseFixture, "Database connects and initializes its schema", "[database]")
{
    REQUIRE(database->IsConnected());

    const auto offers = database->GetOffers();
    REQUIRE(offers.empty());
}

TEST_CASE_METHOD(DatabaseFixture, "StoreOffer round-trips offer fields", "[database]")
{
    const JobOffer offer = MakeOffer("https://example.test/jobs/round-trip");

    const int id = database->StoreOffer(offer);

    REQUIRE(id > 0);
    REQUIRE(database->HasOffer(id));
    REQUIRE(database->HasOffer(offer.job_url));

    const auto stored = database->GetOffer(id);
    REQUIRE(stored.has_value());
    
    CHECK(stored->id == id);
    CHECK(stored->job_offer.title == offer.title);
    CHECK(stored->job_offer.job_description == offer.job_description);
    CHECK(stored->job_offer.company_name == offer.company_name);
    CHECK(stored->job_offer.company_address == offer.company_address);
    CHECK(stored->job_offer.job_url == offer.job_url);
    CHECK(stored->job_offer.salary == offer.salary);
    REQUIRE(stored->job_offer.created_at.has_value());
    CHECK(stored->job_offer.created_at->starts_with(offer.created_at.value()));
    CHECK_FALSE(stored->registered_at.empty());
    CHECK_FALSE(stored->notes.has_value());
}

TEST_CASE_METHOD(DatabaseFixture, "Optional offer fields round-trip as SQL NULL", "[database]")
{
    JobOffer offer = MakeOffer("https://example.test/jobs/nullable");
    offer.company_address.reset();
    offer.salary.reset();
    offer.created_at.reset();

    const int id = database->StoreOffer(offer);
    const auto stored = database->GetOffer(id);

    REQUIRE(stored.has_value());
    CHECK_FALSE(stored->job_offer.company_address.has_value());
    CHECK_FALSE(stored->job_offer.salary.has_value());
    CHECK_FALSE(stored->job_offer.created_at.has_value());
}

TEST_CASE_METHOD(DatabaseFixture, "GetOffer and HasOffer handle missing records", "[database]")
{
    CHECK_FALSE(database->GetOffer(-1).has_value());
    CHECK_FALSE(database->HasOffer(-1));
    CHECK_FALSE(database->HasOffer("https://example.test/jobs/missing"));
}

TEST_CASE_METHOD(DatabaseFixture, "StoreOffers stores each offer in the batch", "[database]")
{
    const std::vector<JobOffer> offers{
        MakeOffer("https://example.test/jobs/batch-one"),
        MakeOffer("https://example.test/jobs/batch-two"),
        MakeOffer("https://example.test/jobs/batch-three")
    };

    database->StoreOffers(offers);

    const auto stored = database->GetOffers();
    REQUIRE(stored.size() == offers.size());
    CHECK(ContainsOffer(stored, offers[0].job_url));
    CHECK(ContainsOffer(stored, offers[1].job_url));
    CHECK(ContainsOffer(stored, offers[2].job_url));
}

TEST_CASE_METHOD(DatabaseFixture, "DeleteOffer reports whether a row was deleted", "[database]")
{
    const int id = database->StoreOffer(MakeOffer("https://example.test/jobs/delete"));

    CHECK(database->DeleteOffer(id));
    CHECK_FALSE(database->DeleteOffer(id));
    CHECK_FALSE(database->GetOffer(id).has_value());
}

TEST_CASE_METHOD(DatabaseFixture, "Database instances are isolated by schema", "[database]")
{
    TestSchema other_schema(config);
    DatabaseConfig other_config = config;
    other_config.schema = other_schema.Name();
    Database other_database(other_config);

    const std::string first_url = "https://example.test/jobs/schema-one";
    const std::string second_url = "https://example.test/jobs/schema-two";
    
    database->StoreOffer(MakeOffer(first_url));

    CHECK(database->HasOffer(first_url));
    CHECK_FALSE(other_database.HasOffer(first_url));

    other_database.StoreOffer(MakeOffer(second_url));
    CHECK(other_database.HasOffer(second_url));
    CHECK_FALSE(database->HasOffer(second_url));
}

TEST_CASE("Database rejects invalid port and empty schema", "[database][validation]")
{
    DatabaseConfig config = LoadDatabaseConfig();
    config.port = 0;
    CHECK_THROWS_AS(Database(config), std::invalid_argument);
    config.port = 65536;
    CHECK_THROWS_AS(Database(config), std::invalid_argument);

    config.port = 5432;
    config.schema = "";
    CHECK_THROWS_AS(Database(config), std::invalid_argument);
}

TEST_CASE("Database rejects missing password configuration", "[database][validation]")
{
    DatabaseConfig config = LoadDatabaseConfig();
    config.password.clear();
    config.password_file.clear();

    CHECK_THROWS_AS(Database(config), std::invalid_argument);
}

TEST_CASE("Database rejects missing password files", "[database][validation]")
{
    DatabaseConfig config = LoadDatabaseConfig();
    config.password.clear();
    config.password_file = (std::filesystem::temp_directory_path() /
        "jobscraper-missing-test-password").string();
    while (std::filesystem::exists(config.password_file))
        config.password_file += "_";

    CHECK_THROWS_AS(Database(config), std::runtime_error);
}

TEST_CASE("Database rejects empty password files", "[database][validation]")
{
    DatabaseConfig config = LoadDatabaseConfig();
    config.password.clear();
    config.password_file = (std::filesystem::temp_directory_path() /
        ("jobscraper-empty-test-password-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))).string();

    {
        std::ofstream password_file(config.password_file);
        REQUIRE(password_file.good());
    }

    CHECK_THROWS_AS(Database(config), std::runtime_error);
    std::filesystem::remove(config.password_file);
}