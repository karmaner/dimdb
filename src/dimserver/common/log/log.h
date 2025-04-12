#pragma once

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>
#include <map>
#include <functional>
#include <ctime>
#include <chrono>
#include <iomanip>

namespace common {

// 定义日志等级
enum class LogLevel {
  TRACE = 0,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  PANIC
};

// 将LogLevel转换为字符串
inline const char* LogLevelToString(LogLevel level) {
  switch (level) {
#define xx(name) case LogLevel::name: return #name;
    xx(TRACE)
    xx(DEBUG)
    xx(INFO)
    xx(WARN)
    xx(ERROR)
    xx(PANIC)
#undef xx
    default:
      return "UNKNOW";
  }
}

// 前向声明
class Logger;

// 日志事件类，用于封装日志信息
class LogEvent {
public:
  LogEvent(std::shared_ptr<Logger> logger, 
          LogLevel level, const char* file, int32_t line, const char* func);
  
  std::stringstream& getSS() { return m_ss; }
  
  std::string getTime() const { return m_time; }
  std::string getFile() const { return m_file; }
  int32_t getLine() const { return m_line; }
  std::string getFunc() const { return m_func; }
  uint64_t getPid() const { return m_pid; }
  uint64_t getTid() const { return m_tid; }
  intptr_t getCtx() const { return m_ctx(); }
  LogLevel getLevel() const { return m_level; }
  std::shared_ptr<Logger> getLogger() const { return m_logger; }
  std::string getContent() const { return m_ss.str(); }

private:
  std::shared_ptr<Logger> m_logger;
  LogLevel m_level;
  std::string m_file;
  int32_t m_line = 0;
  std::string m_func;
  uint64_t m_pid = 0;
  uint64_t m_tid = 0;
  std::function<intptr_t()> m_ctx = []() { return 0; };
  std::string m_time;
  std::stringstream m_ss;
};

// 日志格式化器
class LogFormatter {
public:
  using ptr = std::shared_ptr<LogFormatter>;
  /*
  * @brief 日志格式化器
  * @param pattern 日志格式化字符串
  * 格式化字符串说明：
  * %Y-%m-%d %H:%M:%S.%f 表示时间，格式为：2025-03-28 10:00:00.000
  * pid:%P tid:%T ctx:%C %L: %F@%f:%l 表示日志信息，格式为：pid:123 tid:456 ctx:789 L:INFO F:main@log.cpp:123
  * %m 表示日志内容
  */
  LogFormatter(const std::string& pattern = 
          "[%Y-%m-%d %H:%M:%S.%f pid:%P tid:%T ctx:%C %L: %F@%f:%l] >> %m");
  
  std::string format(std::shared_ptr<LogEvent> event);

private:
  std::string m_pattern;
};

// 日志输出目标的基类
class LogAppender {
public:
  using ptr = std::shared_ptr<LogAppender>;
  
  virtual ~LogAppender() {}
  
  virtual void log(std::shared_ptr<LogEvent> event) = 0;
  
  void setFormatter(LogFormatter::ptr formatter) { m_formatter = formatter; }
  LogFormatter::ptr getFormatter() const { return m_formatter; }
  
  void setLevel(LogLevel level) { m_level = level; }
  LogLevel getLevel() const { return m_level; }

protected:
  LogLevel m_level = LogLevel::INFO;
  LogFormatter::ptr m_formatter;
  std::mutex m_mutex;
};

// 控制台输出
class StdoutLogAppender : public LogAppender {
public:
  using ptr = std::shared_ptr<StdoutLogAppender>;
  
  void log(std::shared_ptr<LogEvent> event) override;
};

// 文件日志输出
enum class LogRotate {
  ROTATE_SIZE, // By MAX line
  ROTATE_TIME  // By Day
};

class FileLogAppender : public LogAppender {
public:
  using ptr = std::shared_ptr<FileLogAppender>;
  
  FileLogAppender(const std::string& filename, 
                  LogRotate rotate = LogRotate::ROTATE_TIME, 
                  size_t max_size = 10 * 1024 * 1024); // 默认10MB
  
  void log(std::shared_ptr<LogEvent> event) override;
  
  bool reopen();

  // 用于测试的时间注入功能
  void setTestTime(time_t time) { m_testTime = time; }
  time_t getTestTime() const { return m_testTime; }
  bool isTestMode() const { return m_testMode; }
  void setTestMode(bool mode) { m_testMode = mode; }

private:
  bool checkRotate();
  void createNewFile();

private:
  std::string m_filename;
  std::ofstream m_filestream;
  LogRotate m_rotate;
  size_t m_maxSize;
  size_t m_currentSize = 0;
  time_t m_lastRotateTime = 0;
  
