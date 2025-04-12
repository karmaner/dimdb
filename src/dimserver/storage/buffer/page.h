#pragma once

#include "common/types.h"
#include <string.h>

using TrxID = int32_t;
using PageNum = int32_t;


// 页面标志位定义
static constexpr uint8_t PAGE_DIRTY_FLAG     = 0x01;  // 脏页标志
static constexpr uint8_t PAGE_IO_IN_PROGRESS = 0x02;  // I/O操作进行中
static constexpr uint8_t PAGE_PINNED         = 0x04;  // 页面被锁定
static constexpr uint8_t PAGE_IN_FLUSH_LIST  = 0x08;  // 在刷新列表中
static constexpr uint8_t PAGE_ENCRYPTED      = 0x10;  // 页面已加密
static constexpr uint8_t PAGE_COMPRESSED     = 0x20;  // 页面已压缩


static constexpr PageNum BP_INVALID_PAGE_NUM = -1;

static constexpr PageNum BP_HEADER_PAGE = 0;

static constexpr const int BP_PAGE_SIZE = (1 << 13);

// 页面类型定义
enum PageType {
  UNKNOWN_PAGE = 0,
  HEADER_PAGE = 1,
  DATA_PAGE = 2,
  INDEX_PAGE = 3,
  OVERFLOW_PAGE = 4,
  FREE_PAGE = 5
};


/**
 * @brief 页面头部结构
 * @ingroup BufferPool
 */
struct PageHeader {
  PageNum   page_num;        // 页面编号
  LSN       lsn;             // 日志序列号
  CheckSum  check_sum;       // 校验和
  uint16_t  free_space;      // 页面中的空闲空间大小
  uint16_t  free_space_offset; // 空闲空间起始位置
  uint16_t  slot_count;      // 槽位数量
  uint8_t   page_type;       // 页面类型: 数据页、索引页、溢出页等
  uint8_t   flags;           // 页面标志: 是否脏页等
  TrxID     last_trx_id;     // 最后修改此页面的事务ID
};

static constexpr const int BP_PAGE_DATA_SIZE = (BP_PAGE_SIZE - sizeof(PageHeader));

/**
 * @brief 表示一个页面，可能放在内存或磁盘上
 * @ingroup BufferPool
 */
struct Page {
  PageHeader header;         // 页面头部
  char       data[BP_PAGE_DATA_SIZE]; // 页面数据

  void init() {
    header.page_num = BP_INVALID_PAGE_NUM;
    header.lsn = 0;
    header.check_sum = 0;
    header.free_space = BP_PAGE_DATA_SIZE;
    header.free_space_offset = 0;
    header.slot_count = 0;
    header.page_type = 0;
    header.flags = 0;
    header.last_trx_id = 0;
    memset(data, 0, BP_PAGE_DATA_SIZE);
  }

  void calc_checksum() {
    static uint32_t crc_table[256];
    static bool table_initialized = false;
    
    // 初始化CRC表（只执行一次）
    if (!table_initialized) {
      for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (uint32_t j = 0; j < 8; j++) {
          c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        }
        crc_table[i] = c;
      }
      table_initialized = true;
    }
    
    // 计算CRC32（只对data部分进行校验）
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < BP_PAGE_DATA_SIZE; i++) {
      crc = crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    header.check_sum = static_cast<CheckSum>(~crc);
  }

  bool verify_checksum() const {
    CheckSum old_checksum = header.check_sum;
    const_cast<Page*>(this)->calc_checksum();
    bool valid = (old_checksum == header.check_sum);
    const_cast<Page*>(this)->header.check_sum = old_checksum;
    return valid;
  }

};

