#pragma once

#ifndef __has_include
#define __has_include(x) 0
#endif

#if __cplusplus < 202302L
#error "lofi/traits.hpp requires C++23"
#endif

#if defined(LOFI_ENABLE_GOOGLE_DRIVE)
#ifndef LOFI_ENABLE_HTTP
#define LOFI_ENABLE_HTTP
#endif
#ifndef LOFI_ENABLE_JSON
#define LOFI_ENABLE_JSON
#endif
#endif

#if defined(LOFI_ENABLE_GITHUB)
#ifndef LOFI_ENABLE_HTTP
#define LOFI_ENABLE_HTTP
#endif
#ifndef LOFI_ENABLE_JSON
#define LOFI_ENABLE_JSON
#endif
#endif

#if defined(LOFI_ENABLE_OPENSSL_JWT)
#ifndef LOFI_ENABLE_HTTP
#define LOFI_ENABLE_HTTP
#endif
#ifndef LOFI_ENABLE_JSON
#define LOFI_ENABLE_JSON
#endif
#endif

#if (defined(LOFI_ENABLE_GOOGLE_DRIVE) || defined(LOFI_ENABLE_GITHUB) || \
     defined(LOFI_ENABLE_OPENSSL_JWT)) &&                                \
    !defined(CPPHTTPLIB_OPENSSL_SUPPORT)
#error "LOFI HTTPS helpers require CPPHTTPLIB_OPENSSL_SUPPORT"
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <print>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(LOFI_ENABLE_DUCKDB)
#if __has_include(<duckdb.h>)
#include <duckdb.h>
#else
#error "LOFI_ENABLE_DUCKDB requires duckdb.h"
#endif
#endif

#if defined(LOFI_ENABLE_HTTP)
#if __has_include(<httplib.h>)
#include <httplib.h>
#else
#error "LOFI_ENABLE_HTTP requires httplib.h"
#endif
#endif

#if defined(LOFI_ENABLE_JSON)
#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
#else
#error "LOFI_ENABLE_JSON requires nlohmann/json.hpp"
#endif
#endif

#if defined(LOFI_ENABLE_OPENSSL_JWT)
#if __has_include(<openssl/bio.h>) && __has_include(<openssl/evp.h>) &&          \
    __has_include(<openssl/pem.h>)
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#else
#error "LOFI_ENABLE_OPENSSL_JWT requires OpenSSL headers"
#endif
#endif

#if defined(LOFI_ENABLE_SFTP)
#if defined(_WIN32)
#error "LOFI_ENABLE_SFTP currently requires POSIX sockets"
#endif
#if __has_include(<libssh2.h>) && __has_include(<libssh2_sftp.h>)
#include <libssh2.h>
#include <libssh2_sftp.h>
#else
#error "LOFI_ENABLE_SFTP requires libssh2 headers"
#endif

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#endif

