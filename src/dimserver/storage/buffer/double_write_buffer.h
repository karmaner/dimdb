#pragma once

#include <unordered_map>
#include <mutex>

#include "common/types.h"
#include "common/rc.h"
#include "storage/buffer/page.h"
#include "storage/buffer/buffer_pool.h"



struct DoubleWritePage;
class BufferPoolManager;

class DoubleWriteBuffer {
public:
  DoubleWriteBuffer()          = default;
  virtual ~DoubleWriteBuffer() = default;

  virtual RC add_page(BufferPool *bp, PageNum page_num, Page &page) = 0;
  virtual RC read_page(BufferPool *bp, PageNum page_num, Page &page) = 0;
  virtual RC clear_pages(BufferPool *bp) = 0;
};


struct DoubleWriteBufferHeader {
  int32_t page_cnt = 0;
  static const int32_t SIZE;
};

struct DoubleWritePageKey {
  int32_t buffer_pool_id;
  PageNum page_num;

  bool operator==(const DoubleWritePageKey &other) const {
    return buffer_pool_id == other.buffer_pool_id && page_num == other.page_num;
  }
};

struct DoubleWritePageKeyHash {
  size_t operator()(const DoubleWritePageKey &key) const {
    return std::hash<int32_t>()(key.buffer_pool_id) ^ std::hash<PageNum>()(key.page_num);
  }
};

class DiskDoubleWriteBuffer : public DoubleWriteBuffer {
public:
  DiskDoubleWriteBuffer(BufferPoolManager &bp_manager, int max_pages = 16);
  virtual ~DiskDoubleWriteBuffer();

  RC open_file(const std::string& filename);
  RC flush_page();

  RC add_page(BufferPool *bp, PageNum page_num, Page &page) override;
  RC read_page(BufferPool *bp, PageNum page_num, Page &page) override;

  RC clear_pages(BufferPool *bp) override;
  RC recover();

private:
  RC write_page(DoubleWritePage *page);
  RC write_page_internal(DoubleWritePage *page);

  RC load_pages();

private:
  int file_desc_ = -1;
  int max_pages_ = 0;

  BufferPoolManager& bp_manager_;
  DoubleWriteBufferHeader header_;

  std::unordered_map<DoubleWritePageKey, DoubleWritePage*,
    DoubleWritePageKeyHash> dblwr_pages_;
};


class VacuousDoubleWriteBuffer : public DoubleWriteBuffer
{
public:
  virtual ~VacuousDoubleWriteBuffer() = default;
  RC add_page(BufferPool *bp, PageNum page_num, Page &page) override;

  RC read_page(BufferPool *bp, PageNum page_num, Page &page) override { 
    return RC::BUFFERPOOL_INVALID_PAGE_NUM;
  }

  RC clear_pages(BufferPool *bp) override { return RC::SUCCESS; }
};