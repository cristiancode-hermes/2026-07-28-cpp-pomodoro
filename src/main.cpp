#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <print>
#include <ranges>
#include <ratio>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// ── Version ──
static constexpr const char* VERSION = "1.0.0";
static constexpr const char* APP_NAME = "Pomodoro Timer";

// ── ANSI color codes ──
namespace color {
  constexpr auto RESET   = "\033[0m";
  constexpr auto BOLD    = "\033[1m";
  constexpr auto DIM     = "\033[2m";
  constexpr auto RED     = "\033[31m";
  constexpr auto GREEN   = "\033[32m";
  constexpr auto YELLOW  = "\033[33m";
  constexpr auto BLUE    = "\033[34m";
  constexpr auto CYAN    = "\033[36m";
  constexpr auto WHITE   = "\033[37m";
  constexpr auto GRAY    = "\033[90m";
  constexpr auto BOLD_RED   = "\033[1;31m";
  constexpr auto BOLD_GREEN = "\033[1;32m";
  constexpr auto BOLD_CYAN  = "\033[1;36m";
}

// ── Session state ──
struct Config {
  int work_minutes     = 25;
  int break_minutes    = 5;
  int long_break_minutes = 15;
  int sessions_before_long = 4;
  bool no_color  = false;
  bool json_mode = false;
  bool timer_mode = false;
};

enum class SessionType { Work, Break, LongBreak };

struct SessionState {
  SessionType type = SessionType::Work;
  int current_session = 0;
  int total_sessions = 4;
  bool running = true;
};

// ── Help text ──
static void print_help() {
  std::println(std::cout, R"(
╔═══════════════════════════════════════════╗
║  {} — C++23 CLI Timer  ║
╚═══════════════════════════════════════════╝

A Pomodoro Technique timer for productivity.
Work in focused intervals with short breaks.

USAGE:
  pomodoro [OPTIONS]

OPTIONS:
  -h, --help            Show this help message
  -V, --version         Show version
  -w, --work MIN        Work duration in minutes (default: 25)
  -b, --break MIN       Break duration in minutes (default: 5)
  -l, --long MIN        Long break duration in minutes (default: 15)
  -s, --sessions N      Sessions before long break (default: 4)
  --no-color            Disable colored output
  --json                Output session plan as JSON
  --timer               Run interactive timer with real-time updates

EXAMPLES:
  pomodoro                        Start with defaults (25 min work, 5 min break)
  pomodoro --work 30 --break 10   Custom durations
  pomodoro --timer                Interactive countdown timer
  pomodoro --json                 Show schedule as JSON)", APP_NAME);
}

// ── Version ──
static void print_version() {
  std::println(std::cout, "{} v{}", APP_NAME, VERSION);
}

// ── Format duration (minutes:seconds) ──
[[maybe_unused]] static std::string fmt_duration(int total_seconds) {
  int m = total_seconds / 60;
  int s = total_seconds % 60;
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
  return buf;
}

// ── Progress bar ──
static std::string make_progress_bar(double pct, int width = 20) {
  int filled = std::clamp(static_cast<int>(std::round(pct / 100.0 * width)), 0, width);
  std::string bar;
  bar.reserve(width + 2);
  bar += '[';
  for (int i = 0; i < filled; ++i) bar += '=';
  if (filled < width) bar += '>';
  for (int i = filled + 1; i < width; ++i) bar += ' ';
  bar += ']';
  return bar;
}

// ── Format session type name ──
[[maybe_unused]] static std::string_view session_type_name(SessionType t) {
  switch (t) {
    case SessionType::Work:     return "Work";
    case SessionType::Break:    return "Break";
    case SessionType::LongBreak: return "Long Break";
  }
  return "Unknown";
}

// ── Parse CLI args ──
static Config parse_args(int argc, char** argv) {
  Config cfg;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg{argv[i]};
    if (arg == "--help" || arg == "-h") {
      print_help();
      std::exit(0);
    } else if (arg == "--version" || arg == "-V") {
      print_version();
      std::exit(0);
    } else if (arg == "--no-color") {
      cfg.no_color = true;
    } else if (arg == "--json") {
      cfg.json_mode = true;
    } else if (arg == "--timer") {
      cfg.timer_mode = true;
    } else if (arg == "--work" || arg == "-w") {
      if (++i >= argc) { std::println(std::cerr, "❌ --work requires a value"); std::exit(1); }
      cfg.work_minutes = std::atoi(argv[i]);
      if (cfg.work_minutes <= 0) { std::println(std::cerr, "❌ Work duration must be > 0"); std::exit(1); }
    } else if (arg == "--break" || arg == "-b") {
      if (++i >= argc) { std::println(std::cerr, "❌ --break requires a value"); std::exit(1); }
      cfg.break_minutes = std::atoi(argv[i]);
      if (cfg.break_minutes <= 0) { std::println(std::cerr, "❌ Break duration must be > 0"); std::exit(1); }
    } else if (arg == "--long" || arg == "-l") {
      if (++i >= argc) { std::println(std::cerr, "❌ --long requires a value"); std::exit(1); }
      cfg.long_break_minutes = std::atoi(argv[i]);
      if (cfg.long_break_minutes <= 0) { std::println(std::cerr, "❌ Long break duration must be > 0"); std::exit(1); }
    } else if (arg == "--sessions" || arg == "-s") {
      if (++i >= argc) { std::println(std::cerr, "❌ --sessions requires a value"); std::exit(1); }
      cfg.sessions_before_long = std::atoi(argv[i]);
      if (cfg.sessions_before_long <= 0) { std::println(std::cerr, "❌ Sessions must be > 0"); std::exit(1); }
    } else if (arg.starts_with('-')) {
      std::println(std::cerr, "❌ Unknown option: {}", arg);
      std::exit(1);
    }
  }
  return cfg;
}

