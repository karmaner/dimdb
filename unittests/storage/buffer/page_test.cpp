#include <gtest/gtest.h>
#include "storage/buffer/page.h"
#include <memory>
#include <cstring>

class PageTest : public ::testing::Test {
protected:
	std::unique_ptr<Page> page;

	void SetUp() override {
			page = std::make_unique<Page>();
			page->init();
	}
};

// 测试页面初始化
TEST_F(PageTest, InitializationTest) {
	EXPECT_EQ(page->header.page_num, BP_INVALID_PAGE_NUM);
	EXPECT_EQ(page->header.lsn, 0);
	EXPECT_EQ(page->header.check_sum, 0);
	EXPECT_EQ(page->header.free_space, BP_PAGE_DATA_SIZE);
	EXPECT_EQ(page->header.free_space_offset, 0);
	EXPECT_EQ(page->header.slot_count, 0);
	EXPECT_EQ(page->header.page_type, 0);
	EXPECT_EQ(page->header.flags, 0);
	EXPECT_EQ(page->header.last_trx_id, 0);
}

// 测试校验和计算和验证
TEST_F(PageTest, ChecksumTest) {
	// 写入一些测试数据
	const char test_data[] = "Hello, World!";
	std::memcpy(page->data, test_data, sizeof(test_data));
	
	// 验证校验和 数据变了
	EXPECT_FALSE(page->verify_checksum());
	
	// 修改数据后校验和应该不匹配
	page->data[0] = 'h';
	EXPECT_FALSE(page->verify_checksum());
}

// 测试页面标志位操作
TEST_F(PageTest, PageFlagsTest) {
	// 测试设置脏页标志
	page->header.flags |= PAGE_DIRTY_FLAG;
	EXPECT_TRUE(page->header.flags & PAGE_DIRTY_FLAG);
	
	// 测试设置多个标志
	page->header.flags |= PAGE_PINNED;
	EXPECT_TRUE(page->header.flags & PAGE_PINNED);
	EXPECT_TRUE(page->header.flags & PAGE_DIRTY_FLAG);
	
	// 测试清除标志
	page->header.flags &= ~PAGE_DIRTY_FLAG;
	EXPECT_FALSE(page->header.flags & PAGE_DIRTY_FLAG);
	EXPECT_TRUE(page->header.flags & PAGE_PINNED);
}

// 测试页面类型设置
TEST_F(PageTest, PageTypeTest) {
	page->header.page_type = PageType::DATA_PAGE;
	EXPECT_EQ(page->header.page_type, PageType::DATA_PAGE);
	
	page->header.page_type = PageType::INDEX_PAGE;
	EXPECT_EQ(page->header.page_type, PageType::INDEX_PAGE);
}

// 测试页面空间管理
TEST_F(PageTest, SpaceManagementTest) {
	const uint16_t allocation_size = 100;
	
	// 模拟分配空间
	uint16_t original_free_space = page->header.free_space;
	page->header.free_space -= allocation_size;
	page->header.free_space_offset += allocation_size;
	
	EXPECT_EQ(page->header.free_space, original_free_space - allocation_size);
	EXPECT_EQ(page->header.free_space_offset, allocation_size);
}

// 测试页面大小常量
TEST_F(PageTest, PageSizeConstantsTest) {
	EXPECT_EQ(BP_PAGE_SIZE, (1 << 13));
	EXPECT_EQ(BP_PAGE_DATA_SIZE, BP_PAGE_SIZE - sizeof(PageHeader));
	
	// 确保页面数据区大小合理
	EXPECT_GT(BP_PAGE_DATA_SIZE, 0);
	EXPECT_LT(sizeof(PageHeader), BP_PAGE_SIZE);
} 