namespace lofi {

namespace ansi {
inline constexpr std::string_view reset = "\033[0m";
inline constexpr std::string_view bold = "\033[1m";
inline constexpr std::string_view dim = "\033[2m";
inline constexpr std::string_view cyan = "\033[36m";
inline constexpr std::string_view green = "\033[32m";
inline constexpr std::string_view yellow = "\033[33m";
inline constexpr std::string_view red = "\033[31m";
}  // namespace ansi

namespace detail {

inline std::mutex& LogMutex() {
  static std::mutex mutex;
  return mutex;
}

inline void WriteLine(std::FILE* stream, std::string_view text) {
  const std::lock_guard<std::mutex> lock(LogMutex());
  std::println(stream, "{}", text);
  std::fflush(stream);
}

inline std::tm LocalTime(std::time_t value) {
  std::tm out{};
#if defined(_WIN32)
  localtime_s(&out, &value);
#else
  localtime_r(&value, &out);
#endif
  return out;
}

inline std::tm GmTime(std::time_t value) {
  std::tm out{};
#if defined(_WIN32)
  gmtime_s(&out, &value);
#else
  gmtime_r(&value, &out);
#endif
  return out;
}

inline long long DaysFromCivil(int year,
                               unsigned month,
                               unsigned day) noexcept {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<long long>(era) * 146097 + static_cast<long long>(doe) -
         719468;
}

inline bool IsSpace(char ch) {
  return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

inline bool IsDigit(char ch) {
  return std::isdigit(static_cast<unsigned char>(ch)) != 0;
}

inline char LowerAscii(char ch) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}

}  // namespace detail

enum class LogLevel {
  step,
  info,
  ok,
  done,
  warn,
  error,
};

inline std::string_view LogLevelName(LogLevel level) {
  switch (level) {
    case LogLevel::step:
      return "step";
    case LogLevel::info:
      return "info";
    case LogLevel::ok:
      return "ok";
    case LogLevel::done:
      return "done";
    case LogLevel::warn:
      return "warn";
    case LogLevel::error:
      return "error";
  }
  return "log";
}

inline std::string_view LogLevelColor(LogLevel level) {
  switch (level) {
    case LogLevel::step:
    case LogLevel::ok:
    case LogLevel::done:
      return ansi::green;
    case LogLevel::info:
      return ansi::cyan;
    case LogLevel::warn:
      return ansi::yellow;
    case LogLevel::error:
      return ansi::red;
  }
  return {};
}

inline std::FILE* LogLevelStream(LogLevel level) {
  return level == LogLevel::warn || level == LogLevel::error ? stderr : stdout;
}

inline void LogLine(LogLevel level,
                    std::string_view message,
                    bool use_color = true) {
  const std::string_view name = LogLevelName(level);
  std::string line;
  line.reserve(message.size() + name.size() + 24);
  if (use_color) {
    line.append(LogLevelColor(level));
  }
  line.append(name);
  if (use_color) {
    line.append(ansi::reset);
  }
  if (name.size() < 5) {
    line.append(5 - name.size(), ' ');
  }
  line.append(" ");
  line.append(message);
  detail::WriteLine(LogLevelStream(level), line);
}

inline void LogStep(std::string_view message) {
  LogLine(LogLevel::step, message);
}

inline void LogInfo(std::string_view message) {
  LogLine(LogLevel::info, message);
}

inline void LogOk(std::string_view message) {
  LogLine(LogLevel::ok, message);
}

inline void LogDone(std::string_view message) {
  LogLine(LogLevel::done, message);
}

inline void LogWarn(std::string_view message) {
  LogLine(LogLevel::warn, message);
}

inline void LogError(std::string_view message) {
  LogLine(LogLevel::error, message);
}

template <class... Args>
inline void LogStep(std::format_string<Args...> fmt, Args&&... args) {
  LogStep(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
inline void LogInfo(std::format_string<Args...> fmt, Args&&... args) {
  LogInfo(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
inline void LogOk(std::format_string<Args...> fmt, Args&&... args) {
  LogOk(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
inline void LogDone(std::format_string<Args...> fmt, Args&&... args) {
  LogDone(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
inline void LogWarn(std::format_string<Args...> fmt, Args&&... args) {
  LogWarn(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
inline void LogError(std::format_string<Args...> fmt, Args&&... args) {
  LogError(std::format(fmt, std::forward<Args>(args)...));
}

inline void PrintStep(std::string_view label,
                      std::string_view state,
                      std::string_view color = ansi::green) {
  std::string line;
  line.reserve(label.size() + state.size() + 32);
  line.append(color);
  line.append(ansi::bold);
  line.push_back('[');
  line.append(state);
  line.push_back(']');
  line.append(ansi::reset);
  line.push_back(' ');
  line.append(label);
  detail::WriteLine(stdout, line);
}

inline void PrintProgress(std::size_t current,
                          std::size_t total,
                          std::string_view label,
                          std::string_view suffix = {}) {
  constexpr std::size_t width = 24;
  const std::size_t filled =
      total == 0 ? width
                 : std::min(width, (current * width + total / 2) / total);

  std::string bar;
  bar.reserve(width);
  bar.append(filled, '#');
  bar.append(width - filled, '-');

  std::string line;
  line.reserve(label.size() + suffix.size() + width + 64);
  line.append(ansi::cyan);
  line.append(ansi::bold);
  line.push_back('[');
  line.append(bar);
  line.push_back(']');
  line.append(ansi::reset);
  line.push_back(' ');
  line.append(std::to_string(current));
  line.push_back('/');
  line.append(std::to_string(total));
  line.push_back(' ');
  line.append(ansi::dim);
  line.append(label);
  line.append(ansi::reset);
  if (!suffix.empty()) {
    line.append(" -> ");
    line.append(ansi::green);
    line.append(suffix);
    line.append(ansi::reset);
  }
  detail::WriteLine(stdout, line);
}

inline void PrintWarn(std::string_view label) {
  PrintStep(label, "WARN", ansi::yellow);
}

inline std::string Trim(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && detail::IsSpace(text[begin])) {
    ++begin;
  }

  std::size_t end = text.size();
  while (end > begin && detail::IsSpace(text[end - 1])) {
    --end;
  }

  return std::string(text.substr(begin, end - begin));
}

inline std::vector<std::string> Split(std::string_view text, char delimiter) {
  std::vector<std::string> parts;
  std::string current;
  for (char ch : text) {
    if (ch == delimiter) {
      parts.push_back(std::move(current));
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  parts.push_back(std::move(current));
  return parts;
}

inline std::unordered_map<std::string, std::string> ParseParams(
    std::string_view query) {
  std::unordered_map<std::string, std::string> out;
  std::size_t pos = 0;
  while (pos <= query.size()) {
    const std::size_t amp = query.find('&', pos);
    const std::string_view part = query.substr(
        pos, amp == std::string_view::npos ? query.size() - pos : amp - pos);
    if (!part.empty()) {
      const std::size_t eq = part.find('=');
      std::string key = std::string(part.substr(0, eq));
      std::string value = eq == std::string_view::npos
                              ? std::string{}
                              : std::string(part.substr(eq + 1));
      out.emplace(std::move(key), std::move(value));
    }
    if (amp == std::string_view::npos) {
      break;
    }
    pos = amp + 1;
  }
  return out;
}

inline std::string ShiftUtcTimestamp(std::string_view timestamp) {
  if (timestamp.empty()) {
    return {};
  }

  std::string clean(timestamp);
  while (!clean.empty() && clean.back() == 'Z') {
    clean.pop_back();
  }
  clean.append("+00:00");
  return clean;
}

inline bool EndsWithCaseInsensitive(std::string_view text,
                                    std::string_view suffix) {
  if (text.size() < suffix.size()) {
    return false;
  }

  text.remove_prefix(text.size() - suffix.size());
  for (std::size_t i = 0; i < suffix.size(); ++i) {
    if (detail::LowerAscii(text[i]) != detail::LowerAscii(suffix[i])) {
      return false;
    }
  }
  return true;
}

inline bool EndsWithCsv(std::string_view name) {
  return EndsWithCaseInsensitive(name, ".csv");
}

inline std::optional<std::string> GetEnv(const char* name) {
  const char* value = std::getenv(name);
  if (value == nullptr || *value == '\0') {
    return std::nullopt;
  }
  return std::string(value);
}

inline std::string EnvOr(const char* name, std::string fallback = {}) {
  if (auto value = GetEnv(name)) {
    return *value;
  }
  return fallback;
}

inline void LoadEnvOverride(const char* name, std::string& value) {
  if (auto env = GetEnv(name)) {
    value = std::move(*env);
  }
}

inline void LoadEnvOverride(const char* name, std::filesystem::path& value) {
  if (auto env = GetEnv(name)) {
    value = std::move(*env);
  }
}

inline std::string ReadFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("failed to open " + path.string());
  }
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

inline void WriteFile(const std::filesystem::path& path,
                      std::string_view data) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw std::runtime_error("failed to write " + path.string());
  }
  output.write(data.data(), static_cast<std::streamsize>(data.size()));
  if (!output) {
    throw std::runtime_error("failed while writing " + path.string());
  }
}

inline std::string QuoteSqlString(std::string_view value) {
  std::string escaped;
  escaped.reserve(value.size() + 8);
  escaped.push_back('\'');
  for (char ch : value) {
    if (ch == '\'') {
      escaped.append("''");
    } else {
      escaped.push_back(ch);
    }
  }
  escaped.push_back('\'');
  return escaped;
}

inline std::string SqlQuote(std::string_view value) {
  return QuoteSqlString(value);
}

inline std::string BuildOdbcConnectionString(std::string_view server,
                                             std::string_view database,
                                             std::string_view username,
                                             std::string_view password,
                                             std::string_view driver) {
  std::ostringstream connection;
  connection << "Driver={" << driver << "};"
             << "Server=" << server << ';' << "Database=" << database << ';'
             << "UID=" << username << ';' << "PWD=" << password << ';'
             << "Encrypt=yes;TrustServerCertificate=yes;";
  return connection.str();
}

inline std::string BuildOdbcConnectionStringFromEnv(const char* server_env,
                                                    const char* database_env,
                                                    const char* username_env,
                                                    const char* password_env,
                                                    const char* driver_env) {
  const auto server = GetEnv(server_env);
  const auto database = GetEnv(database_env);
  const auto username = GetEnv(username_env);
  const auto password = GetEnv(password_env);
  const auto driver = GetEnv(driver_env);
  if (!server || !database || !username || !password || !driver) {
    throw std::runtime_error(
        "missing one or more ODBC connection environment variables");
  }
  return BuildOdbcConnectionString(*server, *database, *username, *password,
                                   *driver);
}

struct DateYmd {
  int year = 0;
  unsigned month = 0;
  unsigned day = 0;
};

inline bool IsLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

inline unsigned DaysInMonth(int year, unsigned month) {
  static constexpr std::array<unsigned, 12> days = {31, 28, 31, 30, 31, 30,
                                                    31, 31, 30, 31, 30, 31};
  if (month == 0 || month > 12) {
    return 0;
  }
  if (month == 2 && IsLeapYear(year)) {
    return 29;
  }
  return days[month - 1];
}

inline bool IsValidDate(DateYmd date) {
  return date.month >= 1 && date.month <= 12 && date.day >= 1 &&
         date.day <= DaysInMonth(date.year, date.month);
}

inline DateYmd ParseDateYmd(std::string_view text) {
  int year = 0;
  int month = 0;
  int day = 0;
  char tail = '\0';
  const std::string value(text);
  if (std::sscanf(value.c_str(), "%d-%d-%d%c", &year, &month, &day, &tail) !=
      3) {
    throw std::runtime_error("invalid date format, expected YYYY-MM-DD");
  }

  DateYmd out{year, static_cast<unsigned>(month), static_cast<unsigned>(day)};
  if (!IsValidDate(out)) {
    throw std::runtime_error("invalid calendar date");
  }
  return out;
}

inline std::string FormatDateYmd(DateYmd date) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d-%02u-%02u", date.year, date.month,
                date.day);
  return buf;
}

inline std::string FormatDateTag(DateYmd date) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d%02u%02u", date.year, date.month,
                date.day);
  return buf;
}

inline std::string FormatDate(DateYmd date) {
  return FormatDateTag(date);
}

inline std::tm ToTm(DateYmd date) {
  std::tm tm{};
  tm.tm_year = date.year - 1900;
  tm.tm_mon = static_cast<int>(date.month) - 1;
  tm.tm_mday = static_cast<int>(date.day);
  tm.tm_isdst = -1;
  return tm;
}

inline DateYmd FromTm(const std::tm& tm) {
  return {tm.tm_year + 1900, static_cast<unsigned>(tm.tm_mon + 1),
          static_cast<unsigned>(tm.tm_mday)};
}

inline std::tm ParseDate(std::string_view text) {
  return ToTm(ParseDateYmd(text));
}

inline std::string YesterdayDateYmd() {
  const auto time = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now() - std::chrono::hours{24});
  return FormatDateYmd(FromTm(detail::LocalTime(time)));
}

inline std::string YesterdayDate() {
  return YesterdayDateYmd();
}

inline std::string DefaultDateYmd() {
  return YesterdayDateYmd();
}

inline std::string NextDateYmd(std::string_view text) {
  std::tm tm = ToTm(ParseDateYmd(text));
  const std::time_t time = std::mktime(&tm) + 24 * 60 * 60;
  return FormatDateYmd(FromTm(detail::LocalTime(time)));
}

inline std::string NextDate(std::string_view text) {
  return NextDateYmd(text);
}

inline DateYmd DateAtUtcOffset(std::chrono::system_clock::time_point time,
                               std::chrono::seconds utc_offset) {
  const auto shifted = time + utc_offset;
  const auto shifted_time = std::chrono::system_clock::to_time_t(shifted);
  return FromTm(detail::GmTime(shifted_time));
}

inline DateYmd TodayAtUtcOffset(std::chrono::seconds utc_offset) {
  return DateAtUtcOffset(std::chrono::system_clock::now(), utc_offset);
}

inline std::chrono::year_month_day CalendarDateAtUtcOffset(
    std::chrono::system_clock::time_point time,
    std::chrono::seconds utc_offset) {
  const DateYmd date = DateAtUtcOffset(time, utc_offset);
  return std::chrono::year{date.year} / std::chrono::month{date.month} /
         std::chrono::day{date.day};
}

inline std::chrono::year_month_day TodayCalendarDateAtUtcOffset(
    std::chrono::seconds utc_offset) {
  return CalendarDateAtUtcOffset(std::chrono::system_clock::now(), utc_offset);
}

inline std::string FormatDate(const std::chrono::year_month_day& date) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d%02u%02u", static_cast<int>(date.year()),
                static_cast<unsigned>(date.month()),
                static_cast<unsigned>(date.day()));
  return buf;
}

inline std::string DateTag(const std::tm& tm) {
  return FormatDateTag(FromTm(tm));
}

inline std::string DateTagAtUtcOffset(std::chrono::seconds utc_offset) {
  return FormatDateTag(TodayAtUtcOffset(utc_offset));
}

inline std::chrono::year_month_day UtcMinusFiveToday() {
  return TodayCalendarDateAtUtcOffset(std::chrono::hours{-5});
}

inline std::string YesterdayDateTagAtUtcOffset(
    std::chrono::seconds utc_offset) {
  return FormatDateTag(DateAtUtcOffset(
      std::chrono::system_clock::now() - std::chrono::hours{24}, utc_offset));
}

inline std::string MonthsAgoDateTagAtUtcOffset(std::chrono::seconds utc_offset,
                                               int months) {
  const DateYmd today = TodayAtUtcOffset(utc_offset);
  int total_months =
      today.year * 12 + static_cast<int>(today.month) - 1 - months;
  int year = total_months / 12;
  int month_index = total_months % 12;
  if (month_index < 0) {
    month_index += 12;
    --year;
  }

  const unsigned month = static_cast<unsigned>(month_index + 1);
  const unsigned day = std::min(today.day, DaysInMonth(year, month));
  return FormatDateTag({year, month, day});
}

inline std::string YesterdayUtcMinusFiveDate() {
  return YesterdayDateTagAtUtcOffset(std::chrono::hours{-5});
}

inline std::string TwoMonthsAgoUtcMinusFiveDate() {
  return MonthsAgoDateTagAtUtcOffset(std::chrono::hours{-5}, 2);
}

inline std::string NormalizeDateTag(std::string_view raw_date) {
  std::string digits;
  digits.reserve(8);
  for (char ch : raw_date) {
    if (detail::IsDigit(ch)) {
      digits.push_back(ch);
      continue;
    }
    if (ch != '-') {
      throw std::runtime_error("date must use YYYYMMDD or YYYY-MM-DD");
    }
  }

  if (digits.size() != 8) {
    throw std::runtime_error("date must use YYYYMMDD or YYYY-MM-DD");
  }

  const int year = std::stoi(digits.substr(0, 4));
  const unsigned month = static_cast<unsigned>(std::stoi(digits.substr(4, 2)));
  const unsigned day = static_cast<unsigned>(std::stoi(digits.substr(6, 2)));
  if (!IsValidDate({year, month, day})) {
    throw std::runtime_error("date is not a valid calendar date");
  }
  return digits;
}

inline std::string NormalizeDate(std::string_view raw_date) {
  return NormalizeDateTag(raw_date);
}

inline bool IsDateTag(std::string_view text) {
  return text.size() == 8 &&
         std::all_of(text.begin(), text.end(), detail::IsDigit);
}

inline bool IsRemoteDateDir(std::string_view name) {
  return IsDateTag(name);
}

inline std::string FormatUtcTimestamp(std::time_t time) {
  const std::tm out = detail::GmTime(time);
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
                out.tm_year + 1900, out.tm_mon + 1, out.tm_mday, out.tm_hour,
                out.tm_min, out.tm_sec);
  return buf;
}

inline std::pair<std::string, std::string> DayBoundsUtc(
    std::string_view local_date,
    std::chrono::seconds utc_offset) {
  const DateYmd date = ParseDateYmd(local_date);
  const long long local_days =
      detail::DaysFromCivil(date.year, date.month, date.day);
  const long long start_seconds = local_days * 86400 - utc_offset.count();
  const std::time_t start = static_cast<std::time_t>(start_seconds);
  const std::time_t end = start + 24 * 60 * 60 - 1;
  return {FormatUtcTimestamp(start), FormatUtcTimestamp(end)};
}

inline std::pair<std::string, std::string> DayBoundsUtc(
    std::string_view local_date) {
  return DayBoundsUtc(local_date, std::chrono::hours{-5});
}

inline std::string ShiftTs(std::string_view timestamp) {
  return ShiftUtcTimestamp(timestamp);
}

inline std::string DecodeBase64(std::string_view input) {
  static constexpr std::string_view alphabet =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  std::array<int, 256> table{};
  table.fill(-1);
  for (std::size_t i = 0; i < alphabet.size(); ++i) {
    table[static_cast<unsigned char>(alphabet[i])] = static_cast<int>(i);
  }

  std::string output;
  int value = 0;
  int bits = -8;
  for (char ch : input) {
    const auto uch = static_cast<unsigned char>(ch);
    if (std::isspace(uch)) {
      continue;
    }
    if (ch == '=') {
      break;
    }
    if (table[uch] == -1) {
      throw std::runtime_error("invalid base64 input");
    }
    value = (value << 6) + table[uch];
    bits += 6;
    if (bits >= 0) {
      output.push_back(static_cast<char>((value >> bits) & 0xFF));
      bits -= 8;
    }
  }
  return output;
}

inline std::string Base64UrlEncode(const unsigned char* data,
                                   std::size_t size) {
  static constexpr char alphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  std::string output;
  output.reserve((size * 4 + 2) / 3);

  std::uint32_t value = 0;
  int bits = -6;
  for (std::size_t i = 0; i < size; ++i) {
    value = (value << 8) | data[i];
    bits += 8;
    while (bits >= 0) {
      output.push_back(alphabet[(value >> bits) & 0x3F]);
      bits -= 6;
    }
  }
  if (bits > -6) {
    output.push_back(alphabet[((value << 8) >> (bits + 8)) & 0x3F]);
  }
  return output;
}

inline std::string Base64UrlEncode(std::string_view text) {
  return Base64UrlEncode(reinterpret_cast<const unsigned char*>(text.data()),
                         text.size());
}

inline std::string UrlEncode(std::string_view text) {
  static constexpr char hex[] = "0123456789ABCDEF";
  std::string output;
  output.reserve(text.size() * 3);
  for (unsigned char ch : text) {
    if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
      output.push_back(static_cast<char>(ch));
    } else {
      output.push_back('%');
      output.push_back(hex[ch >> 4]);
      output.push_back(hex[ch & 0xF]);
    }
  }
  return output;
}