// ── Print session plan ──
static void print_plan(const Config& cfg) {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm* tm = std::localtime(&t);

  char time_buf[32];
  std::strftime(time_buf, sizeof(time_buf), "%H:%M", tm);
  char date_buf[16];
  std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", tm);

  if (!cfg.no_color) {
    std::println(std::cout, "{}📋 Pomodoro Session Plan{}", color::BOLD_CYAN, color::RESET);
    std::println(std::cout, "{}  {}  |  Started at {}{}", color::DIM, date_buf, time_buf, color::RESET);
    std::println(std::cout, "");
    std::println(std::cout, "  {}Work:{}        {} min     {}", color::BOLD, color::RESET, cfg.work_minutes,
      cfg.no_color ? "" : std::string(color::GREEN));
    std::println(std::cout, "  {}Break:{}       {} min", color::BOLD, color::RESET, cfg.break_minutes);
    std::println(std::cout, "  {}Long Break:{}  {} min  (every {} sessions)", color::BOLD, color::RESET,
      cfg.long_break_minutes, cfg.sessions_before_long);
    if (!cfg.no_color) std::print(std::cout, "{}", color::RESET);

    std::println(std::cout, "");
    std::println(std::cout, "  {}Schedule:{}{}", color::BOLD, cfg.no_color ? "" : " " + std::string(color::CYAN), color::RESET);

    // Print session schedule
    for (int s = 0; s < cfg.sessions_before_long; ++s) {
      bool is_last = (s == cfg.sessions_before_long - 1);
      std::println(std::cout, "    Session {}:  Work {} min  →  {} {} min",
        s + 1, cfg.work_minutes,
        is_last ? "Long Break" : "Break",
        is_last ? cfg.long_break_minutes : cfg.break_minutes);
    }
    std::println(std::cout, "");
    std::println(std::cout, "  {}Total time:{}  {} min = {} h {} min{}",
      color::BOLD, color::RESET,
      cfg.sessions_before_long * cfg.work_minutes +
        (cfg.sessions_before_long - 1) * cfg.break_minutes +
        cfg.long_break_minutes,
      (cfg.sessions_before_long * cfg.work_minutes +
        (cfg.sessions_before_long - 1) * cfg.break_minutes +
        cfg.long_break_minutes) / 60,
      (cfg.sessions_before_long * cfg.work_minutes +
        (cfg.sessions_before_long - 1) * cfg.break_minutes +
        cfg.long_break_minutes) % 60,
      cfg.no_color ? "" : std::string(color::GRAY));
    if (!cfg.no_color) std::print(std::cout, "{}", color::RESET);
  } else {
    std::println(std::cout, "Pomodoro Session Plan");
    std::println(std::cout, "  Date: {}  Time: {}", date_buf, time_buf);
    std::println(std::cout, "  Work: {} min | Break: {} min | Long Break: {} min (every {} sessions)",
      cfg.work_minutes, cfg.break_minutes, cfg.long_break_minutes, cfg.sessions_before_long);
    std::println(std::cout, "  Schedule:");
    for (int s = 0; s < cfg.sessions_before_long; ++s) {
      bool is_last = (s == cfg.sessions_before_long - 1);
      std::println(std::cout, "    Session {}: Work {} min -> {} {} min",
        s + 1, cfg.work_minutes,
        is_last ? "Long Break" : "Break",
        is_last ? cfg.long_break_minutes : cfg.break_minutes);
    }
  }
}

// ── Print JSON plan ──
static void print_json_plan(const Config& cfg) {
  std::println(std::cout, R"({{)");
  std::println(std::cout, R"(  "app": "Pomodoro Timer",)");
  std::println(std::cout, R"(  "version": "{}",)", VERSION);
  std::println(std::cout, R"(  "work_minutes": {},)", cfg.work_minutes);
  std::println(std::cout, R"(  "break_minutes": {},)", cfg.break_minutes);
  std::println(std::cout, R"(  "long_break_minutes": {},)", cfg.long_break_minutes);
  std::println(std::cout, R"(  "sessions_before_long": {},)", cfg.sessions_before_long);
  std::println(std::cout, R"(  "total_minutes": {},)",
    cfg.sessions_before_long * cfg.work_minutes +
      (cfg.sessions_before_long - 1) * cfg.break_minutes +
      cfg.long_break_minutes);

  std::println(std::cout, R"(  "schedule": [)");
  for (int s = 0; s < cfg.sessions_before_long; ++s) {
    bool is_last_session = (s == cfg.sessions_before_long - 1);
    std::println(std::cout, R"(    {{"session": {}, "type": "work", "minutes": {}}}{})",
      s + 1, cfg.work_minutes, is_last_session ? "" : ",");
    std::println(std::cout, R"(    {{"session": {}, "type": "{}", "minutes": {}}}{})",
      s + 1,
      is_last_session ? "long_break" : "break",
      is_last_session ? cfg.long_break_minutes : cfg.break_minutes,
      "");  // Always last entry per session pair
  }
  std::println(std::cout, R"(  ])");
  std::println(std::cout, R"(}})");
}

