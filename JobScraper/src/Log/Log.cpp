#include "jobscraper/Log/Log.hpp"

// spdlog
#include <spdlog/sinks/stdout_color_sinks.h>

// Job Scraper
#include "jobscraper/Log/Sink/ColorLevelSink.hpp"
#include "jobscraper/Log/Formatter/UppercaseLevelFormatter.hpp"

namespace jobscraper
{
    namespace log
    {
        std::shared_ptr<spdlog::logger> Log::s_Logger_;

        void Log::Init()
        {   
            std::vector<spdlog::sink_ptr> sinks;
            sinks.push_back(std::make_shared<stdout_color_level_sink_mt>());
            sinks.push_back(std::make_shared<stderr_color_level_sink_mt>());
            
            // Initialize core logger
            s_Logger_ = std::make_shared<spdlog::logger>("JOB_SCRAPER", begin(sinks), end(sinks));
            #ifdef DEBUG
            s_Logger_->set_level(spdlog::level::trace);
            #else
            s_Logger_->set_level(spdlog::level::info);
            #endif
            s_Logger_->flush_on(spdlog::level::trace);
            s_Logger_->set_formatter(std::move(GetFormatter()));

            spdlog::set_default_logger(s_Logger_);
        }

        inline std::unique_ptr<spdlog::pattern_formatter> Log::GetFormatter()
        {
            std::unique_ptr<spdlog::pattern_formatter> formatter = std::make_unique<spdlog::pattern_formatter>();
            formatter->add_flag<UppercaseLevelFormatter>('w');

            #ifdef DEBUG
            formatter->set_pattern("[%d-%m-%Y %X.%e] [%s:%#] [%^%w%$]: %v");
            #else
            formatter->set_pattern("[%d-%m-%Y %X.%e] [%^%w%$]: %v");
            #endif

            return formatter;
        }
    } // namespace log
} // namespace jobscraper