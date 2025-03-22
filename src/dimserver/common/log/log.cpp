#include "log.h"
#include <cassert>
#include <cstdarg>
#include <new>
#include <filesystem>
#include <unistd.h>
#include <boost/algorithm/string.hpp>

namespace common {

Log* g_log = nullptr;

Log::Log(const std::string& filename, const LogLevel level,
  const LogLevel console_level)
  : m_name(filename), m_level(level), m_console_level(console_level) {

  m_logdate.year = -1;
  m_logdate.month = -1;
  m_logdate.day = -1;

  m_line = 0;
  m_max_line = MAX_LINE;

  m_rotate = LogRotate::ROTATE_TIME;

  m_context = []() { return 0; };

  check_params();
}

Log::~Log() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_ofs.is_open()) {
    m_ofs.close();
  }
}

void Log::check_params() {
  assert(!m_name.empty());
  assert(m_level >= LogLevel::TRACE && m_level <= LogLevel::PANIC);
  assert(m_console_level >= LogLevel::TRACE && m_console_level <= LogLevel::PANIC);
  assert(m_rotate == LogRotate::ROTATE_SIZE || m_rotate == LogRotate::ROTATE_TIME);
  assert(m_max_line > 0);
  return;
}


bool Log::check_output(const LogLevel level, const char* module) {
  return level >= m_console_level ||
    level >= m_level ||
    m_modules.find(module) != m_modules.end();
}

int Log::output(const LogLevel level, const char* module, const char* prefix, const char* format, ...) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  va_list args;
  char message[MESSAGE_LENGTH] = {0};

  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  if (level >= m_console_level) {
    std::cout << message << "\n";
  } else if (m_modules.find(module) != m_modules.end()) {
    std::cout << message << "\n";
  }

  if (level >= m_level ) {
    m_ofs << prefix << message << "\n";
    m_ofs.flush();
    m_line++;
  } else if (m_modules.find(module) != m_modules.end()) {
    m_ofs << prefix << message << "\n";
    m_ofs.flush();
    m_line++;
  }
  return 0;
}

int Log::set_console_level(const LogLevel console_level) {
  if (console_level < LogLevel::TRACE || console_level > LogLevel::PANIC) {
    return -1;
  }
  m_console_level = console_level;
  return 0;
}

LogLevel Log::get_console_level() {
  return m_console_level;
}

int Log::set_log_level(const LogLevel log_level) {
  if (log_level < LogLevel::TRACE || log_level > LogLevel::PANIC) { 
    return -1;
  }
  m_level = log_level;
  return 0;
}

LogLevel Log::get_log_level() {
  return m_level;
}

int Log::set_rotate_type(const LogRotate rotate_type) {
  if (rotate_type != LogRotate::ROTATE_SIZE && rotate_type != LogRotate::ROTATE_TIME) {
    return -1;
  }
  m_rotate = rotate_type;
  return 0;
}

LogRotate Log::get_rotate_type() {
  return m_rotate;
}

void Log::set_module(const std::string& modules) {
  if (modules.empty()) {
    m_modules.clear();
    return;
  }
  // 分割字符串使用boost分割
  boost::split(m_modules, modules, boost::is_any_of(","));
}

void Log::set_context(std::function<intptr_t()> context) {
  if (context) {
    m_context = context;
  } else {
    m_context = []() { return 0; };
  }
}

intptr_t Log::context_id() {
  return m_context();
}

std::string Log::get_log_name() {
  return m_name;
}

int Log::rotate(const int year, const int month, const int day) {
  if (m_rotate == LogRotate::ROTATE_SIZE) {
    return rotate_by_size();
  } else if (m_rotate == LogRotate::ROTATE_TIME) {
    return rotate_by_time(year, month, day);
  }
  return -1;
}


Logger::Logger() {

}

Logger::~Logger() {

}

int Logger::init(const std::string& file,
                 Log** log,
                 LogLevel level, 
                 LogLevel console_level, 
                 LogRotate rotate_type) {
  *log = new (std::nothrow) Log(file, level, console_level);
  if (*log == nullptr) {
    std::cerr << "Logger::init() error: new Log failed" << "\n";
    return -1;
  }
  (*log)->set_rotate_type(rotate_type);
  g_log = *log;
  return 0;
}

int Logger::init_default(const std::string& file,
                         Log** log,
                         LogLevel level, 
                         LogLevel console_level, 
                         LogRotate rotate_type) {
  if (g_log) {
    LOG_INFO("Logger::init_default() error: g_log already exists");
    return -1;
  }
  return init(file, log, level, console_level, rotate_type);
}

#define MAX_LOG_NUM 999

int Log::rename_log() {
  int log_index = 1;
  int max_log_index = 0;

  while (log_index < MAX_LOG_NUM) {
    std::string m_name = m_name + "." + std::to_string(log_index);
    int rc = access(m_name.c_str(), R_OK);
    if (rc) {
      break;
    }

    max_log_index = log_index;
    log_index++;
  }

  auto pad_to_digits = [](int number, uint32_t digits) {
    std::string result = std::to_string(number);
    while (result.size() < digits) {
      result = "0" + result;
    }
    return result;
  };

  if (log_index == MAX_LOG_NUM) {
    std::string old_log = m_name + "." + pad_to_digits(log_index, 3);
    remove(old_log.c_str());
  }

  log_index = max_log_index;
  while (log_index > 0) {
    std::string old_log = m_name + "." + pad_to_digits(log_index, 3);
    std::string new_log = m_name + "." + pad_to_digits(log_index + 1, 3);
    int rc = rename(old_log.c_str(), new_log.c_str());
    if (rc) {
      break;
    }
    log_index--;
  }
  return 0;
}

int Log::rotate_by_size() {
  if (m_line < 0) {
    m_ofs.open(m_name.c_str(), std::ios_base::out | std::ios_base::app);
    m_line = 0;
    return 0;
  } else if (m_line >= 0 && m_line < m_max_line) {
    return 0;
  } else {
    int rc = rename_log();
    if (rc) {
      return 0;
    }

    if (m_ofs.is_open()) {
      m_ofs.close();
    }

    m_ofs.open(m_name.c_str(), std::ios_base::out | std::ios_base::app);
    m_line = 0;
    return 0;
  }
}

int Log::rotate_by_time(const int year, const int month, const int day) {
  if (m_logdate.year == year && m_logdate.month == month && m_logdate.day == day) {
    return 0;
  }

  char date_str[16] = {0};
  snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", year, month, day);
  m_name = m_name + "." + date_str;

  if (m_ofs.is_open()) {
    m_ofs.close();
  }

  m_ofs.open(m_name.c_str(), std::ios_base::out | std::ios_base::app);
  if (m_ofs.good()) {
    m_line = 0;
    m_logdate.year = year;
    m_logdate.month = month;
    m_logdate.day = day;
    return 0;
  }
  return -1;
}


} // namespace common