// ── Interactive timer ──
static void run_timer(const Config& cfg) {
  if (!cfg.no_color) {
    std::println(std::cout, "{}⏰ Pomodoro Timer — Interactive Mode{}", color::BOLD_CYAN, color::RESET);
    std::println(std::cout, "{}  Ctrl+C to stop the current session{}", color::DIM, color::RESET);
  } else {
    std::println(std::cout, "Pomodoro Timer — Interactive Mode");
    std::println(std::cout, "  Ctrl+C to stop the current session");
  }

  for (int session = 1; session <= cfg.sessions_before_long; ++session) {
    bool is_last = (session == cfg.sessions_before_long);

    // Work session
    if (!cfg.no_color) {
      std::println(std::cout, "");
      std::println(std::cout, "{}━━━ Work Session {}/{} ━━━{}", color::BOLD_CYAN, session, cfg.sessions_before_long, color::RESET);
    } else {
      std::println(std::cout, "\n--- Work Session {}/{} ---", session, cfg.sessions_before_long);
    }

    int work_seconds = cfg.work_minutes * 60;
    auto start = std::chrono::steady_clock::now();

    for (int elapsed = 0; elapsed < work_seconds; ++elapsed) {
      int remaining = work_seconds - elapsed;
      double pct = static_cast<double>(elapsed) / work_seconds * 100.0;
      std::string bar = make_progress_bar(pct);

      if (cfg.no_color) {
        std::print(std::cout, "\r  ⏳ {} | {}%", fmt_duration(remaining), static_cast<int>(pct));
      } else {
        std::string color_code = (pct < 50.0) ? std::string(color::GREEN) :
                                 (pct < 80.0) ? std::string(color::YELLOW) : std::string(color::RED);
        std::print(std::cout, "\r  {}⏳ {} | {}  {}{:.0f}%{}",
          color_code, fmt_duration(remaining),
          bar, color_code, pct, color::RESET);
      }
      std::cout.flush();
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!cfg.no_color) {
      std::println(std::cout, "");
      std::println(std::cout, "{}✓ Work session {} complete!{}", color::BOLD_GREEN, session, color::RESET);
    } else {
      std::println(std::cout, "\n✓ Work session {} complete!", session);
    }

    // Break session
    int break_seconds = (is_last ? cfg.long_break_minutes : cfg.break_minutes) * 60;
    std::string_view break_name = is_last ? "Long Break" : "Break";

    if (!cfg.no_color) {
      std::println(std::cout, "{}━━━ {} ({}) ━━━{}", color::BOLD_CYAN, break_name, session, color::RESET);
    } else {
      std::println(std::cout, "--- {} ({}) ---", break_name, session);
    }

    start = std::chrono::steady_clock::now();
    for (int elapsed = 0; elapsed < break_seconds; ++elapsed) {
      int remaining = break_seconds - elapsed;
      double pct = static_cast<double>(elapsed) / break_seconds * 100.0;
      std::string bar = make_progress_bar(pct);

      if (cfg.no_color) {
        std::print(std::cout, "\r  ☕ {} | {}%", fmt_duration(remaining), static_cast<int>(pct));
      } else {
        std::string color_code = (elapsed < break_seconds / 2) ? std::string(color::BLUE) : std::string(color::CYAN);
        std::print(std::cout, "\r  {}☕ {} | {}  {}{:.0f}%{}",
          color_code, fmt_duration(remaining),
          bar, color_code, pct, color::RESET);
      }
      std::cout.flush();
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!cfg.no_color) {
      std::println(std::cout, "");
      std::println(std::cout, "{}✓ {} complete!{}", color::BOLD_GREEN, break_name, color::RESET);
    } else {
      std::println(std::cout, "\n✓ {} complete!", break_name);
    }
  }

  if (!cfg.no_color) {
    std::println(std::cout, "");
    std::println(std::cout, "{}🎉 All sessions complete! Great focus!{}", color::BOLD_GREEN, color::RESET);
  } else {
    std::println(std::cout, "\nAll sessions complete! Great focus!");
  }
}

// ── Entry point ──
int main(int argc, char** argv) {
  Config cfg = parse_args(argc, argv);

  if (cfg.json_mode) {
    print_json_plan(cfg);
  } else if (cfg.timer_mode) {
    run_timer(cfg);
  } else {
    print_plan(cfg);
  }

  return 0;
}
