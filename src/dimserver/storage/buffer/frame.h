#pragma once

#include <string>
#include <mutex>
#include <atomic>
#include <sstream>

#include "common/log/log.h"
#include "storage/buffer/page.h"

namespace storage {

/**
 * 帧ID，用于唯一标识缓冲池中的帧
 */
struct FrameId {
  int32_t buffer_pool_id; // 缓冲区ID
  PageNum page_num;       // 页面编号

  FrameId() : buffer_pool_id(-1), page_num(BP_INVALID_PAGE_NUM) {}
  FrameId(int32_t pool_id, PageNum pnum) : buffer_pool_id(pool_id), page_num(pnum) {}

  bool operator==(const FrameId& other) const;
  bool operator!=(const FrameId& other) const;
  bool operator<(const FrameId& other) const;
  bool is_valid() const;
  size_t hash() const;
  std::string to_string() const;
};

/**
 * 帧类，缓冲池的基本单位，用于管理内存中的页面
 */
class Frame {
public:
  // 构造函数和析构函数
  Frame();
  ~Frame();

  /**
   * @brief reinit 和 reset 在 MemPoolSimple 中使用
   * @details 在 MemPoolSimple 分配和释放一个Frame对象时，不会调用构造函数和析构函数，
   * 而是调用reinit和reset。
   */
  void reinit() {
    pin_count_ = 0;
    acc_time_ = 0;
    frame_id_ = FrameId();
    page_.init();
  }

  void reset() {
    pin_count_ = 0;
    acc_time_ = 0;
    frame_id_ = FrameId();
    page_.init();
  }

  void clear_page() { memset(&page_, 0, sizeof(page_)); }

  // 获取帧ID
  const FrameId& frame_id() const;

  char* data() { return page_.data; }
  
  // 设置帧ID
  void set_frame_id(const FrameId& frame_id);
  
  // 获取页面
  Page& page();
  const Page& page() const;
  
  // 获取页号
  PageNum page_num() const;
  
  // 增加引用计数
  void pin();
  
  // 减少引用计数
  void unpin();
  
  // 获取引用计数
  int pin_count() const;

  bool can_purge() const { return pin_count_.load() == 0; }

  void access();
  
  // 设置/获取脏页标记 - 直接使用Page中的标记
  bool is_dirty() const;
  void mark_dirty();
  void clear_dirty();

  int buffer_pool_id() const;
  void set_buffer_pool_id(int id);

  LSN lsn() const;
  void set_lsn(LSN lsn);

  void set_page_num(PageNum page_num);
  PageType page_type() const;
  void set_page_type(PageType type);

  void calc_checksum();
  bool verify_checksum() const;
  
  std::string to_string() const;

private:
  std::atomic<int> pin_count_{0};       // 引用计数
  unsigned long acc_time_ = 0;          // 最后访问时间（用于LRU替换）
  FrameId frame_id_;                    // 帧ID
  Page page_;                           // 页面数据
  std::mutex mutex_;                    // 互斥锁，用于并发控制
};

} // namespace storage