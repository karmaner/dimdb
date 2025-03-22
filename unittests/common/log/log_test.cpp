#include <gtest/gtest.h>
#include "common/log/log.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace common {
namespace test {

class LogTest : public ::testing::Test {
protected:
  void SetUp() override {
    // 清理之前的测试日志文件
    std::filesystem::remove_all("./test_logs");
    std::filesystem::create_directory("./test_logs");
    
    // 在每个测试开始时初始化日志系统
    Logger::init_default("./test_logs/test.log", &g_log, LogLevel::TRACE, LogLevel::INFO);
  }

  void TearDown() override {
    // 清理测试后的状态，置空 g_log
    g_log = nullptr;
  }

  // 辅助函数：读取日志文件内容
  std::string ReadLogFile(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
      return "";
    }
    
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
  }
};

// 测试日志初始化
TEST_F(LogTest, Initialization) {
  auto file = g_log->get_log_name();
  
  EXPECT_EQ(ret, 0);
  EXPECT_NE(g_log, nullptr);
  EXPECT_EQ(g_log->get_log_level(), LogLevel::INFO);
  EXPECT_EQ(g_log->get_console_level(), LogLevel::WARN);
  EXPECT_EQ(file, "./test_logs/test_init.log");
}

// 测试全局日志实例
TEST_F(LogTest, GlobalLogger) {
  EXPECT_EQ(g_log, nullptr);
  
  int ret = Logger::init_default("./test_logs/test_global.log", &g_log);
  EXPECT_EQ(ret, 0);
  EXPECT_NE(g_log, nullptr);
  
  // 测试再次初始化应该失败
  Log* another_log = nullptr;
  ret = Logger::init_default("./test_logs/another.log", &another_log);
  EXPECT_NE(ret, 0);
  EXPECT_EQ(another_log, nullptr);
}

// 测试不同日志级别
TEST_F(LogTest, LogLevels) {
  // 这里不需要初始化，直接使用 g_log
  ASSERT_NE(g_log, nullptr);
  
  g_log->trace("This is a TRACE message");
  g_log->debug("This is a DEBUG message");
  g_log->info("This is an INFO message");
  g_log->warn("This is a WARN message");
  g_log->error("This is an ERROR message");
  g_log->panic("This is a PANIC message");
  
  // 也测试流式操作符
  *g_log << "Stream operator test";
  
  std::string content = ReadLogFile("./test_logs/test.log");
  EXPECT_TRUE(content.find("TRACE") != std::string::npos);
  EXPECT_TRUE(content.find("DEBUG") != std::string::npos);
  EXPECT_TRUE(content.find("INFO") != std::string::npos);
  EXPECT_TRUE(content.find("WARN") != std::string::npos);
  EXPECT_TRUE(content.find("ERROR") != std::string::npos);
  EXPECT_TRUE(content.find("PAINC") != std::string::npos);
  EXPECT_TRUE(content.find("Stream operator test") != std::string::npos);
}

// 测试日志级别设置
TEST_F(LogTest, LevelSettings) {
  ASSERT_NE(g_log, nullptr);
  
  // 测试级别变更
  g_log->set_log_level(LogLevel::ERROR);
  g_log->set_console_level(LogLevel::PAINC);
  
  EXPECT_EQ(g_log->get_log_level(), LogLevel::ERROR);
  EXPECT_EQ(g_log->get_console_level(), LogLevel::PAINC);
  
  g_log->trace("This TRACE message should not be logged");
  g_log->error("This ERROR message should be logged");
  g_log->panic("This PANIC message should be logged and printed");
  
  std::string content = ReadLogFile("./test_logs/test.log");
  EXPECT_FALSE(content.find("TRACE message should not be logged") != std::string::npos);
  EXPECT_TRUE(content.find("ERROR message should be logged") != std::string::npos);
  EXPECT_TRUE(content.find("PANIC message should be logged") != std::string::npos);
}

