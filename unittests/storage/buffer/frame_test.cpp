#include <gtest/gtest.h>
#include "storage/buffer/frame.h"
#include <memory>
#include <cstring>
#include <thread>

using namespace storage;

class FrameTest : public ::testing::Test {
protected:
  std::unique_ptr<Frame> frame;

  void SetUp() override {
    frame = std::make_unique<Frame>();
  }
};

// 测试FrameId的基本操作
TEST_F(FrameTest, FrameIdTest) {
  FrameId frame_id(1, 100);
  EXPECT_EQ(frame_id.buffer_pool_id, 1);
  EXPECT_EQ(frame_id.page_num, 100);
  EXPECT_TRUE(frame_id.is_valid());

  FrameId invalid_frame_id;
  EXPECT_FALSE(invalid_frame_id.is_valid());
}

// 测试页面基本操作
TEST_F(FrameTest, PageOperationsTest) {
  // 测试页面访问
  Page& page = frame->page();
  EXPECT_EQ(page.header.page_num, BP_INVALID_PAGE_NUM);
  
  // 测试数据访问
  char* data = frame->data();
  EXPECT_NE(data, nullptr);
  
  // 测试清除页面
  frame->clear_page();
  EXPECT_EQ(page.header.page_num, 0);
}

// 测试引用计数操作
TEST_F(FrameTest, PinUnpinTest) {
  EXPECT_EQ(frame->pin_count(), 0);
  
  frame->pin();
  EXPECT_EQ(frame->pin_count(), 1);
  
  frame->pin();
  EXPECT_EQ(frame->pin_count(), 2);
  
  frame->unpin();
  EXPECT_EQ(frame->pin_count(), 1);
  
  frame->unpin();
  EXPECT_EQ(frame->pin_count(), 0);
}

// 测试脏页标记
TEST_F(FrameTest, DirtyFlagTest) {
  EXPECT_FALSE(frame->is_dirty());
  
  frame->mark_dirty();
  EXPECT_TRUE(frame->is_dirty());
  
  frame->clear_dirty();
  EXPECT_FALSE(frame->is_dirty());
}

// 测试缓冲区ID操作
TEST_F(FrameTest, BufferPoolIdTest) {
  EXPECT_EQ(frame->buffer_pool_id(), -1);
  
  frame->set_buffer_pool_id(1);
  EXPECT_EQ(frame->buffer_pool_id(), 1);
}

// 测试LSN操作
TEST_F(FrameTest, LSNTest) {
  EXPECT_EQ(frame->lsn(), 0);
  
  LSN test_lsn = 12345;
  frame->set_lsn(test_lsn);
  EXPECT_EQ(frame->lsn(), test_lsn);
}

// 测试页面类型操作
TEST_F(FrameTest, PageTypeTest) {
  EXPECT_EQ(frame->page_type(), PageType::UNKNOWN_PAGE);
  
  frame->set_page_type(PageType::DATA_PAGE);
  EXPECT_EQ(frame->page_type(), PageType::DATA_PAGE);
  
  frame->set_page_type(PageType::INDEX_PAGE);
  EXPECT_EQ(frame->page_type(), PageType::INDEX_PAGE);
}

// 测试校验和操作
TEST_F(FrameTest, ChecksumTest) {
  // 写入一些测试数据
  const char test_data[] = "Hello, World!";
  std::memcpy(frame->data(), test_data, sizeof(test_data));
  
  // 计算校验和
  frame->calc_checksum();
  
  // 验证校验和
  EXPECT_TRUE(frame->verify_checksum());
  
  // 修改数据后校验和应该不匹配
  frame->data()[0] = 'h';
  EXPECT_FALSE(frame->verify_checksum());
}

// 测试并发访问
TEST_F(FrameTest, ConcurrentAccessTest) {
  const int num_threads = 10;
  std::vector<std::thread> threads;
  
  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back([this]() {
      frame->pin();
      frame->access();
      frame->unpin();
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  EXPECT_EQ(frame->pin_count(), 0);
}

// 测试FrameId比较操作
TEST_F(FrameTest, FrameIdComparisonTest) {
  FrameId id1(1, 100);
  FrameId id2(1, 100);
  FrameId id3(1, 200);
  FrameId id4(2, 100);
  
  EXPECT_TRUE(id1 == id2);
  EXPECT_FALSE(id1 == id3);
  EXPECT_FALSE(id1 == id4);
  
  EXPECT_TRUE(id1 < id3);
  EXPECT_TRUE(id1 < id4);
  EXPECT_FALSE(id3 < id1);
} 