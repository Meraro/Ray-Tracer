#ifndef RENDER_PROGRESS_H
#define RENDER_PROGRESS_H

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

class render_progress {
    public:
        explicit render_progress(
            int total_steps,
            std::chrono::milliseconds min_progress_update_interval = std::chrono::milliseconds(50),
            std::chrono::milliseconds spinner_update_interval = std::chrono::milliseconds(200),
            int bar_width = 40
        )
            : total_steps(total_steps > 0 ? total_steps : 1),
              min_progress_update_interval(min_progress_update_interval),
              spinner_update_interval(spinner_update_interval),
              bar_width(bar_width > 0 ? bar_width : 1) {
            std::clog << ansi_hide_cursor << std::flush;
            cursor_hidden = true;
            auto now = clock::now();
            last_progress_report_time = now - min_progress_update_interval;
            last_spinner_update_time = now - spinner_update_interval;
        }

        ~render_progress() {
            if (cursor_hidden) {
                std::clog << ansi_show_cursor << std::flush;
            }
        }

        void update(int completed_steps, bool force = false) {
            int target_percent = (100 * completed_steps) / total_steps;
            if (target_percent > 100) target_percent = 100;
            if (target_percent < 0) target_percent = 0;

            auto now = clock::now();

            bool should_update_percent = false;
            if (force) {
                should_update_percent = true;
            } else if (target_percent >= displayed_percent + 1 &&
                       (now - last_progress_report_time) >= min_progress_update_interval) {
                should_update_percent = true;
            }

            bool should_update_spinner = false;
            if (force) {
                should_update_spinner = true;
            } else if ((now - last_spinner_update_time) >= spinner_update_interval) {
                should_update_spinner = true;
            }

            if (!should_update_percent && !should_update_spinner) return;

            if (should_update_percent) {
                displayed_percent = target_percent;
                last_progress_report_time = now;
            }

            if (should_update_spinner) {
                spinner_index = (spinner_index + 1) % 4;
                last_spinner_update_time = now;
            }

            int filled = (bar_width * displayed_percent) / 100;
            const char* spinner = spinner_frames[spinner_index % 4];

            std::clog << "\r" << ansi_clear_line << "\r"
                      << ansi_spinner << spinner << ansi_reset << " "
                      << ansi_title << "Rendering" << ansi_reset << " ["
                      << ansi_fill << std::string(filled, '=') << ansi_reset
                      << ansi_empty << std::string(bar_width - filled, '.') << ansi_reset
                      << "] "
                      << ansi_percent << std::setw(3) << displayed_percent << "%" << ansi_reset
                      << std::flush;
        }

        void done() {
            update(total_steps, true);
            std::clog << "\r" << ansi_clear_line << "\r"
                      << ansi_done << "Done." << ansi_reset << "\n"
                      << ansi_show_cursor << std::flush;
            cursor_hidden = false;
        }

    private:
        using clock = std::chrono::steady_clock;

        static constexpr const char* spinner_frames[4] = {"|", "/", "-", "\\"};
        static constexpr const char* ansi_reset = "\x1b[0m";
        static constexpr const char* ansi_clear_line = "\x1b[2K";
        static constexpr const char* ansi_hide_cursor = "\x1b[?25l";
        static constexpr const char* ansi_show_cursor = "\x1b[?25h";
        static constexpr const char* ansi_title = "\x1b[1;38;2;93;173;226m";    // #5DADE2
        static constexpr const char* ansi_spinner = "\x1b[1;38;2;226;213;93m";  // #E2D55D
        static constexpr const char* ansi_fill = "\x1b[1;38;2;173;226;93m";     // #ADE25D
        static constexpr const char* ansi_empty = "\x1b[38;2;168;176;186m";     // #A8B0BA
        // static constexpr const char* ansi_percent = "\x1b[1;38;2;146;93;226m";  // #925DE2
        static constexpr const char* ansi_percent = "";
        static constexpr const char* ansi_done = "\x1b[1;38;2;173;226;93m";     // #ADE25D

        int total_steps;
        std::chrono::milliseconds min_progress_update_interval;
        std::chrono::milliseconds spinner_update_interval;
        int bar_width;

        int displayed_percent = 0;
        clock::time_point last_progress_report_time;
        clock::time_point last_spinner_update_time;
        int spinner_index = 0;
        bool cursor_hidden = false;
};

#endif
