#include "common/log/log.h"
#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <filesystem>
#include <regex>
#include <iomanip>

using namespace common;

class LogTest : public testing::Test {
protected:
  void SetUp() override {
    // 清理之前的测试日志文件
    cleanLogFiles("logs");
    
    // 初始化日志系统
    InitLogger("dimdb", LogLevel::TRACE, LogLevel::INFO);
  }
  
  void TearDown() override {
    // 清理测试产生的日志文件
    // cleanLogFiles("logs");
  }
  
  void cleanLogFiles(const std::string& log_dir) {
    std::filesystem::path dir(log_dir);
    if (std::filesystem::exists(dir)) {
      for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().string().find(".log") != std::string::npos) {
          std::filesystem::remove(entry.path());
        }
      }
    }
  }
  
  // 读取最后N行日志
  std::vector<std::string> readLastNLines(const std::string& filename, int n) {
    std::vector<std::string> lines;
    std::filesystem::path file_path(filename);
    std::ifstream file(file_path);
    if (!file.is_open()) {
      return lines;
    }
    
    std::string line;
    while (std::getline(file, line)) {
      lines.push_back(line);
      if ((int)lines.size() > n) {
        lines.erase(lines.begin());
      }
    }
    return lines;
  }
  
  // 获取当前日志文件名
  std::string getCurrentLogFile(const std::string& log_dir = "logs", const std::string& log_pattern = "common.log") {
    std::filesystem::path dir(log_dir);
    std::string latest_file;
    std::filesystem::file_time_type latest_time;
    bool found = false;
    
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (entry.path().string().find(log_pattern) != std::string::npos) {
        if (!found || std::filesystem::last_write_time(entry.path()) > latest_time) {
          latest_file = entry.path().string();
          latest_time = std::filesystem::last_write_time(entry.path());
          found = true;
        }
      }
    }
    return latest_file;
  }
};

// 测试基本日志输出
TEST_F(LogTest, BasicLogging) {
  g_log->clearAppenders();
  InitLogger("dimdb", LogLevel::TRACE, LogLevel::INFO);

  LOG_TRACE("This is a trace message");
  LOG_DEBUG("This is a debug message");
  LOG_INFO("This is an info message");
  LOG_WARN("This is a warning message");
  LOG_ERROR("This is an error message");
  LOG_PANIC("This is a panic message");
  
  auto lines = readLastNLines(getCurrentLogFile("logs"), 4);
  ASSERT_EQ(lines.size(), 4);
  
  // 验证每条日志的格式
  std::regex log_pattern(R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{6} pid:\d+ tid:\d+ ctx:\d+ \w+: [\w:]+@[\w.]+:\d+\] >> .*)");
  for (const auto& line : lines) {
    EXPECT_TRUE(std::regex_match(line, log_pattern));
  }
}

