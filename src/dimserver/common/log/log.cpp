#include "log.h"
#include <iostream>
#include <functional>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <chrono>
#include <ctime>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <filesystem>
#include <regex>

namespace common {

// 全局日志对象定义
Logger::ptr g_log;

/**
 * @brief 初始化日志系统
 * @param name 日志名称
 * @param console_level 控制台日志级别
 * @param level 文件日志级别
 */
void InitLogger(const std::string& name,
				LogLevel console_level,LogLevel level) {
	g_log = LogManager::getInstance().getLogger(name);
	g_log->setLevel(LogLevel::TRACE);
	
	// 添加控制台输出，使用简单格式
	auto console_appender = std::make_shared<StdoutLogAppender>();
	console_appender->setLevel(console_level);
	auto console_formatter = std::make_shared<LogFormatter>("[%L] >> %m");
	console_appender->setFormatter(console_formatter);
	g_log->addAppender(console_appender);
	
	// 添加文件输出，使用详细格式
	auto file_appender = 
		std::make_shared<FileLogAppender>("logs/common.log", LogRotate::ROTATE_TIME);
	file_appender->setLevel(level);
	auto file_formatter = std::make_shared<LogFormatter>
					("[%Y-%m-%d %H:%M:%S.%f pid:%P tid:%T ctx:%C %L: %F@%f:%l] >> %m");
	file_appender->setFormatter(file_formatter);
	g_log->addAppender(file_appender);
}

// 获取当前线程ID
static uint64_t GetThreadId() {
#if defined(__linux__)
  return syscall(SYS_gettid);
#else
  std::stringstream ss;
  ss << std::this_thread::get_id();
  uint64_t id;
  ss >> id;
  return id;
#endif
}

// 获取进程ID
static uint64_t GetPid() {
	return static_cast<uint64_t>(::getpid());
}

// 获取上下文ID (这里简单返回0，实际中可根据需求实现)
static uint64_t GetContextId() {
	return 0;
}

// 获取当前时间的字符串表示
static std::string GetCurrentTimeStr() {
  auto now = std::chrono::system_clock::now();
  auto time_point = std::chrono::system_clock::to_time_t(now);
  auto duration = now.time_since_epoch();
  auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000000;
  
  std::tm tm_time;
  localtime_r(&time_point, &tm_time);
  
  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_time);
  
  std::string result(buffer);
  char micro_buffer[10];
  snprintf(micro_buffer, sizeof(micro_buffer), ".%06ld", micros);
  result += micro_buffer;
  
  return result;
}

// LogEvent 实现
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel level, const char* file, int32_t line, const char* func)
	: m_logger(logger), 
		m_level(level),
		m_file(file),
		m_line(line),
		m_func(func),
		m_pid(GetPid()),
		m_tid(GetThreadId()),
		m_ctx([]() { return GetContextId(); }),
		m_time(GetCurrentTimeStr()) {
	// 处理文件路径，只保留文件名
	if (m_file.find_last_of('/') != std::string::npos) {
		m_file = m_file.substr(m_file.find_last_of('/') + 1);
	}
}

// LogFormatter 实现
LogFormatter::LogFormatter(const std::string& pattern)
	: m_pattern(pattern) {
}

std::string LogFormatter::format(std::shared_ptr<LogEvent> event) {
	std::string result = m_pattern;
	
	// 替换时间相关的占位符 (%Y-%m-%d %H:%M:%S.%f)
	std::regex time_regex("%[YmdHMSf]");
	std::string time_str = event->getTime();
	result = std::regex_replace(result, std::regex("%Y-%m-%d %H:%M:%S.%f"), time_str);
	
	// 替换其他占位符
	result = std::regex_replace(result, std::regex("%P"), std::to_string(event->getPid()));
	result = std::regex_replace(result, std::regex("%T"), std::to_string(event->getTid()));
	result = std::regex_replace(result, std::regex("%C"), std::to_string(event->getCtx()));
	result = std::regex_replace(result, std::regex("%L"), LogLevelToString(event->getLevel()));
	result = std::regex_replace(result, std::regex("%F"), event->getFunc());
	result = std::regex_replace(result, std::regex("%f"), event->getFile());
	result = std::regex_replace(result, std::regex("%l"), std::to_string(event->getLine()));
	result = std::regex_replace(result, std::regex("%m"), event->getContent());
	
	return result;
}

// StdoutLogAppender 实现
void StdoutLogAppender::log(std::shared_ptr<LogEvent> event) {
	if (event->getLevel() < m_level) {
		return;
	}
	
	std::lock_guard<std::mutex> lock(m_mutex);
	std::cout << m_formatter->format(event) << std::endl;
}

