#pragma once

// std
#include <memory>

// spdlog
#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>

namespace jobscraper
{
    class JobScraper;

    namespace log
    {
        class Log
        {
            friend class jobscraper::JobScraper;
        public:
            inline static spdlog::logger* GetLogger() { return s_Logger_.get(); }

        private:
            static void Init();

            inline static std::unique_ptr<spdlog::pattern_formatter> GetFormatter();

            static std::shared_ptr<spdlog::logger> s_Logger_;
        };
    } // namespace log
} // namespace jobscraper

//  log macros
#ifdef DEBUG
#define JOBSCRAPER_TRACE(...)    jobscraper::log::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::trace, __VA_ARGS__)
#define JOBSCRAPER_DEBUG(...)    jobscraper::log::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::debug, __VA_ARGS__)
#else
#define JOBSCRAPER_TRACE(...)    (void)0
#define JOBSCRAPER_DEBUG(...)    (void)0
#endif
#define JOBSCRAPER_INFO(...)     jobscraper::log::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::info, __VA_ARGS__)
#define JOBSCRAPER_WARN(...)     jobscraper::log::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::warn, __VA_ARGS__)
#define JOBSCRAPER_ERROR(...)    jobscraper::log::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::err, __VA_ARGS__)
#define JOBSCRAPER_CRITICAL(...) jobscraper::log::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::critical, __VA_ARGS__)


// Format management macros
#define JOBSCRAPER_LOGGER_SET_PATTERN(pattern) jobscraper::log::Log::GetLogger()->set_pattern(pattern);

// Log level management macros
#define JOBSCRAPER_LOGGER_SET_LEVEL(level) jobscraper::log::Log::GetLogger()->set_level(level);
#define JOBSCRAPER_LOGGER_GET_LEVEL() jobscraper::log::Log::GetLogger()->level()

#define JOBSCRAPER_LOGGER_INIT() jobscraper::log::Log::Init();