// 测试多线程日志
TEST_F(LogTest, MultiThreadLogging) {
  const int thread_count = 4;
  const int logs_per_thread = 100;
  std::vector<std::thread> threads;
  
  for (int i = 0; i < thread_count; ++i) {
    threads.emplace_back([i, logs_per_thread]() {
      for (int j = 0; j < logs_per_thread; ++j) {
        LOG_INFO("Thread %d log %d", i, j);
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  // 验证日志文件中的日志数量
  auto lines = readLastNLines(getCurrentLogFile("logs"), thread_count * logs_per_thread);
  EXPECT_EQ(lines.size(), thread_count * logs_per_thread);
}

// 测试日志级别过滤
TEST_F(LogTest, LogLevelFiltering) {
  // 重新初始化日志系统，设置控制台级别为WARN，文件级别为INFO
  g_log->clearAppenders();
  InitLogger("dimdb", LogLevel::WARN);
  
  // 格式化输出测试
  LOG_TRACE("%s", "Trace message");  // 不应该记录
  LOG_DEBUG("%s", "Debug message");  // 不应该记录
  LOG_INFO("%s", "Info message");    // 应该记录
  LOG_WARN("%s", "Warn message");    // 应该记录
  LOG_ERROR("%s", "Error message");  // 应该记录
  
  // 流式输出测试
  LOG_TRACE_STREAM << "Trace stream";  // 不应该记录
  LOG_DEBUG_STREAM << "Debug stream";  // 不应该记录
  LOG_INFO_STREAM << "Info stream";    // 应该记录
  LOG_WARN_STREAM << "Warn stream";    // 应该记录
  LOG_ERROR_STREAM << "Error stream";  // 应该记录
  
  auto lines = readLastNLines(getCurrentLogFile("logs"), 6);
  EXPECT_EQ(lines.size(), 6);  // 应该只有6条日志（INFO及以上）
  
  if (lines.size() >= 6) {
    // 验证格式化输出
    EXPECT_TRUE(lines[0].find("Info message") != std::string::npos);
    EXPECT_TRUE(lines[1].find("Warn message") != std::string::npos);
    EXPECT_TRUE(lines[2].find("Error message") != std::string::npos);
    // 验证流式输出
    EXPECT_TRUE(lines[3].find("Info stream") != std::string::npos);
    EXPECT_TRUE(lines[4].find("Warn stream") != std::string::npos);
    EXPECT_TRUE(lines[5].find("Error stream") != std::string::npos);
  }
}

// 测试日志文件轮转（按时间）
TEST_F(LogTest, TimeBasedRotation) {
  // 获取文件日志记录器
  auto file_appender = std::make_shared<FileLogAppender>("logs/time_test.log", LogRotate::ROTATE_TIME);
  file_appender->setLevel(LogLevel::INFO);
  g_log->addAppender(file_appender);
  
  // 启用测试模式并设置初始时间
  file_appender->setTestMode(true);
  time_t base_time = std::time(nullptr);
  file_appender->setTestTime(base_time);
  
  // 写入第一条日志
  LOG_INFO("First log message");
  std::string first_log_file = getCurrentLogFile("logs", "time_test.log");
  
  // 设置时间为第二天
  file_appender->setTestTime(base_time + 24 * 60 * 60);
  
  // 写入第二条日志
  LOG_INFO("Second log message");
  std::string second_log_file = getCurrentLogFile("logs", "time_test.log");
  
  // 验证是否创建了新的日志文件
  EXPECT_NE(first_log_file, second_log_file);
  
  // 验证文件名中包含了正确的日期
  std::regex date_pattern(R"(.*\.\d{8})");
  EXPECT_TRUE(std::regex_match(first_log_file, date_pattern));
  EXPECT_TRUE(std::regex_match(second_log_file, date_pattern));
}

// 测试日志文件轮转（按大小）
TEST_F(LogTest, SizeBasedRotation) {
  // 创建一个小容量的文件日志记录器进行测试
  auto file_appender = std::make_shared<FileLogAppender>(
    "logs/size_test.log", 
    LogRotate::ROTATE_SIZE,
    1024  // 1KB
  );
  file_appender->setLevel(LogLevel::INFO);
  g_log->addAppender(file_appender);
  
  std::string long_message(100, 'x');
  
  // 使用格式化输出写入一半数据
  for (int i = 0; i < 10; ++i) {
    LOG_INFO("%s", long_message.c_str());
  }
  
  // 使用流式输出写入另一半数据
  for (int i = 0; i < 10; ++i) {
    LOG_INFO_STREAM << long_message;
  }
  
  int log_file_count = 0;
  for (const auto& entry : std::filesystem::directory_iterator("logs")) {
    if (entry.path().string().find("size_test.log") != std::string::npos) {
      log_file_count++;
    }
  }
  EXPECT_GT(log_file_count, 1);
}

// 测试日志格式化
TEST_F(LogTest, LogFormatting) {
  LOG_INFO_STREAM << "Test message with number " << 42 << " and string " << "hello";
  
  auto lines = readLastNLines(getCurrentLogFile("logs"), 1);
  ASSERT_EQ(lines.size(), 1);
  
  // 验证格式化后的消息
  EXPECT_TRUE(lines[0].find("Test message with number 42 and string hello") != std::string::npos);
}

// 测试格式化日志输出
TEST_F(LogTest, FormatLogging) {
  g_log->clearAppenders();
  InitLogger("dimdb", LogLevel::TRACE, LogLevel::TRACE);

  LOG_TRACE("%s", "This is a trace message");
  LOG_DEBUG("This is a debug message with number %d", 42);
  LOG_INFO("Info message with string %s and number %d", "test", 100);
  LOG_WARN("Warning message with float %.2f", 3.14);
  LOG_ERROR("Error code: %d, message: %s", 404, "Not Found");
  LOG_PANIC("System panic with code 0x%x", 0xDEAD);
  
  auto lines = readLastNLines(getCurrentLogFile("logs"), 6);
  ASSERT_EQ(lines.size(), 6);
  
  EXPECT_TRUE(lines[0].find("This is a trace message") != std::string::npos);
  EXPECT_TRUE(lines[1].find("This is a debug message with number 42") != std::string::npos);
  EXPECT_TRUE(lines[2].find("Info message with string test and number 100") != std::string::npos);
  EXPECT_TRUE(lines[3].find("Warning message with float 3.14") != std::string::npos);
  EXPECT_TRUE(lines[4].find("Error code: 404, message: Not Found") != std::string::npos);
  EXPECT_TRUE(lines[5].find("System panic with code 0xdead") != std::string::npos);
}

// 测试流式日志输出
TEST_F(LogTest, StreamLogging) {

  g_log->clearAppenders();
  InitLogger("dimdb", LogLevel::TRACE, LogLevel::TRACE);

  LOG_TRACE_STREAM << "This is a trace message";
  LOG_DEBUG_STREAM << "Debug with number: " << 42;
  LOG_INFO_STREAM << "Info with multiple " << "string " << "concatenation";
  LOG_WARN_STREAM << "Warning with bool: " << true << " and float: " << 3.14;
  LOG_ERROR_STREAM << "Error with hex: 0x" << std::hex << 255;
  LOG_PANIC_STREAM << "Panic with scientific: " << std::scientific << 0.000123;
  
  auto lines = readLastNLines(getCurrentLogFile("logs"), 6);
  ASSERT_EQ(lines.size(), 6);
  
  EXPECT_TRUE(lines[0].find("This is a trace message") != std::string::npos);
  EXPECT_TRUE(lines[1].find("Debug with number: 42") != std::string::npos);
  EXPECT_TRUE(lines[2].find("Info with multiple string concatenation") != std::string::npos);
  EXPECT_TRUE(lines[3].find("Warning with bool: 1 and float: 3.14") != std::string::npos);
  EXPECT_TRUE(lines[4].find("Error with hex: 0xff") != std::string::npos);
  EXPECT_TRUE(lines[5].find("Panic with scientific: 1.230000e-04") != std::string::npos);
} 