#if defined(LOFI_ENABLE_DUCKDB)
inline std::string DuckdbValueVarchar(duckdb_result* result,
                                      idx_t col,
                                      idx_t row) {
  char* value = ::duckdb_value_varchar(result, col, row);
  std::string out = value != nullptr ? value : "";
  duckdb_free(value);
  return out;
}

inline void RunSql(duckdb_connection connection,
                   std::string_view sql,
                   std::string_view context) {
  duckdb_result result{};
  const std::string query(sql);
  const duckdb_state state = duckdb_query(connection, query.c_str(), &result);
  if (state == DuckDBError) {
    const char* error = duckdb_result_error(&result);
    const std::string message = error != nullptr ? error : "unknown error";
    duckdb_destroy_result(&result);
    throw std::runtime_error(std::string(context) + ": " + message);
  }
  duckdb_destroy_result(&result);
}
#endif

#if defined(LOFI_ENABLE_HTTP)
struct HttpResponse {
  long status = 0;
  std::string body;
};

inline std::pair<std::string, std::string> SplitUrlOriginPath(
    const std::string& url) {
  const auto scheme_end = url.find("://");
  if (scheme_end == std::string::npos) {
    throw std::runtime_error("invalid URL: " + url);
  }
  const auto path_start = url.find('/', scheme_end + 3);
  const std::string origin =
      path_start == std::string::npos ? url : url.substr(0, path_start);
  const std::string path =
      path_start == std::string::npos ? "/" : url.substr(path_start);
  return {origin, path};
}

