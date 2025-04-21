#include <gtest/gtest.h>
#include "common/bitmap/bitmap.h"

class BitmapTest : public ::testing::Test {
protected:
  void SetUp() override {
    // 测试前设置
  }

  void TearDown() override {
    // 测试后清理
  }
};

// 测试默认构造函数
TEST_F(BitmapTest, DefaultConstructor) {
  Bitmap bitmap;
  EXPECT_EQ(bitmap.size(), 0);
  EXPECT_EQ(bitmap.bytes(), 0);
  EXPECT_EQ(bitmap.data(), nullptr);
}

// 测试带数据的构造函数
TEST_F(BitmapTest, DataConstructor) {
  // 使用更小的值避免截断
  char data[2] = {0x15, 0x2A};  // 00010101 00101010
  Bitmap bitmap(data, 16);
  
  // 检查位设置
  EXPECT_TRUE(bitmap.get(0));   // 位0: 1
  EXPECT_FALSE(bitmap.get(1));  // 位1: 0
  EXPECT_TRUE(bitmap.get(2));   // 位2: 1
  EXPECT_FALSE(bitmap.get(3));  // 位3: 0
  EXPECT_TRUE(bitmap.get(4));   // 位4: 1
  EXPECT_FALSE(bitmap.get(5));  // 位5: 0
  EXPECT_FALSE(bitmap.get(6));  // 位6: 0
  EXPECT_FALSE(bitmap.get(7));  // 位7: 0
  
  EXPECT_FALSE(bitmap.get(8));  // 位8: 0
  EXPECT_TRUE(bitmap.get(9));   // 位9: 1
  EXPECT_FALSE(bitmap.get(10)); // 位10: 0
  EXPECT_TRUE(bitmap.get(11));  // 位11: 1
  EXPECT_FALSE(bitmap.get(12)); // 位12: 0
  EXPECT_TRUE(bitmap.get(13));  // 位13: 1
  EXPECT_FALSE(bitmap.get(14)); // 位14: 0
  EXPECT_FALSE(bitmap.get(15)); // 位15: 0
  
  // 验证数据指针
  EXPECT_NE(bitmap.data(), nullptr);
  EXPECT_EQ(bitmap.data(), data);  // 应该是浅拷贝
  
}

// 测试next_zero_bit方法
TEST_F(BitmapTest, NextZeroBit) {
  Bitmap bitmap;
  unsigned char data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  bitmap.init(reinterpret_cast<char*>(data), 64);
  
  // 设置所有位为1
  for (int i = 0; i < 64; i++) {
    bitmap.set(i);
  }
  
  // 清除一些位
  bitmap.clear(5);
  bitmap.clear(10);
  bitmap.clear(20);
  
  // 从开始位置查找
  EXPECT_EQ(bitmap.next_zero_bit(0), 5);
  
  // 从中间位置查找
  EXPECT_EQ(bitmap.next_zero_bit(6), 10);
  EXPECT_EQ(bitmap.next_zero_bit(11), 20);
  
  // 从超出范围的位置查找
  EXPECT_EQ(bitmap.next_zero_bit(64), -1);
  
  // 清除第一个位
  bitmap.clear(0);
  EXPECT_EQ(bitmap.next_zero_bit(0), 0);
}

// 测试next_one_bit方法
TEST_F(BitmapTest, NextOneBit) {
  Bitmap bitmap;
  unsigned char data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  bitmap.init(reinterpret_cast<char*>(data), 64);
  
  // 所有位初始为0
  EXPECT_EQ(bitmap.next_one_bit(0), -1);
  
  // 设置一些位
  bitmap.set(5);
  bitmap.set(10);
  bitmap.set(20);
  
  // 从开始位置查找
  EXPECT_EQ(bitmap.next_one_bit(0), 5);
  
  // 从中间位置查找
  EXPECT_EQ(bitmap.next_one_bit(6), 10);
  EXPECT_EQ(bitmap.next_one_bit(11), 20);
  
  // 从超出范围的位置查找
  EXPECT_EQ(bitmap.next_one_bit(64), -1);
  
  // 设置第一个位
  bitmap.set(0);
  EXPECT_EQ(bitmap.next_one_bit(0), 0);
}

// 测试init方法
TEST_F(BitmapTest, Init) {
  Bitmap bitmap;
  
  // 初始化为空
  EXPECT_EQ(bitmap.size(), 0);
  
  // 初始化新数据
  unsigned char data[2] = {0x15, 0x2A};  // 00010101 00101010
  bitmap.init(reinterpret_cast<char*>(data), 16);
  
  // 验证初始化
  EXPECT_EQ(bitmap.size(), 16);
  EXPECT_TRUE(bitmap.get(0));   // 位0: 1
  EXPECT_FALSE(bitmap.get(1));  // 位1: 0
  EXPECT_TRUE(bitmap.get(2));   // 位2: 1
  EXPECT_FALSE(bitmap.get(3));  // 位3: 0
  EXPECT_TRUE(bitmap.get(4));   // 位4: 1
  EXPECT_FALSE(bitmap.get(5));  // 位5: 0
  EXPECT_FALSE(bitmap.get(6));  // 位6: 0
  EXPECT_FALSE(bitmap.get(7));  // 位7: 0
  
  EXPECT_FALSE(bitmap.get(8));  // 位8: 0
  EXPECT_TRUE(bitmap.get(9));   // 位9: 1
  EXPECT_FALSE(bitmap.get(10)); // 位10: 0
  EXPECT_TRUE(bitmap.get(11));  // 位11: 1
  EXPECT_FALSE(bitmap.get(12)); // 位12: 0
  EXPECT_TRUE(bitmap.get(13));  // 位13: 1
  EXPECT_FALSE(bitmap.get(14)); // 位14: 0
  EXPECT_FALSE(bitmap.get(15)); // 位15: 0
}

// 测试to_string方法
TEST_F(BitmapTest, ToString) {
  Bitmap bitmap;
  char data[1] = {0x00};  // 00101010
  bitmap.init(data, 8);
  
  bitmap.set(0);
  bitmap.set(2);
  bitmap.set(4);
  bitmap.set(6);
  
  std::string expected = "10101010";
  EXPECT_EQ(bitmap.to_string(), expected);
} 