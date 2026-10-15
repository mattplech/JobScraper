#include "jobscraper/Log/Formatter/UppercaseLevelFormatter.hpp"

// std
#include <algorithm>

namespace jobscraper
{
    namespace log
    {
        void UppercaseLevelFormatter::format(const spdlog::details::log_msg &msg, const std::tm &time, spdlog::memory_buf_t &dest)
        {
            std::string levelString = spdlog::level::to_string_view(msg.level).data();
            std::transform(levelString.begin(), levelString.end(), levelString.begin(), ::toupper); // Convert to uppercase

            dest.append(levelString.data(), levelString.data() + levelString.size());
        }

        std::unique_ptr<spdlog::custom_flag_formatter> UppercaseLevelFormatter::clone() const
        {
            return spdlog::details::make_unique<UppercaseLevelFormatter>();
        }
    } // namespace log
} // namespace jobscraper