inline HttpResponse HttpRequest(std::string_view method,
                                const std::string& url,
                                const std::vector<std::string>& headers,
                                const std::string* body = nullptr) {
  const auto [origin, path] = SplitUrlOriginPath(url);

  httplib::Client client(origin);
  client.set_read_timeout(120);
  client.set_follow_location(true);

  httplib::Headers request_headers;
  std::string content_type;
  for (const auto& header : headers) {
    const auto colon = header.find(": ");
    if (colon == std::string::npos) {
      continue;
    }
    std::string key = header.substr(0, colon);
    std::string value = header.substr(colon + 2);
    if (key == "Content-Type") {
      content_type = std::move(value);
    } else {
      request_headers.emplace(std::move(key), std::move(value));
    }
  }

  httplib::Result result;
  if (method == "GET") {
    result = client.Get(path, request_headers);
  } else if (method == "POST") {
    result = client.Post(path, request_headers,
                         body != nullptr ? *body : std::string{}, content_type);
  } else if (method == "DELETE") {
    result = client.Delete(path, request_headers);
  } else {
    throw std::runtime_error("unsupported HTTP method: " + std::string(method));
  }

  if (!result) {
    throw std::runtime_error("HTTP request failed: " +
                             httplib::to_string(result.error()));
  }

  return {result->status, std::move(result->body)};
}
#endif

#if defined(LOFI_ENABLE_OPENSSL_JWT)
inline constexpr std::string_view google_oauth_token_url =
    "https://oauth2.googleapis.com/token";
inline constexpr std::string_view google_drive_scope =
    "https://www.googleapis.com/auth/drive";

