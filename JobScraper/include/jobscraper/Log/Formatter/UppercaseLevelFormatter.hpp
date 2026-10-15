#pragma once

// spdlog
#include <spdlog/pattern_formatter.h>

namespace jobscraper
{
    namespace log
    {
        class UppercaseLevelFormatter : public spdlog::custom_flag_formatter
        {
        public:
            void format(const spdlog::details::log_msg &msg, const std::tm &time, spdlog::memory_buf_t &dest) override;
            std::unique_ptr<spdlog::custom_flag_formatter> clone() const override;
        };
    } // namespace log
} // namespace jobscraper