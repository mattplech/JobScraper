#include "jobscraper/Adapters/AlmaAdapter.hpp"

// std
#include <array>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <functional>
#include <iomanip>
#include <memory>
#include <regex>
#include <set>
#include <stdexcept>
#include <sstream>
#include <string_view>

// LibXml2
#include <libxml/HTMLparser.h>
#include <libxml/uri.h>

// nlohmann-json
#include <nlohmann/json.hpp>

// Job Scraper
#include "jobscraper/Log/Log.hpp"

namespace jobscraper
{
    namespace
    {
        using Json = nlohmann::json;
        using Document = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;

        std::string NodeContent(const xmlNode *node)
        {
            if (!node)
                return {};

            xmlChar *raw = xmlNodeGetContent(const_cast<xmlNode *>(node));

            if (!raw)
                return {};

            std::string result(reinterpret_cast<const char *>(raw));
            xmlFree(raw);

            return result;
        }

        std::string GetProperty(const xmlNode *node, const char *name)
        {
            if (!node)
                return {};

            xmlChar *raw = xmlGetProp(const_cast<xmlNode *>(node),
                reinterpret_cast<const xmlChar *>(name));
            
            if (!raw)
                return {};

            std::string result(reinterpret_cast<const char *>(raw));
            xmlFree(raw);

            return result;
        }