struct ServiceAccountCredentials {
  std::string client_email;
  std::string private_key;
  std::string token_uri = std::string(google_oauth_token_url);
};

inline ServiceAccountCredentials ParseServiceAccountCredentials(
    std::string_view json_text) {
  nlohmann::json json;
  try {
    json = nlohmann::json::parse(json_text);
  } catch (const nlohmann::json::exception& error) {
    throw std::runtime_error(std::string("credentials JSON is invalid: ") +
                             error.what());
  }

  ServiceAccountCredentials credentials;
  try {
    credentials.client_email = json.at("client_email").get<std::string>();
    credentials.private_key = json.at("private_key").get<std::string>();
  } catch (const nlohmann::json::exception& error) {
    throw std::runtime_error(std::string("credentials JSON missing field: ") +
                             error.what());
  }
  credentials.token_uri =
      json.value("token_uri", std::string(google_oauth_token_url));
  return credentials;
}

inline ServiceAccountCredentials ParseServiceAccountCredentialsBase64(
    std::string_view encoded) {
  return ParseServiceAccountCredentials(DecodeBase64(encoded));
}

inline ServiceAccountCredentials ReadServiceAccountCredentialsFromEnv(
    const char* env_name) {
  const auto value = GetEnv(env_name);
  if (!value) {
    throw std::runtime_error(std::string(env_name) + " is required");
  }
  return ParseServiceAccountCredentialsBase64(*value);
}

inline std::string SignRs256(std::string_view data,
                             const std::string& private_key) {
  std::unique_ptr<BIO, decltype(&BIO_free)> bio(
      BIO_new_mem_buf(private_key.data(), static_cast<int>(private_key.size())),
      BIO_free);
  if (!bio) {
    throw std::runtime_error("failed to create BIO for private key");
  }

  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(
      PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr),
      EVP_PKEY_free);
  if (!key) {
    throw std::runtime_error("failed to read private key");
  }

  std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(),
                                                              EVP_MD_CTX_free);
  if (!ctx) {
    throw std::runtime_error("failed to create signing context");
  }

  if (EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha256(), nullptr,
                         key.get()) != 1) {
    throw std::runtime_error("failed to initialize RS256 signing");
  }
  if (EVP_DigestSignUpdate(ctx.get(), data.data(), data.size()) != 1) {
    throw std::runtime_error("failed to sign data");
  }

  std::size_t signature_size = 0;
  if (EVP_DigestSignFinal(ctx.get(), nullptr, &signature_size) != 1) {
    throw std::runtime_error("failed to size signature");
  }

  std::vector<unsigned char> signature(signature_size);
  if (EVP_DigestSignFinal(ctx.get(), signature.data(), &signature_size) != 1) {
    throw std::runtime_error("failed to finalize signature");
  }
  signature.resize(signature_size);
  return Base64UrlEncode(signature.data(), signature.size());
}

inline std::string BuildJwt(const ServiceAccountCredentials& credentials,
                            std::string_view scope = google_drive_scope) {
  const auto now = std::chrono::system_clock::now();
  const auto iat =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();

  const std::string header =
      nlohmann::json{{"alg", "RS256"}, {"typ", "JWT"}}.dump();
  const std::string claim = nlohmann::json{{"iss", credentials.client_email},
                                           {"scope", std::string(scope)},
                                           {"aud", credentials.token_uri},
                                           {"iat", iat},
                                           {"exp", iat + 3600}}
                                .dump();

  const std::string signing_input =
      Base64UrlEncode(header) + "." + Base64UrlEncode(claim);
  return signing_input + "." +
         SignRs256(signing_input, credentials.private_key);
}

inline std::string RequestAccessToken(
    const ServiceAccountCredentials& credentials,
    std::string_view scope = google_drive_scope) {
  const std::string jwt = BuildJwt(credentials, scope);
  const std::string body =
      "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer"
      "&assertion=" +
      UrlEncode(jwt);

  const auto response =
      HttpRequest("POST", credentials.token_uri,
                  {"Content-Type: application/x-www-form-urlencoded"}, &body);
  if (response.status < 200 || response.status >= 300) {
    throw std::runtime_error("failed to get access token: HTTP " +
                             std::to_string(response.status) + " " +
                             response.body);
  }

  return nlohmann::json::parse(response.body)
      .at("access_token")
      .get<std::string>();
}
#endif

#if defined(LOFI_ENABLE_GOOGLE_DRIVE)
inline constexpr std::string_view google_drive_api_url =
    "https://www.googleapis.com/drive/v3";
inline constexpr std::string_view google_drive_upload_api_url =
    "https://www.googleapis.com/upload/drive/v3";

struct DriveItem {
  std::string id;
  std::string name;
  std::string mime_type;
};

inline std::string FindSharedDrive(const std::string& token,
                                   std::string_view name) {
  const std::string url = std::string(google_drive_api_url) + "/drives?q=" +
                          UrlEncode("name='" + std::string(name) + "'") +
                          "&fields=" + UrlEncode("drives(id,name)");

  const auto response =
      HttpRequest("GET", url, {"Authorization: Bearer " + token});
  if (response.status < 200 || response.status >= 300) {
    throw std::runtime_error("failed to list shared drives: HTTP " +
                             std::to_string(response.status));
  }

  const auto drives = nlohmann::json::parse(response.body)
                          .value("drives", nlohmann::json::array());
  if (drives.empty()) {
    throw std::runtime_error("shared drive not found: " + std::string(name));
  }
  return drives[0].at("id").get<std::string>();
}

inline std::string FindFolderOptional(const std::string& token,
                                      const std::string& parent_id,
                                      std::string_view name) {
  const std::string query =
      "mimeType='application/vnd.google-apps.folder' and "
      "trashed=false and name='" +
      std::string(name) + "' and '" + parent_id + "' in parents";
  const std::string url =
      std::string(google_drive_api_url) + "/files?q=" + UrlEncode(query) +
      "&fields=" + UrlEncode("files(id,name)") +
      "&supportsAllDrives=true&includeItemsFromAllDrives=true";

  const auto response =
      HttpRequest("GET", url, {"Authorization: Bearer " + token});
  if (response.status < 200 || response.status >= 300) {
    return {};
  }

  const auto files = nlohmann::json::parse(response.body)
                         .value("files", nlohmann::json::array());
  if (files.empty()) {
    return {};
  }
  return files[0].value("id", std::string{});
}

inline std::string FindFolderByName(const std::string& token,
                                    const std::string& parent_id,
                                    std::string_view name) {
  std::string id = FindFolderOptional(token, parent_id, name);
  if (id.empty()) {
    throw std::runtime_error("folder not found: " + std::string(name));
  }
  return id;
}