  // 用于测试的时间注入
  bool m_testMode = false;
  time_t m_testTime = 0;
};

// 日志器
class Logger : public std::enable_shared_from_this<Logger> {
public:
  using ptr = std::shared_ptr<Logger>;
  
  Logger(const std::string& name = "system");
  
  void log(LogLevel level, const std::shared_ptr<LogEvent>& event);
  
  void addAppender(LogAppender::ptr appender);
  void delAppender(LogAppender::ptr appender);
  void clearAppenders();
  
  LogLevel getLevel() const { return m_level; }
  void setLevel(LogLevel level) { m_level = level; }

  const std::string& getName() const { return m_name; }
  
  const std::vector<LogAppender::ptr>& getAppenders() const { return m_appenders; }

private:
  std::string m_name;
  LogLevel m_level = LogLevel::INFO;
  std::vector<LogAppender::ptr> m_appenders;
  std::mutex m_mutex;
};

// 日志管理器，单例模式
class LogManager {
public:
  static LogManager& getInstance() {
    static LogManager instance;
    return instance;
  }
  
  Logger::ptr getLogger(const std::string& name);
  Logger::ptr getRoot() const { return m_root; }

private:
  LogManager();
  ~LogManager() = default;
  LogManager(const LogManager&) = delete;
  LogManager& operator=(const LogManager&) = delete;

private:
  std::mutex m_mutex;
  std::map<std::string, Logger::ptr> m_loggers;
  Logger::ptr m_root;
};

// 全局日志对象
extern Logger::ptr g_log;

/*
* @brief 初始化全局日志对象
* @param name 日志对象名称
* @param console_level 控制台日志级别
* @param level 文件日志级别
*/
void InitLogger(const std::string& name = "system", 
  LogLevel console_level = LogLevel::WARN, LogLevel level = LogLevel::INFO);


// 格式化日志宏定义
#define LOG_LEVEL(level, fmt, ...) \
  if(common::g_log && common::g_log->getLevel() <= level) { \
    char buf[2048]; \
    snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
    common::LogEventWrapper(std::make_shared<common::LogEvent>(common::g_log, level, __FILE__, __LINE__, __func__)).getSS() << buf; \
  }

#define LOG_TRACE(fmt, ...) LOG_LEVEL(common::LogLevel::TRACE, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG_LEVEL(common::LogLevel::DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  LOG_LEVEL(common::LogLevel::INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG_LEVEL(common::LogLevel::WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG_LEVEL(common::LogLevel::ERROR, fmt, ##__VA_ARGS__)
#define LOG_PANIC(fmt, ...) LOG_LEVEL(common::LogLevel::PANIC, fmt, ##__VA_ARGS__)

// 流式日志宏定义
#define LOG_LEVEL_STREAM(level) \
    if(common::g_log && common::g_log->getLevel() <= level) \
        common::LogEventWrapper(std::make_shared<common::LogEvent>(common::g_log, level, __FILE__, __LINE__, __func__)).getSS()

#define LOG_TRACE_STREAM LOG_LEVEL_STREAM(common::LogLevel::TRACE)
#define LOG_DEBUG_STREAM LOG_LEVEL_STREAM(common::LogLevel::DEBUG)
#define LOG_INFO_STREAM  LOG_LEVEL_STREAM(common::LogLevel::INFO)
#define LOG_WARN_STREAM  LOG_LEVEL_STREAM(common::LogLevel::WARN)
#define LOG_ERROR_STREAM LOG_LEVEL_STREAM(common::LogLevel::ERROR)
#define LOG_PANIC_STREAM LOG_LEVEL_STREAM(common::LogLevel::PANIC)

// LogEventWrapper类，用于自动输出日志

class LogEventWrapper {
public:
  LogEventWrapper(std::shared_ptr<LogEvent> event)
    : m_event(event) {}
  
  ~LogEventWrapper() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
  }
  
  std::stringstream& getSS() {
    return m_event->getSS();
  }

private:
  std::shared_ptr<LogEvent> m_event;
};

/**
 * @brief 获取当前线程的调用栈信息
 * @details 该函数通过backtrace和backtrace_symbols获取当前线程的调用栈，
 *          并以十六进制地址格式输出。默认情况下只输出地址，不解析符号信息。
 *          只有在定义了SYMBOLS_SUPPORT宏的情况下才会解析符号。
 *          调用栈格式为："0x地址1 符号1 0x地址2 符号2 ..."或者在没有符号时："0x地址1 0x地址2 ..."
 *          使用thread_local存储以提高性能，避免频繁内存分配。
 *          支持最多100帧的调用栈，输出缓冲区大小为8192字节。
 * @return 格式化后的调用栈字符串
 */
std::string stacktrace();

} // namespace common
