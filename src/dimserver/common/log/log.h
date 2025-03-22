#pragma once

#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <mutex>
#include <map>
#include <set>
#include <functional>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <boost/format.hpp>
#include <cstdarg>

namespace common {

#define MESSAGE_LENGTH  2048
#define LINE_LENGTH     1024
#define MAX_LINE        100000
#define LOG_HEAD_LENGTH 2048

enum class LogLevel {
  TRACE = 0,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  PANIC
};

static const char* LevelToString(LogLevel level) {
  switch (level) {
#define xx(name) case name: return #name;
    xx(LogLevel::TRACE);
    xx(LogLevel::DEBUG);
    xx(LogLevel::INFO);
    xx(LogLevel::WARN);
    xx(LogLevel::ERROR);
    xx(LogLevel::PANIC);
#undef xx
    default:
      return "UNKNOWN";
  }
}

enum class LogRotate {
  ROTATE_SIZE, // By MAX line
  ROTATE_TIME  // By Day
};

class Log {
public:
  Log(const std::string& filename, LogLevel level = LogLevel::INFO,
      LogLevel console_level = LogLevel::WARN);
  ~Log(void);

  template <class T>
  Log& operator<<(T message);

  template <class T>
  int panic(T message);

  template <class T>
  int error(T message);

  template <class T>
  int warn(T message);

  template <class T>
  int info(T message);

  template <class T>
  int debug(T message);

  template <class T>
  int trace(T message);

  int output(const LogLevel level, const char* module, const char* prefix, const char* format, ...);

  int set_console_level(const LogLevel console_level);
  LogLevel get_console_level();

  int set_log_level(const LogLevel log_level);
  LogLevel get_log_level();

  int set_rotate_type(LogRotate rotate_type);
  LogRotate get_rotate_type();

  void set_module(const std::string& modules);
  bool check_output(const LogLevel log_level, const char* module);

  void set_context(std::function<intptr_t()> context);
  intptr_t context_id();

  int rotate(const int year = 0, const int month = 0, const int day = 0);

  std::string get_log_name();

private:
  std::mutex m_mutex;
  std::ofstream m_ofs;
  std::string m_name;
  LogLevel m_level;
  LogLevel m_console_level;

  struct _LogDate {
    int year;
    int month;
    int day;
  };

  _LogDate m_logdate;
  int m_line;
  int m_max_line = MAX_LINE;
  LogRotate m_rotate;

  std::set<std::string> m_modules;

  std::function<intptr_t()> m_context;

private:
  void check_params();

  int rotate_by_size();
  int rotate_by_time(const int year, const int month, const int day);
  int rename_log();

  template <typename T>
  int out(const LogLevel console_level, const LogLevel level, T& message);
};

class Logger {
public:
  Logger();
  virtual ~Logger();

  static int init(const std::string& file, Log** log, LogLevel level = LogLevel::INFO,
    LogLevel console_level = LogLevel::WARN, LogRotate rotate_type = LogRotate::ROTATE_TIME);

  static int init_default(const std::string& file, Log** log, LogLevel level = LogLevel::INFO,
    LogLevel console_level = LogLevel::WARN, LogRotate rotate_type = LogRotate::ROTATE_TIME);
};

#define __FILE_NAME__ (::strrchr(__FILE__, '/') ? ::strrchr(__FILE__, '/') + 1 : __FILE__)

extern Log* g_log;

#define LOG_HEAD(prefix, level) \
  if (g_log) { \
    auto now = std::chrono::system_clock::now(); \
    auto time_t_now = std::chrono::system_clock::to_time_t(now); \
    std::tm tm_now = *std::localtime(&time_t_now); \
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000; \
    std::string head = (boost::format("%04d-%02d-%02d %02d:%02d:%02d.%06d pid:%u tid:%u ctx:%u") \
      % (tm_now.tm_year + 1900) \
      % (tm_now.tm_mon + 1) \
      % tm_now.tm_mday \
      % tm_now.tm_hour \
      % tm_now.tm_min \
      % tm_now.tm_sec \
      % ms.count() \
      % getpid() \
      % std::this_thread::get_id() \
      % g_log->context_id()).str(); \
    \
    g_log->rotate(tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday); \
    std::string prefix_str = (boost::format("[%s %s %s@%s:%d] >> ") \
      % head \
      % LevelToString(level) \
      % __FUNCTION__ \
      % __FILE_NAME__ \
      % (int32_t)__LINE__).str(); \
    std::strncpy(prefix, prefix_str.c_str(), LOG_HEAD_LENGTH - 1); \
    prefix[LOG_HEAD_LENGTH - 1] = '\0'; \
  }

#define LOG_OUTPUT(level, format, ...) \
  do { \
    if (g_log && g_log->check_output(level, __FILE_NAME__)) { \
      char prefix[LOG_HEAD_LENGTH] = { 0 }; \
      LOG_HEAD(prefix, level); \
      g_log->output(level, __FILE_NAME__, prefix, format, ##__VA_ARGS__); \
    } \
  } while (0)

#define LOG_PANIC(format, ...)  LOG_OUTPUT(LogLevel::PANIC, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)  LOG_OUTPUT(LogLevel::ERROR, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)   LOG_OUTPUT(LogLevel::WARN, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)   LOG_OUTPUT(LogLevel::INFO, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)  LOG_OUTPUT(LogLevel::DEBUG, format, ##__VA_ARGS__)
#define LOG_TRACE(format, ...)  LOG_OUTPUT(LogLevel::TRACE, format, ##__VA_ARGS__)

template <typename T>
Log& Log::operator<<(T message) {
  out(m_console_level, m_level, message);
  return *this;
}

template <typename T>
int Log::trace(T message) {
  return out(LogLevel::TRACE, LogLevel::TRACE, message);
}

template <typename T>
int Log::debug(T message) {
  return out(LogLevel::DEBUG, LogLevel::DEBUG, message);
}

template <typename T>
int Log::info(T message) {
  return out(LogLevel::INFO, LogLevel::INFO, message);
}

template <typename T>
int Log::error(T message) {
  return out(LogLevel::ERROR, LogLevel::ERROR, message);
}

template <typename T>
int Log::warn(T message) {
  return out(LogLevel::WARN, LogLevel::WARN, message);
}

template <typename T>
int Log::panic(T message) {
  return out(LogLevel::PAINC, LogLevel::PAINC, message);
}

template <typename T>
int Log::out(const LogLevel console_level, const LogLevel level, T& message) {
  if (console_level < LogLevel::TRACE || console_level > LogLevel::PAINC 
    || level < LogLevel::TRACE || level > LogLevel::PAINC) {
    return -1;
  }

  char prefix[LOG_HEAD_LENGTH] = {0};
  LOG_HEAD(prefix, level);

  if (console_level >= LogLevel::TRACE && console_level <= LogLevel::PAINC) {
    std::cout << LevelToString(console_level) << message << "\n";
  }

  if (level >= LogLevel::TRACE && level <= LogLevel::PAINC) {
    m_ofs << prefix << message << "\n";
    m_ofs.flush();
  }
  
  return 0;
}

} // namespace common