inline std::string CreateFolder(const std::string& token,
                                std::string_view name,
                                const std::string& parent_id = {}) {
  nlohmann::json metadata{{"name", std::string(name)},
                          {"mimeType", "application/vnd.google-apps.folder"}};
  if (!parent_id.empty()) {
    metadata["parents"] = {parent_id};
  }

  const std::string body = metadata.dump();
  const std::string url = std::string(google_drive_api_url) +
                          "/files?fields=id&supportsAllDrives=true";
  const auto response = HttpRequest(
      "POST", url,
      {"Authorization: Bearer " + token, "Content-Type: application/json"},
      &body);
  if (response.status < 200 || response.status >= 300) {
    throw std::runtime_error("failed to create folder " + std::string(name) +
                             ": HTTP " + std::to_string(response.status));
  }
  return nlohmann::json::parse(response.body).at("id").get<std::string>();
}

inline std::string FindOrCreateFolder(const std::string& token,
                                      std::string_view name,
                                      const std::string& parent_id = {}) {
  std::string id = FindFolderOptional(token, parent_id, name);
  if (!id.empty()) {
    return id;
  }
  return CreateFolder(token, name, parent_id);
}

inline std::string FindFileOptional(const std::string& token,
                                    const std::string& parent_id,
                                    std::string_view name) {
  const std::string query =
      "mimeType!='application/vnd.google-apps.folder' and "
      "trashed=false and name='" +
      std::string(name) + "' and '" + parent_id + "' in parents";
  const std::string url =
      std::string(google_drive_api_url) + "/files?q=" + UrlEncode(query) +
      "&fields=" + UrlEncode("files(id)") +
      "&supportsAllDrives=true&includeItemsFromAllDrives=true";

  const auto response =
      HttpRequest("GET", url, {"Authorization: Bearer " + token});
  if (response.status < 200 || response.status >= 300) {
    return {};
  }

  const auto files = nlohmann::json::parse(response.body)
                         .value("files", nlohmann::json::array());
  if (files.empty()) {
    return {};
  }
  return files[0].value("id", std::string{});
}

inline bool FileExistsInFolder(const std::string& token,
                               const std::string& parent_id,
                               std::string_view name) {
  return !FindFileOptional(token, parent_id, name).empty();
}

inline std::vector<DriveItem> ListDriveChildren(const std::string& token,
                                                const std::string& parent_id) {
  const std::string query = "'" + parent_id + "' in parents and trashed=false";
  const std::string url =
      std::string(google_drive_api_url) + "/files?q=" + UrlEncode(query) +
      "&fields=" + UrlEncode("files(id,name,mimeType)") +
      "&pageSize=1000&supportsAllDrives=true&includeItemsFromAllDrives=true";
  const auto response =
      HttpRequest("GET", url, {"Authorization: Bearer " + token});
  if (response.status < 200 || response.status >= 300) {
    throw std::runtime_error("failed to list Drive children: HTTP " +
                             std::to_string(response.status));
  }

  const auto files = nlohmann::json::parse(response.body)
                         .value("files", nlohmann::json::array());
  std::vector<DriveItem> items;
  items.reserve(files.size());
  for (const auto& file : files) {
    items.push_back({file.value("id", ""), file.value("name", ""),
                     file.value("mimeType", "")});
  }
  return items;
}

inline std::string DownloadDriveFile(const std::string& token,
                                     const std::string& file_id) {
  const std::string url = std::string(google_drive_api_url) + "/files/" +
                          file_id + "?alt=media&supportsAllDrives=true";
  const auto response =
      HttpRequest("GET", url, {"Authorization: Bearer " + token});
  if (response.status < 200 || response.status >= 300) {
    throw std::runtime_error("failed to download file " + file_id + ": HTTP " +
                             std::to_string(response.status));
  }
  return response.body;
}

inline void DownloadDriveFile(const std::string& token,
                              const std::string& file_id,
                              const std::filesystem::path& output_path) {
  WriteFile(output_path, DownloadDriveFile(token, file_id));
}

inline void UploadDriveFile(const std::string& token,
                            const std::filesystem::path& local_path,
                            std::string_view remote_name,
                            const std::string& parent_id) {
  const std::string boundary = "lofi-boundary";
  const std::string metadata = nlohmann::json{
      {"name", std::string(remote_name)},
      {"parents", {parent_id}}}.dump();
  const std::string file_data = ReadFile(local_path);
  const std::string body =
      "--" + boundary +
      "\r\nContent-Type: application/json; charset=UTF-8\r\n\r\n" + metadata +
      "\r\n--" + boundary +
      "\r\nContent-Type: application/octet-stream\r\n\r\n" + file_data +
      "\r\n--" + boundary + "--\r\n";
  const std::string url =
      std::string(google_drive_upload_api_url) +
      "/files?uploadType=multipart&fields=id&supportsAllDrives=true";

  const auto response =
      HttpRequest("POST", url,
                  {"Authorization: Bearer " + token,
                   "Content-Type: multipart/related; boundary=" + boundary},
                  &body);
  if (response.status < 200 || response.status >= 300) {
    throw std::runtime_error("failed to upload " + local_path.string() +
                             ": HTTP " + std::to_string(response.status) + " " +
                             response.body);
  }
}
#endif

#if defined(LOFI_ENABLE_GITHUB)
inline constexpr std::string_view github_api_base = "https://api.github.com";
inline constexpr std::string_view github_upload_base =
    "https://uploads.github.com";

inline std::vector<std::string> GithubHeaders(
    const std::string& token,
    std::string_view content_type,
    std::string_view user_agent = "lofi-traits") {
  return {
      "Authorization: Bearer " + token,
      "Accept: application/vnd.github+json",
      "X-GitHub-Api-Version: 2022-11-28",
      "User-Agent: " + std::string(user_agent),
      "Content-Type: " + std::string(content_type),
  };
}

inline nlohmann::json GetOrCreateRelease(const std::string& token,
                                         const std::string& repo,
                                         const std::string& tag,
                                         const std::string& title = {},
                                         const std::string& notes = {}) {
  const std::string get_url =
      std::string(github_api_base) + "/repos/" + repo + "/releases/tags/" + tag;
  const auto get_response = HttpRequest(
      "GET", get_url, GithubHeaders(token, "application/vnd.github+json"));

  if (get_response.status == 200) {
    return nlohmann::json::parse(get_response.body);
  }
  if (get_response.status != 404) {
    throw std::runtime_error("failed to query GitHub release: HTTP " +
                             std::to_string(get_response.status) + " " +
                             get_response.body);
  }

  const std::string create_body = nlohmann::json{
      {"tag_name", tag},
      {"name", title.empty() ? tag : title},
      {"body", notes}}.dump();
  const std::string create_url =
      std::string(github_api_base) + "/repos/" + repo + "/releases";
  const auto create_response =
      HttpRequest("POST", create_url, GithubHeaders(token, "application/json"),
                  &create_body);

  if (create_response.status != 201) {
    throw std::runtime_error("failed to create GitHub release: HTTP " +
                             std::to_string(create_response.status) + " " +
                             create_response.body);
  }
  return nlohmann::json::parse(create_response.body);
}