        std::string Lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char character) { return static_cast<char>(std::tolower(character)); });

            return value;
        }

        Document ParseHtml(const std::string &html)
        {
            htmlDocPtr document = htmlReadMemory(html.data(),
                static_cast<int>(html.size()), nullptr, "UTF-8",
                HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | 
                HTML_PARSE_NOWARNING | HTML_PARSE_NONET);

            return Document(document, xmlFreeDoc);
        }

        std::string BuildAbsoluteUrl(const std::string &base, const std::string &reference)
        {
            // Listing links may be relative, so resolve each one against the page that contained it.
            xmlChar *resolved = xmlBuildURI(reinterpret_cast<const xmlChar *>(reference.c_str()),
                reinterpret_cast<const xmlChar *>(base.c_str()));
            
            if (!resolved)
                return {};

            std::string result(reinterpret_cast<const char *>(resolved));
            xmlFree(resolved);

            return result;
        }

        std::string BuildCanonicalUrl(const std::string &url)
        {
            xmlURIPtr parsed = xmlParseURI(url.c_str());
            if (!parsed)
                return url;

            xmlFree(parsed->query);
            xmlFree(parsed->fragment);
            parsed->query = nullptr;
            parsed->fragment = nullptr;

            xmlChar *serialized = xmlSaveUri(parsed);
            xmlFreeURI(parsed);

            if (!serialized)
                return url;

            std::string result(reinterpret_cast<const char *>(serialized));
            xmlFree(serialized);

            return result;
        }

        bool IsJobUrl(const std::string &url)
        {
            // Jobs.cz uses /nabidka/ for its own pages and /pd/ for employer-hosted Alma Career pages.
            return url.find("/nabidka/") != std::string::npos || url.find("/pd/") != std::string::npos;
        }

        xmlNode *FindHead(xmlNode *node)
        {
            for (xmlNode *current = node; current; current = current->next)
            {
                if (current->type == XML_ELEMENT_NODE &&
                    xmlStrcasecmp(current->name, BAD_CAST "head") == 0)
                {
                    return current;
                }

                xmlNode *head = FindHead(current->children);
                if (head)
                    return head;
            }

            return nullptr;
        }

        xmlNode *FindContentElement(xmlNode *node)
        {
            for (xmlNode *current = node; current; current = current->next)
            {
                if (current->type == XML_ELEMENT_NODE &&
                    (xmlStrcasecmp(current->name, BAD_CAST "main") == 0 ||
                    xmlStrcasecmp(current->name, BAD_CAST "article") == 0))
                {
                    return current;
                }

                if (xmlNode *match = FindContentElement(current->children))
                    return match;
            }

            return nullptr;
        }

        xmlNode *FindCardContainer(xmlNode *node)
        {
            for (xmlNode *current = node; current; current = current->next)
            {
                if (current->type == XML_ELEMENT_NODE && 
                    xmlStrcasecmp(current->name, BAD_CAST "div") == 0)
                {
                    const std::string test_id = GetProperty(current, "data-testid");
                    if (test_id.find("search-results-section-stack") != std::string::npos)
                        return current;
                }

                xmlNode *card_container = FindCardContainer(current->children);
                if (card_container)
                    return card_container;
            }

            return nullptr;
        }

        bool ExtractCardLink(xmlNode *node, const std::string &pageUrl, 
            std::string &jobUrl, std::string &title)
        {
            for (xmlNode *current = node; current; current = current->next)
            {
                if (current->type == XML_ELEMENT_NODE &&
                    xmlStrcasecmp(current->name, BAD_CAST "a") == 0)
                {
                    const std::string href = GetProperty(current, "href");
                    const std::string absolute_url = BuildAbsoluteUrl(pageUrl, href);
                    if (IsJobUrl(absolute_url))
                    {
                        jobUrl = absolute_url;
                        title = NodeContent(current);

                        return true;
                    }
                }

                if (ExtractCardLink(current->children, pageUrl, jobUrl, title))
                    return true;
            }

            return false;
        }

        void ExtractCompanyName(const std::string &text, std::string &companyName)
        {
            const size_t value_start = text.find("firmy:");
            if (value_start == std::string::npos)
                return;

            const size_t value_end = text.find("Typ", value_start);
            if (value_end == std::string::npos)
                return;

            companyName = text.substr(value_start + 6, value_end - value_start - 6);
        }

        std::string FindMeta(xmlNode *node, const std::set<std::string> &keys)
        {
            for (xmlNode *current = node; current; current = current->next)
            {
                if (current->type == XML_ELEMENT_NODE &&
                    xmlStrcasecmp(current->name, BAD_CAST "meta") == 0)
                {
                    const std::string key = Lower(GetProperty(current, "property").empty()
                        ? GetProperty(current, "name") : GetProperty(current, "property"));

                    if (keys.contains(key))
                        return GetProperty(current, "content");
                }

                const std::string nested = FindMeta(current->children, keys);
                if (!nested.empty())
                    return nested;
            }
            
            return {};
        }

        std::string JsonString(const Json &value, const char *key)
        {
            if (!value.is_object() || !value.contains(key) || value[key].is_null())
                return {};

            if (value[key].is_string())
                return value[key].get<std::string>();

            return value[key].dump();
        }

        std::optional<std::string> JsonToSalary(const Json &value)
        {            
            if (!value.is_string())
                return std::nullopt;

            std::string digits;
            for (const unsigned char character : value.get_ref<const std::string &>())
                if (std::isdigit(character))
                    digits.push_back(static_cast<char>(character));
            
            if (digits.empty())
                return std::nullopt;

            return std::optional(digits);
        }

        std::optional<std::string> ExtractSalary(const Json &posting)
        {
            if (!posting.contains("baseSalary"))
                return std::nullopt;

            const Json &salary = posting["baseSalary"];
            if (!salary.is_object() || !salary.contains("value"))
                return JsonToSalary(salary);

            const Json &value = salary["value"];
            if (value.is_object())
            {
                std::string salary;
                if (value.contains("minValue"))
                    salary += value["minValue"].dump();
                if (value.contains("maxValue"))
                {
                    if (!salary.empty())
                        salary += " - ";

                    salary += value["maxValue"].dump();
                }

                if (value.contains("unitText"))
                {
                    const std::string unit = value["unitText"].dump();
                    if (unit == "HOUR")
                    {
                        salary += "/hour";
                    } else if (unit == "MONTH")
                    {
                        salary += "/month";
                    }
                }
                    
                return salary.empty() ? std::nullopt : std::optional(salary);
            }

            return JsonToSalary(value);
        }

        bool IsJobPosting(const Json &value)
        {
            if (!value.is_object() || !value.contains("@type"))
                return false;

            const Json &type = value["@type"];
            if (type.is_string())
                return type.get<std::string>() == "JobPosting";

            if (type.is_array())
                return std::find(type.begin(), type.end(), Json("JobPosting")) != type.end();

            return false;
        }

        const Json *FindJobPosting(const Json &value)
        {
            // JobPosting may be nested in an array or an @graph, so search the complete JSON tree.
            if (IsJobPosting(value))
                return &value;
            
            if (value.is_array())
            {
                for (const Json &child : value)
                    if (const Json *posting = FindJobPosting(child))
                        return posting;
            }

            if (value.is_object())
            {
                for (const auto &[key, child] : value.items())
                {
                    static_cast<void>(key);
                    if (const Json *posting = FindJobPosting(child))
                        return posting;
                }
            }

            return nullptr;
        }

        std::string AddressFromJson(const Json &posting)
        {
            if (!posting.contains("jobLocation"))
                return {};
            
            const Json &location = posting["jobLocation"];
            const Json *address = &location;
            if (location.is_array() && !location.empty())
                address = &location.front();
            if (!address->is_object() || !address->contains("address"))
                return {};

            const Json &parts = (*address)["address"];
            std::string result;
            for (const char *key : {"streetAddress", "addressLocality", "addressRegion"})
            {
                const std::string value = JsonString(parts, key);
                if (!value.empty())
                {
                    if (!result.empty())
                        result += ", ";

                    result += value;
                }
            }

            return result;
        }

        void ApplyPosting(JobOffer &offer, const Json &posting)
        {
            const std::string title = JsonString(posting, "title");
            if (!title.empty())
                offer.title = title;

            const std::string description = JsonString(posting, "description");
            if (!description.empty())
            {
                // Descriptions commonly contain HTML; parse it to store readable text rather than markup.
                Document description_document = ParseHtml(description);
                offer.job_description = description_document
                    ? NodeContent(xmlDocGetRootElement(description_document.get()))
                    : description;
            }

            if (posting.contains("hiringOrganization"))
            {
                const std::string company = JsonString(posting["hiringOrganization"], "name");
                if (!company.empty())
                    offer.company_name = company;
            }

            const std::string address = AddressFromJson(posting);
            if (!address.empty())
                offer.company_address = address;

            const std::string date_posted = JsonString(posting, "datePosted");
            if (!date_posted.empty())
                offer.created_at = date_posted;

            offer.salary = ExtractSalary(posting);
        }

        std::optional<std::string> RelativePostedDate(const std::string &text)
        {
            const auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
            const std::string lower = Lower(text);

            if (lower.find("dnes") != std::string::npos)
            {
                std::ostringstream date;
                date << std::chrono::year_month_day{today};
                return date.str();
            }

            if (lower.find("včera") != std::string::npos)
            {
                std::ostringstream date;
                date << std::chrono::year_month_day{today - std::chrono::days{1}};
                return date.str();
            }

            static const std::regex relative_date(
                R"(před\s+([0-9]+)\s+(minutami?|minutou|hodinami?|hodinou|dnem|dne|dny|dní|týdnem|týdne|týdny|měsícem|měsíci|měsíců))");
            std::smatch match;
            if (std::regex_search(lower, match, relative_date))
            {
                const int amount = std::stoi(match[1].str());
                const std::string unit = Lower(match[2].str());
                std::chrono::system_clock::duration age{};
                if (unit.starts_with("minut"))
                    age = std::chrono::minutes{amount};
                else if (unit.starts_with("hodin"))
                    age = std::chrono::hours{amount};
                else if (unit.starts_with("d"))
                    age = std::chrono::days{amount};
                else if (unit.starts_with("t"))
                    age = std::chrono::days{amount * 7};
                else
                    age = std::chrono::days{amount * 30};

                std::ostringstream date;
                date << std::chrono::year_month_day{
                    std::chrono::floor<std::chrono::days>(
                        std::chrono::system_clock::now() - age)};

                return date.str();
            }

            static const std::regex named_date(
                R"(([0-9]{1,2})\.\s*(ledna|února|března|dubna|května|června|července|srpna|září|října|listopadu|prosince))");
            if (!std::regex_search(lower, match, named_date))
                return std::nullopt;

            static const std::array<std::string_view, 12> months{
                "ledna", "února", "března", "dubna", "května", "června",
                "července", "srpna", "září", "října", "listopadu", "prosince"};
            const auto month = std::find(months.begin(), months.end(), match[2].str());
            const auto month_number = static_cast<unsigned>(std::distance(months.begin(), month) + 1);
            std::chrono::year_month_day posted{
                std::chrono::year_month_day{today}.year(),
                std::chrono::month{month_number},
                std::chrono::day{static_cast<unsigned>(std::stoi(match[1].str()))}};
            if (!posted.ok())
                return std::nullopt;

            std::chrono::sys_days posted_day{posted};
            if (posted_day > today)
            {
                posted.year() -= std::chrono::years{1};
                if (!posted.ok())
                    return std::nullopt;
                posted_day = std::chrono::sys_days{posted};
            }

            std::ostringstream date;
            date << std::chrono::year_month_day{posted_day};

            return date.str();
        }

        void ApplyStructuredData(JobOffer &offer, xmlNode *root)
        {
            for (xmlNode *node = root; node; node = node->next)
            {
                if (node->type == XML_ELEMENT_NODE &&
                    xmlStrcasecmp(node->name, BAD_CAST "script") == 0 &&
                    Lower(GetProperty(node, "type")) == "application/ld+json")
                {
                    try
                    {
                        const Json data = Json::parse(NodeContent(node));
                        if (const Json *posting = FindJobPosting(data))
                        {
                            ApplyPosting(offer, *posting);

                            break;
                        }
                    }
                    catch (const Json::exception &)
                    {
                    }
                }

                ApplyStructuredData(offer, node->children);
            }
        }

        void ApplyMetadata(JobOffer &offer, xmlNode *root)
        {
            xmlNode *head = FindHead(root);
            
            if (offer.title.empty())
                offer.title = FindMeta(head, {"og:title", "twitter:title"});

            if (!offer.created_at.has_value())
                offer.created_at = RelativePostedDate(NodeContent(root));
        }

        JobOffer ParseOffer(JobOffer &offer, const std::string &detailHtml)
        {
            if (!detailHtml.empty())
            {
                Document detail = ParseHtml(detailHtml);
                if (detail)
                {
                    xmlNode *root = xmlDocGetRootElement(detail.get());

                    // For Jobs.cz's own pages the data is contained within script
                    if (offer.job_url.find("/nabidka/") != std::string::npos)
                        ApplyStructuredData(offer, root);
                    
                    ApplyMetadata(offer, root);

                    if (offer.job_description.empty())
                    {
                        xmlNode *content = FindContentElement(root);
                        offer.job_description = NodeContent(content ? content : root);
                    }
                }
            }

            if (offer.title.empty())
                offer.title = "Unknown Job offer";

            if (offer.company_name.empty())
                offer.company_name = "Unknown Company";

            return offer;
        }

        void ReadListingPage(std::vector<JobOffer> &offers, const std::string &pageHtml, 
            const std::string &pageUrl)
        {
            Document page = ParseHtml(pageHtml);
            if (!page)
                return;

            xmlNode *card_container = FindCardContainer(FindContentElement(xmlDocGetRootElement(page.get())));
            if (!card_container)
                return;

            // Traverse all cards in the container and parse them to JobOffer.
            for (xmlNode *stack_item = card_container->children; stack_item; stack_item = stack_item->next)
            {
                JobOffer offer;
                if (!ExtractCardLink(stack_item->children, pageUrl, offer.job_url, offer.title))
                    continue;

                const std::string node_content = NodeContent(stack_item);

                const auto posted_date = RelativePostedDate(node_content);
                if (posted_date.has_value())
                    offer.created_at = posted_date;

                ExtractCompanyName(node_content, offer.company_name);

                offers.push_back(std::move(offer));
            }
        }
    } // namespace

    AlmaAdapter::AlmaAdapter(FetchAdapter *fetcher)
        : fetcher_(std::unique_ptr<FetchAdapter>(fetcher))
    {

    }

    std::string AlmaAdapter::Name() const
    {
        return fetcher_->Name();
    }

    void AlmaAdapter::Scrape(const SearchFilters &filters, std::vector<JobOffer> &offers)
    {
        std::set<std::string> seen;
        for (int page = 1; page <= filters.max_pages; ++page)
        {
            const std::string page_url = fetcher_->BuildSearchUrl(filters, page);

            JOBSCRAPER_INFO("Reading {} search page {}: {}", Name(), page, page_url);

            std::vector<JobOffer> listings;
            try
            {
                ReadListingPage(listings, fetcher_->Fetch(page_url), page_url);
            }
            catch (const std::exception &error)
            {
                if (page == 1)
                    throw;
                
                JOBSCRAPER_WARN("Could not read {} page {}: {}", Name(), page, error.what());

                break;
            }

            JOBSCRAPER_INFO("Found {} offers on {} search page {}.", listings.size(), Name(), page);

            int added = 0;;

            for (JobOffer &listing : listings)
            {
                const std::string url = listing.job_url;
                listing.job_url = BuildCanonicalUrl(url);

                // Skip duplicate listings that may appear across multiple pages.
                if (!seen.insert(listing.job_url).second)
                    continue;

                JOBSCRAPER_INFO("Scraping offer page \"{}\": {}", listing.title, url);
                try
                {
                    offers.push_back(ParseOffer(listing, fetcher_->Fetch(url)));
                    added++;
                }
                catch (const std::exception &error)
                {
                    JOBSCRAPER_WARN("Could not read job detail {}: {}", url, error.what());
                }
            }

            JOBSCRAPER_INFO("Scraped {} offers from {} search page {}.", added, Name(), page);

            // Stop scraping if no new listings were added, which may indicate the last page.
            if (added == 0)
            {
                JOBSCRAPER_INFO("No new offers found on {} search page {}; stopping.", Name(), page);
                
                break;
            }
        }
    }
} // namespace jobscraper