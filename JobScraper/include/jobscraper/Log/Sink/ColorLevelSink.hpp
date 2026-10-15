#pragma once

// spdlog
#include <spdlog/spdlog.h>

// Job Scraper
#include "jobscraper/Log/Sink/LevelSink.hpp"

namespace jobscraper
{
        namespace log
        {
            // --- Color Level Sink --- //

            template <typename ConsoleMutex>
            class color_level_sink : public spdlog::sinks::sink, public level_sink_base
            {
            public:
                using mutex_t = typename ConsoleMutex::mutex_t;

                color_level_sink(FILE *target_file, spdlog::color_mode mode) :
                    target_file_(target_file),
                    mutex_(ConsoleMutex::mutex()),
                    formatter_(spdlog::details::make_unique<spdlog::pattern_formatter>())
                {
                    set_color_mode(mode);

                    colors_.at(spdlog::level::trace) = to_string_(intensive_cyan);
                    colors_.at(spdlog::level::debug) = to_string_(cyan);
                    colors_.at(spdlog::level::info) = to_string_(green);
                    colors_.at(spdlog::level::warn) = to_string_(yellow_bold);
                    colors_.at(spdlog::level::err) = to_string_(red_bold);
                    colors_.at(spdlog::level::critical) = to_string_(bold_on_red);
                    colors_.at(spdlog::level::off) = to_string_(reset);
                }

                ~color_level_sink() override = default;

                void set_color(spdlog::level::level_enum color_level, spdlog::string_view_t color)
                {
                    std::lock_guard<mutex_t> lock(mutex_);

                    colors_.at(static_cast<size_t>(color_level)) = to_string_(color);
                }

                void set_color_mode(spdlog::color_mode mode)
                {
                    switch (mode)
                    {
                        case spdlog::color_mode::always:
                            should_do_colors_ = true;
                            return;
                        case spdlog::color_mode::automatic:
                            should_do_colors_ = spdlog::details::os::in_terminal(target_file_) && spdlog::details::os::is_color_terminal();
                            return;
                        case spdlog::color_mode::never:
                            should_do_colors_ = false;
                            return;
                        default:
                            should_do_colors_ = false;
                    }
                }

                bool should_color()
                {
                    return should_do_colors_;
                }

                void log(const spdlog::details::log_msg &msg) override
                {
                    std::lock_guard<mutex_t> lock(mutex_);

                    if (!should_log_level(msg.level))
                        return;

                    msg.color_range_start = 0;
                    msg.color_range_end = 0;

                    spdlog::memory_buf_t formatted;
                    formatter_->format(msg, formatted);

                    if (should_do_colors_ && msg.color_range_end > msg.color_range_start)
                    {
                        print_range_(formatted, 0, msg.color_range_start);

                        print_ccode_(colors_.at(static_cast<size_t>(msg.level)));
                        print_range_(formatted, msg.color_range_start, msg.color_range_end);
                        print_ccode_(reset);

                        print_range_(formatted, msg.color_range_end, formatted.size());
                    } else
                    {
                        print_range_(formatted, 0, formatted.size());
                    }

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

                // Formatting codes
                static constexpr std::string_view reset = "\033[m";
                static constexpr std::string_view bold = "\033[1m";
                static constexpr std::string_view dark = "\033[2m";
                static constexpr std::string_view underline = "\033[4m";
                static constexpr std::string_view blink = "\033[5m";
                static constexpr std::string_view reverse = "\033[7m";
                static constexpr std::string_view concealed = "\033[8m";
                static constexpr std::string_view clear_line = "\033[K";

                // Foreground colors
                static constexpr std::string_view black = "\033[30m";
                static constexpr std::string_view red = "\033[31m";
                static constexpr std::string_view green = "\033[32m";
                static constexpr std::string_view yellow = "\033[33m";
                static constexpr std::string_view blue = "\033[34m";
                static constexpr std::string_view magenta = "\033[35m";
                static constexpr std::string_view cyan = "\033[36m";
                static constexpr std::string_view white = "\033[37m";

                static constexpr std::string_view intensive_black = "\033[90m";
                static constexpr std::string_view intensive_red = "\033[91m";
                static constexpr std::string_view intensive_green = "\033[92m";
                static constexpr std::string_view intensive_yellow = "\033[93m";
                static constexpr std::string_view intensive_blue = "\033[94m";
                static constexpr std::string_view intensive_magenta = "\033[95m";
                static constexpr std::string_view intensive_cyan = "\033[96m";
                static constexpr std::string_view intensive_white = "\033[97m";

                /// Background colors
                static constexpr std::string_view on_black = "\033[40m";
                static constexpr std::string_view on_red = "\033[41m";
                static constexpr std::string_view on_green = "\033[42m";
                static constexpr std::string_view on_yellow = "\033[43m";
                static constexpr std::string_view on_blue = "\033[44m";
                static constexpr std::string_view on_magenta = "\033[45m";
                static constexpr std::string_view on_cyan = "\033[46m";
                static constexpr std::string_view on_white = "\033[47m";

                /// Bold colors
                static constexpr std::string_view yellow_bold = "\033[33m\033[1m";
                static constexpr std::string_view red_bold = "\033[31m\033[1m";
                static constexpr std::string_view bold_on_red = "\033[1m\033[41m";

            private:
                FILE *target_file_;
                mutex_t &mutex_;
                bool should_do_colors_;
                std::unique_ptr<spdlog::formatter> formatter_;
                std::array<std::string, spdlog::level::n_levels> colors_;

                void print_ccode_(const spdlog::string_view_t &color_code)
                {
                    spdlog::details::os::fwrite_bytes(color_code.data(), color_code.size(), target_file_);
                }

                void print_range_(const spdlog::memory_buf_t &formatted, size_t start, size_t end)
                {
                    spdlog::details::os::fwrite_bytes(formatted.data() + start, end - start, target_file_);
                }

                static std::string to_string_(const spdlog::string_view_t &sv)
                {
                    return std::string(sv.data(), sv.size());
                }
            };

            // --- Stdout Color Level Sink --- //

            template <typename ConsoleMutex>
            class stdout_color_level_sink : public color_level_sink<ConsoleMutex>
            {
            public:
                stdout_color_level_sink(spdlog::color_mode mode = spdlog::color_mode::automatic) : color_level_sink<ConsoleMutex>(stdout, mode)
                {
                    this->enabled_levels[spdlog::level::err] = false;
                    this->enabled_levels[spdlog::level::critical] = false;
                }
            };

            using stdout_color_level_sink_mt = stdout_color_level_sink<spdlog::details::console_mutex>;
            using stdout_color_level_sink_st = stdout_color_level_sink<spdlog::details::null_mutex>;

            // --- Stderr Color Level Sink --- //

            template <typename ConsoleMutex>
            class stderr_color_level_sink : public color_level_sink<ConsoleMutex>
            {
            public:
                stderr_color_level_sink(spdlog::color_mode mode = spdlog::color_mode::automatic) : color_level_sink<ConsoleMutex>(stderr, mode)
                {
                    this->enabled_levels[spdlog::level::trace] = false;
                    this->enabled_levels[spdlog::level::debug] = false;
                    this->enabled_levels[spdlog::level::info] = false;
                    this->enabled_levels[spdlog::level::warn] = false;
                }
            };

            using stderr_color_level_sink_mt = stderr_color_level_sink<spdlog::details::console_mutex>;
            using stderr_color_level_sink_st = stderr_color_level_sink<spdlog::details::null_mutex>;
        } // namespace log
} // namespace jobscraper