inline void DeleteAssetIfExists(const std::string& token,
                                const std::string& repo,
                                const nlohmann::json& release,
                                const std::string& asset_name) {
  for (const auto& asset : release.value("assets", nlohmann::json::array())) {
    if (asset.value("name", "") != asset_name) {
      continue;
    }

    const std::string asset_id =
        std::to_string(asset.value("id", std::int64_t{0}));
    const std::string delete_url = std::string(github_api_base) + "/repos/" +
                                   repo + "/releases/assets/" + asset_id;
    const auto response =
        HttpRequest("DELETE", delete_url,
                    GithubHeaders(token, "application/vnd.github+json"));
    if (response.status != 204) {
      throw std::runtime_error("failed to delete existing asset '" +
                               asset_name + "': HTTP " +
                               std::to_string(response.status));
    }
    return;
  }
}

inline void UploadReleaseAsset(const std::string& token,
                               const std::string& repo,
                               const nlohmann::json& release,
                               const std::string& asset_name,
                               const std::filesystem::path& local_path) {
  const std::int64_t release_id = release.value("id", std::int64_t{0});
  if (release_id == 0) {
    throw std::runtime_error("release JSON missing id field");
  }

  const std::string file_data = ReadFile(local_path);
  const std::string url = std::string(github_upload_base) + "/repos/" + repo +
                          "/releases/" + std::to_string(release_id) +
                          "/assets?name=" + UrlEncode(asset_name);

  const auto response =
      HttpRequest("POST", url, GithubHeaders(token, "application/octet-stream"),
                  &file_data);
  if (response.status != 201) {
    throw std::runtime_error("failed to upload asset '" + asset_name +
                             "': HTTP " + std::to_string(response.status) +
                             " " + response.body);
  }
}
#endif

#if defined(LOFI_ENABLE_SFTP)
inline constexpr int default_sftp_connect_timeout_ms = 15000;
inline constexpr int default_sftp_io_timeout_seconds = 60;

struct SftpConnectionArgs {
  std::string host;
  std::string username;
  std::string password;
  int port = 0;
};

struct RemoteEntry {
  std::string name;
  bool is_dir = false;
};

inline SftpConnectionArgs ParseSftpConnectionArgs(std::string_view raw) {
  if (raw.empty()) {
    throw std::runtime_error(
        "connection arguments are required with format "
        "host:username:password:port");
  }

  const auto parts = Split(raw, ':');
  if (parts.size() != 4 ||
      std::any_of(parts.begin(), parts.end(),
                  [](const std::string& part) { return part.empty(); })) {
    throw std::runtime_error(
        "connection arguments must use format host:username:password:port");
  }

  int port = 0;
  try {
    std::size_t consumed = 0;
    port = std::stoi(parts[3], &consumed);
    if (consumed != parts[3].size() || port <= 0 || port > 65535) {
      throw std::out_of_range("invalid port");
    }
  } catch (const std::exception&) {
    throw std::runtime_error("port must be an integer between 1 and 65535");
  }

  return {parts[0], parts[1], parts[2], port};
}

inline SftpConnectionArgs ParseSftpConnectionArgsFromEnv(const char* env_name) {
  const auto raw = GetEnv(env_name);
  if (!raw) {
    throw std::runtime_error(std::string(env_name) + " is required");
  }
  return ParseSftpConnectionArgs(*raw);
}

namespace detail {

inline std::string LastSessionError(LIBSSH2_SESSION* session) {
  char* message = nullptr;
  const int length = libssh2_session_last_error(session, &message, nullptr, 0);
  if (length <= 0 || message == nullptr) {
    return "unknown libssh2 error";
  }
  return std::string(message, static_cast<std::size_t>(length));
}

inline std::string LastSftpError(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp) {
  const auto code = libssh2_sftp_last_error(sftp);
  return LastSessionError(session) + " (SFTP error " + std::to_string(code) +
         ")";
}

inline void SetSocketBlocking(int sock, bool blocking) {
  const int flags = fcntl(sock, F_GETFL, 0);
  if (flags == -1) {
    throw std::runtime_error("failed to read socket flags: " +
                             std::string(std::strerror(errno)));
  }

  const int next_flags =
      blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
  if (fcntl(sock, F_SETFL, next_flags) == -1) {
    throw std::runtime_error("failed to update socket flags: " +
                             std::string(std::strerror(errno)));
  }
}

inline void SetSocketTimeouts(int sock, int io_timeout_seconds) {
  timeval timeout{};
  timeout.tv_sec = io_timeout_seconds;

  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ==
      -1) {
    throw std::runtime_error("failed to set socket read timeout: " +
                             std::string(std::strerror(errno)));
  }
  if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) ==
      -1) {
    throw std::runtime_error("failed to set socket write timeout: " +
                             std::string(std::strerror(errno)));
  }
}

inline void SetSocketTimeouts(int sock) {
  SetSocketTimeouts(sock, default_sftp_io_timeout_seconds);
}

inline bool ConnectWithTimeout(int sock,
                               const sockaddr* addr,
                               socklen_t addr_len,
                               int connect_timeout_ms,
                               int io_timeout_seconds,
                               std::string& last_error) {
  SetSocketBlocking(sock, false);

  if (connect(sock, addr, addr_len) == 0) {
    SetSocketBlocking(sock, true);
    SetSocketTimeouts(sock, io_timeout_seconds);
    return true;
  }

  if (errno != EINPROGRESS) {
    last_error = std::strerror(errno);
    return false;
  }

  pollfd pfd{};
  pfd.fd = sock;
  pfd.events = POLLOUT;

  const int poll_result = poll(&pfd, 1, connect_timeout_ms);
  if (poll_result == 0) {
    last_error =
        "connect timed out after " + std::to_string(connect_timeout_ms) + " ms";
    return false;
  }
  if (poll_result < 0) {
    last_error = std::strerror(errno);
    return false;
  }

  int socket_error = 0;
  socklen_t socket_error_len = sizeof(socket_error);
  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &socket_error,
                 &socket_error_len) == -1) {
    last_error = std::strerror(errno);
    return false;
  }
  if (socket_error != 0) {
    last_error = std::strerror(socket_error);
    return false;
  }

  SetSocketBlocking(sock, true);
  SetSocketTimeouts(sock, io_timeout_seconds);
  return true;
}

inline bool ConnectWithTimeout(int sock,
                               const sockaddr* addr,
                               socklen_t addr_len,
                               std::string& last_error) {
  return ConnectWithTimeout(sock, addr, addr_len,
                            default_sftp_connect_timeout_ms,
                            default_sftp_io_timeout_seconds, last_error);
}