// 测试日志级别过滤
TEST_F(LogTest, LevelFiltering) {
  Log* log = nullptr;
  Logger::init("./test_logs/test_filtering.log", &log, LogLevel::WARN, LogLevel::ERROR);
  
  log->trace("This is a TRACE message");  // 不应该记录
  log->debug("This is a DEBUG message");  // 不应该记录
  log->info("This is an INFO message");   // 不应该记录
  log->warn("This is a WARN message");    // 应该记录到文件
  log->error("This is an ERROR message"); // 应该记录到文件和控制台
  log->panic("This is a PANIC message");  // 应该记录到文件和控制台
  
  std::string content = ReadLogFile("./test_logs/test_filtering.log");
  EXPECT_FALSE(content.find("TRACE message") != std::string::npos);
  EXPECT_FALSE(content.find("DEBUG message") != std::string::npos);
  EXPECT_FALSE(content.find("INFO message") != std::string::npos);
  EXPECT_TRUE(content.find("WARN message") != std::string::npos);
  EXPECT_TRUE(content.find("ERROR message") != std::string::npos);
  EXPECT_TRUE(content.find("PANIC message") != std::string::npos);
}

// 测试宏定义方式的日志输出
TEST_F(LogTest, LogMacros) {
  ASSERT_NE(g_log, nullptr);
  
  LOG_TRACE("Trace log via macro: %d", 1);
  LOG_DEBUG("Debug log via macro: %d", 2);
  LOG_INFO("Info log via macro: %d", 3);
  LOG_WARN("Warn log via macro: %d", 4);
  LOG_ERROR("Error log via macro: %d", 5);
  LOG_PANIC("Panic log via macro: %d", 6);
  
  std::string content = ReadLogFile("./test_logs/test.log");
  EXPECT_TRUE(content.find("Trace log via macro: 1") != std::string::npos);
  EXPECT_TRUE(content.find("Debug log via macro: 2") != std::string::npos);
  EXPECT_TRUE(content.find("Info log via macro: 3") != std::string::npos);
  EXPECT_TRUE(content.find("Warn log via macro: 4") != std::string::npos);
  EXPECT_TRUE(content.find("Error log via macro: 5") != std::string::npos);
  EXPECT_TRUE(content.find("Panic log via macro: 6") != std::string::npos);
}

// 测试模块过滤功能
TEST_F(LogTest, ModuleFiltering) {
  ASSERT_NE(g_log, nullptr);
  
  // 设置仅接受当前文件名的日志
  g_log->set_module(std::string("log_test.cpp"));
  
  LOG_INFO("This should be logged - module filtering test");
  
  std::string content = ReadLogFile("./test_logs/test.log");
  EXPECT_TRUE(content.find("module filtering test") != std::string::npos);
  
  // 重置模块过滤
  g_log->set_module("");
}

// 测试上下文功能
TEST_F(LogTest, ContextId) {
  ASSERT_NE(g_log, nullptr);
  
  // 设置上下文ID提供器
  g_log->set_context([]() -> intptr_t { return 12345; });
  
  LOG_INFO("This log should have context ID 12345");
  
  std::string content = ReadLogFile("./test_logs/test.log");
  EXPECT_TRUE(content.find("ctx:12345") != std::string::npos);
}

// 测试日志轮转功能
TEST_F(LogTest, LogRotation) {
  ASSERT_NE(g_log, nullptr);
  
  // 设置为按行数轮转
  g_log->set_rotate_type(LogRotate::ROTATE_SIZE);
  
  // 写入多条日志尝试触发轮转
  for (int i = 0; i < 50; i++) {
    LOG_INFO("Log line to trigger rotation: %d", i);
  }
}

// 测试多线程日志
TEST_F(LogTest, MultiThreadedLogging) {
  ASSERT_NE(g_log, nullptr);
  
  std::vector<std::thread> threads;
  for (int i = 0; i < 5; i++) {
    threads.emplace_back([i]() {
      for (int j = 0; j < 10; j++) {
        LOG_INFO("Thread %d: Log %d", i, j);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });
  }
  
  for (auto& t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
  
  // 检查是否所有线程的日志都被记录
  std::string content = ReadLogFile("./test_logs/test.log");
  for (int i = 0; i < 5; i++) {
    EXPECT_TRUE(content.find("Thread " + std::to_string(i)) != std::string::npos);
  }
}

} // namespace test
} // namespace common

// 主函数
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
