#include "storage/clog/log_entry.h"
#include "common/log/log.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>

using namespace std;

class LogEntryTest : public testing::Test {
protected:
  void SetUp() override {
  }
};

// 测试 LogEntry 的初始化
TEST_F(LogEntryTest, Init) {
  LogEntry entry;
  vector<char> data = {'t', 'e', 's', 't'};
  
  // 使用 LogModule::Id 初始化
  EXPECT_EQ(entry.init(1, LogModule(1), vector<char>(data)), RC::SUCCESS);
  EXPECT_EQ(entry.lsn(), 1);
  EXPECT_EQ(entry.module().index(), 1);
  EXPECT_EQ(entry.payload_size(), 4);
  
  // 使用 LogModule 初始化
  LogEntry entry2;
  EXPECT_EQ(entry2.init(2, LogModule(2), vector<char>(data)), RC::SUCCESS);
  EXPECT_EQ(entry2.lsn(), 2);
  EXPECT_EQ(entry2.module().index(), 2);
  EXPECT_EQ(entry2.payload_size(), 4);
  EXPECT_EQ(sizeof(entry2.header()), 16);
  EXPECT_EQ(entry2.total_size(), 20);
}

// 测试 LogEntry 的数据访问
TEST_F(LogEntryTest, DataAccess) {
  LogEntry entry;
  vector<char> data = {'t', 'e', 's', 't'};
  EXPECT_EQ(entry.init(1, LogModule(1), vector<char>(data)), RC::SUCCESS);
  
  // 测试数据访问
  EXPECT_EQ(entry.payload_size(), 4);
  EXPECT_EQ(entry.total_size(), 4 + LogHeader::HEAD_SIZE);
  EXPECT_EQ(string(entry.data(), entry.payload_size()), "test");
  
  // 测试 header 访问
  const LogHeader& header = entry.header();
  EXPECT_EQ(header.lsn, 1);
  EXPECT_EQ(header.module_id, 1);
  EXPECT_EQ(header.data_size, 4);
}

// 测试 LogEntry 的移动语义
TEST_F(LogEntryTest, MoveSemantics) {
  LogEntry entry1;
  vector<char> data = {'t', 'e', 's', 't'};
  EXPECT_EQ(entry1.init(1, LogModule(1), vector<char>(data)), RC::SUCCESS);
  
  // 测试移动构造
  LogEntry entry2(std::move(entry1));
  EXPECT_EQ(entry2.lsn(), 1);
  EXPECT_EQ(entry2.payload_size(), 4);
  
  // 测试移动赋值
  LogEntry entry3;
  entry3 = std::move(entry2);
  EXPECT_EQ(entry3.lsn(), 1);
  EXPECT_EQ(entry3.payload_size(), 4);
}

// 测试 LogEntry 的大小限制
TEST_F(LogEntryTest, SizeLimit) {
  LogEntry entry;
  vector<char> data(LogEntry::max_payload_size() + 1, 'a');
  
  // 测试超出大小限制
  EXPECT_EQ(entry.init(1, LogModule(1), vector<char>(data)), RC::MESSAGE_INVAID);
  
  // 测试刚好在限制内
  data.resize(LogEntry::max_payload_size());
  EXPECT_EQ(entry.init(1, LogModule(1), vector<char>(data)), RC::SUCCESS);
  // cout << entry.to_string();
}

// 测试 LogEntry 的字符串表示
TEST_F(LogEntryTest, ToString) {
  LogEntry entry;
  vector<char> data = {'t', 'e', 's', 't'};
  EXPECT_EQ(entry.init(1, LogModule(1), vector<char>(data)), RC::SUCCESS);
  
  string str = entry.to_string();
  cout << str << endl;
  EXPECT_TRUE(str.find("lsn=1") != string::npos);
  EXPECT_TRUE(str.find("module_id=1") != string::npos);
  EXPECT_TRUE(str.find("data=test") != string::npos);
} 