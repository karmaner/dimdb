#include "storage/clog/log_file.h"
#include "storage/clog/log_entry.h"
#include "common/log/log.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <vector>
#include <string>

using namespace std;

class LogFileTest : public testing::Test {
protected:
  void SetUp() override {
    // 创建测试目录
    test_dir = "test_logs";
    if (!filesystem::exists(test_dir)) {
      filesystem::create_directories(test_dir);
    }
    common::InitLogger();
  }
  
  void TearDown() override {
    // 清理测试文件
    if (filesystem::exists(test_dir)) {
      filesystem::remove_all(test_dir);
    }
  }
  
  string test_dir;
};

// // 测试 LogFileManager 的文件名解析
// TEST_F(LogFileTest, GetLsnFromFilename) {
//   LogFileManager manager;
//   LSN lsn;
  
//   // 测试有效的文件名
//   EXPECT_EQ(manager.get_lsn_from_filename("clog_123.log", lsn), RC::SUCCESS);
//   EXPECT_EQ(lsn, 123);
  
//   // 测试无效的文件名
//   EXPECT_EQ(manager.get_lsn_from_filename("invalid.log", lsn), RC::FILE_NAME_INVALID);
//   EXPECT_EQ(manager.get_lsn_from_filename("clog_abc.log", lsn), RC::FILE_NAME_INVALID);
//   EXPECT_EQ(manager.get_lsn_from_filename("clog_123.txt", lsn), RC::FILE_NAME_INVALID);
// }

// 测试 LogFileWriter 的写入功能
TEST_F(LogFileTest, LogFileWriter) {
  LogFileWriter writer;
  string filename = test_dir + "/clog_1.log";
  
  // 打开文件
  EXPECT_EQ(writer.open(filename, 1000), RC::SUCCESS);
  EXPECT_TRUE(writer.is_open());
  
  // 创建并写入日志条目
  LogEntry entry;
  vector<char> data = {'t', 'e', 's', 't'};
  EXPECT_EQ(entry.init(1, LogModule(1), std::move(data)), RC::SUCCESS);
  EXPECT_EQ(writer.write(entry), RC::SUCCESS);
  
  // 关闭文件
  EXPECT_EQ(writer.close(), RC::SUCCESS);
  EXPECT_FALSE(writer.is_open());
}

// 测试 LogFileReader 的读取功能
TEST_F(LogFileTest, LogFileReader) {
  // 先写入一些日志
  LogFileWriter writer;
  string filename = test_dir + "/clog_1.log";
  EXPECT_EQ(writer.open(filename, 1000), RC::SUCCESS);
  
  // 写入多个日志条目
  for (int i = 1; i <= 3; i++) {
    LogEntry entry;
    vector<char> data = {'t', 'e', 's', 't'};
    EXPECT_EQ(entry.init(i, LogModule(1), std::move(data)), RC::SUCCESS);
    EXPECT_EQ(writer.write(entry), RC::SUCCESS);
  }
  writer.close();
  
  // 读取日志
  LogFileReader reader;
  EXPECT_EQ(reader.open(filename), RC::SUCCESS);
  
  int count = 0;
  auto callback = [&count](LogEntry& entry) {
    count++;
    EXPECT_EQ(entry.lsn(), count);
    EXPECT_EQ(entry.module().index(), 1);
    return RC::SUCCESS;
  };
  
  EXPECT_EQ(reader.iterate(callback, 1), RC::SUCCESS);
  EXPECT_EQ(count, 3);
  
  reader.close();
}

// 测试 LogFileManager 的文件管理功能
TEST_F(LogFileTest, LogFileManager) {
  // 创建一些日志文件
  vector<string> files;
  for (int i = 1; i <= 3; i++) {
    string filename = test_dir + "/clog_" + to_string(i) + ".log";
    ofstream file(filename);
    file.close();
  }

  LogFileManager manager;
  EXPECT_EQ(manager.init(test_dir, 1000), RC::SUCCESS);
  
  // 列出文件
  EXPECT_EQ(manager.list_files(files, 1), RC::SUCCESS);
  EXPECT_EQ(files.size(), 3);
  
  // 获取最后一个文件
  LogFileWriter writer;
  EXPECT_EQ(manager.last_file(writer), RC::SUCCESS);
  EXPECT_TRUE(writer.is_open());
  writer.close();
  
  // 获取下一个文件
  EXPECT_EQ(manager.next_file(writer), RC::SUCCESS);
  EXPECT_TRUE(writer.is_open());
  writer.close();
} 