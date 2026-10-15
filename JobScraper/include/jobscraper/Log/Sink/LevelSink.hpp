#pragma once

// std
#include <string>
#include <array>

// spdlog
#include <spdlog/spdlog.h>

namespace jobscraper
{
    namespace log
    {
        // --- Level Sink Base --- //

        class level_sink_base
        {
        public:
            level_sink_base()
            {
                enabled_levels.fill(true);
            }

            void set_level(spdlog::level::level_enum level, bool enabled)
            {
                if (level >= spdlog::level::level_enum::n_levels)
                    return;

                enabled_levels[level] = enabled;
            }

            bool should_log_level(spdlog::level::level_enum level) const
            {
                if (level >= spdlog::level::level_enum::n_levels)
                    return false;
                
                return enabled_levels[level];
            }

        protected:
            std::array<bool, spdlog::level::level_enum::n_levels> enabled_levels;
        };

        // --- Level Sink --- //

        template <typename ConsoleMutex>
        class level_sink : public spdlog::sinks::sink, public level_sink_base
        {
        public:
            using mutex_t = typename ConsoleMutex::mutex_t;

            level_sink(FILE *target_file) :
                target_file_(target_file),
                mutex_(ConsoleMutex::mutex()),
                formatter_(spdlog::details::make_unique<spdlog::pattern_formatter>())
            {

            }

            ~level_sink() override = default;

            void log(const spdlog::details::log_msg &msg) override
            {
                std::lock_guard<mutex_t> lock(mutex_);

                if (!should_log_level(msg.level))
                    return;

                msg.color_range_start = 0;
                msg.color_range_end = 0;

                spdlog::memory_buf_t formatted;
                formatter_->format(msg, formatted);

                spdlog::details::os::fwrite_bytes(formatted.data(), formatted.size(), target_file_);
                fflush(target_file_);
            }

            void flush() override
            {
                std::lock_guard<mutex_t> lock(mutex_);

                fflush(target_file_);
            }

            void set_pattern(const std::string &pattern) final override
            {
                std::lock_guard<mutex_t> lock(mutex_);

                formatter_ = std::unique_ptr<spdlog::formatter>(new spdlog::pattern_formatter(pattern));
            }

            void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override
            {
                std::lock_guard<mutex_t> lock(mutex_);
                
                formatter_ = std::move(sink_formatter);
            }

        private:
            FILE *target_file_;
            mutex_t &mutex_;
            std::unique_ptr<spdlog::formatter> formatter_;
        };

        // --- Stdout Level Sink --- //

        template <typename ConsoleMutex>
        class stdout_level_sink : public level_sink<ConsoleMutex>
        {
        public:
            stdout_level_sink() : level_sink<ConsoleMutex>(stdout)
            {
                this->enabled_levels[spdlog::level::err] = false;
                this->enabled_levels[spdlog::level::critical] = false;
            }
        };

        using stdout_level_sink_mt = stdout_level_sink<spdlog::details::console_mutex>;
        using stdout_level_sink_st = stdout_level_sink<spdlog::details::null_mutex>;

        // --- Stderr Level Sink --- //

        template <typename ConsoleMutex>
        class stderr_level_sink : public level_sink<ConsoleMutex>
        {
        public:
            stderr_level_sink() : level_sink<ConsoleMutex>(stderr)
            {
                this->enabled_levels[spdlog::level::trace] = false;
                this->enabled_levels[spdlog::level::debug] = false;
                this->enabled_levels[spdlog::level::info] = false;
                this->enabled_levels[spdlog::level::warn] = false;
            }
        };

        using stderr_level_sink_mt = stderr_level_sink<spdlog::details::console_mutex>;
        using stderr_level_sink_st = stderr_level_sink<spdlog::details::null_mutex>;
    } // namespace log
} // namespace jobscraper