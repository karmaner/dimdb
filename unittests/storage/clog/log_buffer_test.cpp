#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <filesystem>

#include "storage/clog/log_buffer.h"
#include "storage/clog/log_file.h"
#include "common/rc.h"
#include "common/log/log.h"

/**
 * @brief LogBuffer测试类
 * 用于测试LogBuffer的各种功能和边界条件
 */
class LogBufferTest : public testing::Test {
protected:
  /**
   * @brief 测试用例初始化
   * 创建临时目录并初始化LogBuffer
   */
  void SetUp() override {
    // 创建临时目录用于测试
    test_dir = "test_logs";
    if (!std::filesystem::exists(test_dir)) {
      std::filesystem::create_directories(test_dir);
    }
    
    // 初始化LogBuffer，设置1MB的缓冲区大小
    buffer.init(0, 1024 * 1024);
  }

  /**
   * @brief 测试用例清理
   * 删除临时目录
   */
  void TearDown() override {
    // 清理临时目录
    std::filesystem::remove_all(test_dir);
  }

protected:
  LogBuffer buffer;        // LogBuffer实例
  std::string test_dir;    // 测试目录路径
};

/**
 * @brief 测试基本操作
 * 包括初始化、追加日志和刷盘功能
 */
TEST_F(LogBufferTest, BasicOperations) {
  // 测试初始化状态
  EXPECT_EQ(buffer.current_lsn(), 0);
  EXPECT_EQ(buffer.flushed_lsn(), 0);
  EXPECT_EQ(buffer.size(), 0);
  EXPECT_EQ(buffer.bytes(), 0);
  EXPECT_FALSE(buffer.is_full());

  // 测试追加日志
  std::vector<char> data = {'t', 'e', 's', 't'};
  LSN lsn = 0;
  RC rc = buffer.append(lsn, LogModule(1), std::move(data));
  EXPECT_EQ(rc, RC::SUCCESS);
  EXPECT_EQ(lsn, 1);
  EXPECT_EQ(buffer.size(), 1);
  EXPECT_EQ(buffer.current_lsn(), 1);

  // 测试刷盘功能
  LogFileWriter writer;
  rc = writer.open(test_dir + "/test.log", 1000);
  ASSERT_EQ(rc, RC::SUCCESS);

  rc = buffer.flush(writer);
  EXPECT_EQ(rc, RC::SUCCESS);
  EXPECT_EQ(buffer.flushed_lsn(), 1);
  EXPECT_EQ(buffer.size(), 0);
}

/**
 * @brief 测试边界条件
 * 包括缓冲区满和空缓冲区刷盘的情况
 */
TEST_F(LogBufferTest, BoundaryConditions) {
  // 测试缓冲区满的情况
  std::vector<char> large_data(16 * 1024 * 1024 + 1, 'a');
  LSN lsn = 0;
  RC rc = buffer.append(lsn, LogModule(1), std::move(large_data));
  EXPECT_EQ(rc, RC::MESSAGE_INVAID);  // 应该返回消息无效错误

  // 测试空缓冲区刷盘
  LogFileWriter writer;
  rc = writer.open(test_dir + "/test.log", 1000);
  ASSERT_EQ(rc, RC::SUCCESS);

  rc = buffer.flush(writer);
  EXPECT_EQ(rc, RC::SUCCESS);
}

/**
 * @brief 测试并发操作
 * 测试多线程同时追加日志的情况
 */
TEST_F(LogBufferTest, ConcurrentOperations) {
  const int num_threads = 4;          // 线程数
  const int entries_per_thread = 100; // 每个线程追加的日志数
  std::vector<std::thread> threads;

  // 创建多个线程同时追加日志
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([this, i]() {
      for (int j = 0; j < entries_per_thread; ++j) {
        std::vector<char> data = {'t', 'e', 's', 't'};
        LSN lsn = j;
        RC rc = buffer.append(lsn, LogModule(0), std::move(data));
        EXPECT_EQ(rc, RC::SUCCESS);
      }
    });
  }

  // 等待所有线程完成
  for (auto& thread : threads) {
    thread.join();
  }

  // 验证结果
  EXPECT_EQ(buffer.size(), num_threads * entries_per_thread);
  EXPECT_EQ(buffer.current_lsn(), num_threads * entries_per_thread);
}

/**
 * @brief 测试性能统计
 * 测试性能统计信息的收集和输出
 */
TEST_F(LogBufferTest, PerformanceStats) {
  // 追加一些日志
  for (int i = 0; i < 100; ++i) {
    std::vector<char> data = {'t', 'e', 's', 't'};
    LSN lsn = 0;
    RC rc = buffer.append(lsn, LogModule(1), std::move(data));
    EXPECT_EQ(rc, RC::SUCCESS);
  }

  // 刷盘
  LogFileWriter writer;
  RC rc = writer.open(test_dir + "/test.log", 1000);
  ASSERT_EQ(rc, RC::SUCCESS);

  rc = buffer.flush(writer);
  EXPECT_EQ(rc, RC::SUCCESS);

  // 打印性能统计信息
  LOG_INFO("Performance stats: %s", buffer.to_string().c_str());
  std::cout << buffer.to_string();
}

/**
 * @brief 测试配置参数
 * 测试缓冲区大小和刷盘阈值的设置
 */
TEST_F(LogBufferTest, Configuration) {
  int max_byte = 2 * 1024 * 1024;
  // 测试设置最大字节数
  buffer.set_max_bytes(max_byte);  // 2MB
  EXPECT_EQ(buffer.bytes(), 0);

  // 测试设置刷盘阈值
  buffer.set_flush_threshold(0.5f);
  std::vector<char> data(1024 * 1024, 'a');  // 1MB
  LSN lsn = 0;
  RC rc = buffer.append(lsn, LogModule(1), std::move(data));
  EXPECT_EQ(rc, RC::SUCCESS);
  // 检查缓冲区使用率是否超过阈值
  EXPECT_GE(buffer.bytes(), max_byte * 0.5f);
}

/**
 * @brief 测试日志条目移动语义
 * 测试LogEntry的移动构造和移动赋值
 */
TEST_F(LogBufferTest, MoveSemantics) {
  LogEntry entry;
  std::vector<char> data = {'t', 'e', 's', 't'};
  RC rc = entry.init(1, LogModule(1), std::move(data));
  ASSERT_EQ(rc, RC::SUCCESS);

  // 测试移动构造
  LogEntry entry2(std::move(entry));
  EXPECT_EQ(entry2.lsn(), 1);
  EXPECT_EQ(entry2.payload_size(), 4);

  // 测试移动赋值
  LogEntry entry3;
  entry3 = std::move(entry2);
  EXPECT_EQ(entry3.lsn(), 1);
  EXPECT_EQ(entry3.payload_size(), 4);
}

/**
 * @brief 测试错误处理
 * 测试各种异常情况的处理
 */
TEST_F(LogBufferTest, ErrorHandling) {
  // 测试无效的模块ID
  std::vector<char> data = {'t', 'e', 's', 't'};
  LSN lsn = 0;
  RC rc = buffer.append(lsn, LogModule(1), std::move(data));
  EXPECT_EQ(rc, RC::SUCCESS);

  // 测试空数据
  data.clear();
  rc = buffer.append(lsn, LogModule(1), std::move(data));
  EXPECT_EQ(rc, RC::SUCCESS);  // 空数据应该是允许的
}

/**
 * @brief 主函数
 * 运行所有测试用例
 */
int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
} 