// FileLogAppender 实现
FileLogAppender::FileLogAppender(const std::string& filename, LogRotate rotate, size_t max_size)
	: m_filename(filename), m_rotate(rotate), m_maxSize(max_size) {
	createNewFile();
	m_lastRotateTime = std::time(nullptr);
}

bool FileLogAppender::reopen() {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_filestream) {
		m_filestream.close();
	}
	
	m_filestream.open(m_filename, std::ios::app);
	return m_filestream.is_open();
}

void FileLogAppender::log(std::shared_ptr<LogEvent> event) {
	if (event->getLevel() < m_level) {
		return;
	}
	
	std::lock_guard<std::mutex> lock(m_mutex);
	
	if (checkRotate()) {
		createNewFile();
	}
	
	// 检查文件是否打开，如果没有打开则尝试重新打开
	if (!m_filestream.is_open() && !reopen()) {
		std::cerr << "Failed to reopen log file: " << m_filename << std::endl;
		return;
	}
	
	std::string log_line = m_formatter->format(event);
	m_filestream << log_line << std::endl;
	m_currentSize += log_line.size() + 1; // +1 for newline
}

bool FileLogAppender::checkRotate() {
	if (m_rotate == LogRotate::ROTATE_SIZE) {
		return m_currentSize >= m_maxSize;
	} else if (m_rotate == LogRotate::ROTATE_TIME) {
		time_t now = m_testMode ? m_testTime : std::time(nullptr);
		std::tm tm_now, tm_last;
		localtime_r(&now, &tm_now);
		localtime_r(&m_lastRotateTime, &tm_last);
		return tm_now.tm_mday != tm_last.tm_mday ||
						tm_now.tm_mon != tm_last.tm_mon ||
						tm_now.tm_year != tm_last.tm_year;
	}
	return false;
}

void FileLogAppender::createNewFile() {
	if (m_filestream) {
		m_filestream.close();
	}
	
	// 首先确保原始目录存在
	std::filesystem::path original_path(m_filename);
	if (!std::filesystem::exists(original_path.parent_path())) {
		std::filesystem::create_directories(original_path.parent_path());
	}
	
	std::string filename = m_filename;
	if (m_rotate == LogRotate::ROTATE_TIME) {
		time_t now = m_testMode ? m_testTime : std::time(nullptr);
		std::tm tm_now;
		localtime_r(&now, &tm_now);
		
		char time_buffer[32];
		std::strftime(time_buffer, sizeof(time_buffer), ".%Y%m%d", &tm_now);
		
		// 直接添加时间戳到文件名末尾
		filename += time_buffer;
		
		m_lastRotateTime = now;
	} else if (m_rotate == LogRotate::ROTATE_SIZE) {
		// 按大小轮换时的处理
		int index = 0;
		// 查找可用的序号
		while (std::filesystem::exists(filename + "." + std::to_string(index))) {
			++index;
		}
		
		filename += "." + std::to_string(index);
	}
	
	m_filestream.open(filename, std::ios::app);
	if (!m_filestream) {
		std::cerr << "Failed to open log file: " << filename << std::endl;
	}
	
	m_currentSize = 0;
}

// Logger 实现
Logger::Logger(const std::string& name)
	: m_name(name) {
}

void Logger::log(LogLevel level, const std::shared_ptr<LogEvent>& event) {
	if (level < m_level) {
		return;
	}
	
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& appender : m_appenders) {
		appender->log(event);
	}
}

void Logger::addAppender(LogAppender::ptr appender) {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!appender->getFormatter()) {
		appender->setFormatter(std::make_shared<LogFormatter>());
	}
	m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
		if (*it == appender) {
			m_appenders.erase(it);
			break;
		}
	}
}

void Logger::clearAppenders() {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_appenders.clear();
}

// LogManager 实现
LogManager::LogManager() {
	m_root = std::make_shared<Logger>();
	
	// 默认添加控制台输出
	auto console_appender = std::make_shared<StdoutLogAppender>();
	m_root->addAppender(console_appender);
	
	m_loggers["root"] = m_root;
}

Logger::ptr LogManager::getLogger(const std::string& name) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_loggers.find(name);
	if (it != m_loggers.end()) {
		return it->second;
	}
	
	auto logger = std::make_shared<Logger>(name);
	// // 默认继承root的输出器
	// for (auto& appender : m_root->getAppenders()) {
	// 	logger->addAppender(appender);
	// }
	
	m_loggers[name] = logger;
	return logger;
}

} // namespace common