inline int ConnectSocket(const SftpConnectionArgs& args,
                         int connect_timeout_ms,
                         int io_timeout_seconds) {
  addrinfo hints{};
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;

  addrinfo* result = nullptr;
  const std::string port = std::to_string(args.port);
  const int rc = getaddrinfo(args.host.c_str(), port.c_str(), &hints, &result);
  if (rc != 0) {
    throw std::runtime_error("failed to resolve " + args.host + ": " +
                             gai_strerror(rc));
  }

  int sock = -1;
  std::string last_error = "unknown connection error";
  for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == -1) {
      last_error = std::strerror(errno);
      continue;
    }

    if (ConnectWithTimeout(sock, rp->ai_addr, rp->ai_addrlen,
                           connect_timeout_ms, io_timeout_seconds,
                           last_error)) {
      break;
    }

    close(sock);
    sock = -1;
  }

  freeaddrinfo(result);

  if (sock == -1) {
    throw std::runtime_error("failed to connect to " + args.host + ":" + port +
                             ": " + last_error);
  }
  return sock;
}

inline int ConnectSocket(const SftpConnectionArgs& args) {
  return ConnectSocket(args, default_sftp_connect_timeout_ms,
                       default_sftp_io_timeout_seconds);
}

}  // namespace detail

class Libssh2Runtime {
 public:
  Libssh2Runtime() {
    if (const int rc = libssh2_init(0); rc != 0) {
      throw std::runtime_error("libssh2 initialization failed with code " +
                               std::to_string(rc));
    }
  }

  Libssh2Runtime(const Libssh2Runtime&) = delete;
  Libssh2Runtime& operator=(const Libssh2Runtime&) = delete;

  ~Libssh2Runtime() { libssh2_exit(); }
};

class SftpClient {
 public:
  explicit SftpClient(const SftpConnectionArgs& args,
                      int connect_timeout_ms = default_sftp_connect_timeout_ms,
                      int io_timeout_seconds = default_sftp_io_timeout_seconds)
      : sock_(detail::ConnectSocket(args,
                                    connect_timeout_ms,
                                    io_timeout_seconds)) {
    session_ = libssh2_session_init();
    if (session_ == nullptr) {
      throw std::runtime_error("failed to initialize SSH session");
    }

    libssh2_session_set_blocking(session_, 1);

    if (libssh2_session_handshake(session_, sock_) != 0) {
      throw std::runtime_error("SSH handshake failed: " +
                               detail::LastSessionError(session_));
    }

    if (libssh2_userauth_password(session_, args.username.c_str(),
                                  args.password.c_str()) != 0) {
      throw std::runtime_error("SSH authentication failed: " +
                               detail::LastSessionError(session_));
    }

    sftp_ = libssh2_sftp_init(session_);
    if (sftp_ == nullptr) {
      throw std::runtime_error("failed to initialize SFTP session: " +
                               detail::LastSessionError(session_));
    }
  }

  SftpClient(const SftpClient&) = delete;
  SftpClient& operator=(const SftpClient&) = delete;

  ~SftpClient() {
    if (sftp_ != nullptr) {
      libssh2_sftp_shutdown(sftp_);
    }
    if (session_ != nullptr) {
      libssh2_session_disconnect(session_, "Normal Shutdown");
      libssh2_session_free(session_);
    }
    if (sock_ != -1) {
      close(sock_);
    }
  }

  std::optional<std::vector<RemoteEntry>> ListDirectory(
      const std::string& remote_dir) {
    LIBSSH2_SFTP_HANDLE* dir = libssh2_sftp_opendir(sftp_, remote_dir.c_str());
    if (dir == nullptr) {
      return std::nullopt;
    }

    std::vector<RemoteEntry> entries;
    std::array<char, 1024> buffer{};
    LIBSSH2_SFTP_ATTRIBUTES attrs{};
    while (true) {
      const int rc =
          libssh2_sftp_readdir(dir, buffer.data(), buffer.size(), &attrs);
      if (rc > 0) {
        std::string name(buffer.data(), static_cast<std::size_t>(rc));
        if (name != "." && name != "..") {
          const bool is_dir =
              (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) != 0 &&
              LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
          entries.push_back({std::move(name), is_dir});
        }
        continue;
      }
      if (rc == 0) {
        break;
      }

      libssh2_sftp_closedir(dir);
      throw std::runtime_error("failed to read " + remote_dir + ": " +
                               detail::LastSftpError(session_, sftp_));
    }

    libssh2_sftp_closedir(dir);
    std::sort(entries.begin(), entries.end(),
              [](const RemoteEntry& lhs, const RemoteEntry& rhs) {
                return lhs.name < rhs.name;
              });
    return entries;
  }

  void DownloadFile(const std::string& remote_path,
                    const std::filesystem::path& local_path) {
    LIBSSH2_SFTP_HANDLE* remote =
        libssh2_sftp_open(sftp_, remote_path.c_str(), LIBSSH2_FXF_READ, 0);
    if (remote == nullptr) {
      throw std::runtime_error("failed to open " + remote_path + ": " +
                               detail::LastSftpError(session_, sftp_));
    }

    std::ofstream local(local_path, std::ios::binary | std::ios::trunc);
    if (!local) {
      libssh2_sftp_close(remote);
      throw std::runtime_error("failed to open " + local_path.string() +
                               " for writing");
    }

    std::array<char, 16 * 1024> buffer{};
    while (true) {
      const ssize_t rc =
          libssh2_sftp_read(remote, buffer.data(), buffer.size());
      if (rc > 0) {
        local.write(buffer.data(), static_cast<std::streamsize>(rc));
        if (!local) {
          libssh2_sftp_close(remote);
          throw std::runtime_error("failed while writing " +
                                   local_path.string());
        }
        continue;
      }
      if (rc == 0) {
        break;
      }

      libssh2_sftp_close(remote);
      throw std::runtime_error("failed while reading " + remote_path + ": " +
                               detail::LastSftpError(session_, sftp_));
    }

    libssh2_sftp_close(remote);
  }

  void RemoveFile(const std::string& remote_path) {
    if (libssh2_sftp_unlink(sftp_, remote_path.c_str()) != 0) {
      throw std::runtime_error("failed to remove file " + remote_path + ": " +
                               detail::LastSftpError(session_, sftp_));
    }
  }

  void RemoveDirectory(const std::string& remote_path) {
    if (libssh2_sftp_rmdir(sftp_, remote_path.c_str()) != 0) {
      throw std::runtime_error("failed to remove directory " + remote_path +
                               ": " + detail::LastSftpError(session_, sftp_));
    }
  }

 private:
  int sock_ = -1;
  LIBSSH2_SESSION* session_ = nullptr;
  LIBSSH2_SFTP* sftp_ = nullptr;
};
#endif

}  // namespace lofi
