#pragma once

#include <vector>
#include <deque>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <map>
#include <string>

#include "common/types.h"
#include "storage/clog/log_entry.h"
#include "storage/clog/log_file.h"


class LogFileWriter;


class LogBuffer {
public:
  explicit LogBuffer() = default;
  ~LogBuffer() = default;

  RC init(LSN lsn, int32_t max_bytes = 0);

  // 核心操作接口
  RC append(LSN& lsn, LogModule::Id module_id, std::vector<char>&& data);
  RC append(LSN& lsn, LogModule module, std::vector<char>&& data);
  RC append(LogEntry&& entry);
  RC flush(LogFileWriter& writer);
  
  // 查询接口
  bool is_full() const { return current_bytes_ >= max_bytes_; }
  size_t size() const { return entries_.size(); }
  size_t bytes() const { return current_bytes_; }
  LSN current_lsn() const { return current_lsn_; }
  LSN flushed_lsn() const { return flushed_lsn_; }

  // 配置接口
  void set_max_bytes(size_t max_bytes) { max_bytes_ = max_bytes; }
  void set_flush_threshold(float threshold) { flush_threshold_ = threshold; }

  // 刷盘接口
  RC flush_batch(LogFileWriter& writer, size_t batch_size);

  // 性能统计展示
  std::string to_string() const;

private:
  // 内部辅助方法
  void try_notify_flush();
  bool should_flush() const;

private:
  // 数据存储
  std::deque<LogEntry> entries_;
  
  // 并发控制
  mutable std::mutex mutex_;
  std::condition_variable flush_cv_;
  
  // 状态追踪
  std::atomic<size_t> current_bytes_{0};
  std::atomic<LSN> current_lsn_{0};
  std::atomic<LSN> flushed_lsn_{0};
  
  // 配置参数
  size_t max_bytes_{16 * 1024 * 1024};  // 默认16MB
  float flush_threshold_{0.75};          // 触发刷盘的阈值（默认75%）
  size_t default_batch_size_{1024};      // 默认批量刷盘大小
  
  // 性能统计
  std::atomic<uint64_t> total_appends_{0};
  std::atomic<uint64_t> total_flushes_{0};
  std::atomic<uint64_t> total_wait_time_us_{0};
};
