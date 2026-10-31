#include "jobscraper/Log/Formatter/UppercaseLevelFormatter.hpp"

// std
#include <algorithm>

namespace jobscraper
{
    namespace log
    {
        void UppercaseLevelFormatter::format(const spdlog::details::log_msg &msg, const std::tm &time, spdlog::memory_buf_t &dest)
        {
            std::string level_string = spdlog::level::to_string_view(msg.level).data();

            // Convert to uppercase
            std::transform(level_string.begin(), level_string.end(), level_string.begin(), ::toupper);

            dest.append(level_string.data(), level_string.data() + level_string.size());
        }

        std::unique_ptr<spdlog::custom_flag_formatter> UppercaseLevelFormatter::clone() const
        {
            return spdlog::details::make_unique<UppercaseLevelFormatter>();
        }
    } // namespace log
} // namespace jobscraper