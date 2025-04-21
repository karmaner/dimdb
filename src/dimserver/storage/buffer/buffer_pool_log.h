#pragma once

#include <string.h>

#include "common/types.h"
#include "common/rc.h"
#include "storage/clog/log_replayer.h"
#include "storage/buffer/buffer_pool.h"

class BufferPool;
class BufferPoolManager;
class LogHandler;
struct Page;

class BufferPoolOperation
{
public:
  enum class Type : int32_t
  {
    ALLOCATE,
    DEALLOCATE
  };

public:
  BufferPoolOperation(Type type) : type_(type) {}
  explicit BufferPoolOperation(int32_t type) : type_(static_cast<Type>(type)) {}
  ~BufferPoolOperation() = default;

  Type    type() const { return type_; }
  int32_t type_id() const { return static_cast<int32_t>(type_); }

  std::string to_string() const
  {
    std::string ret = std::to_string(type_id()) + ":";
    switch (type_) {
      case Type::ALLOCATE: return ret + "ALLOCATE";
      case Type::DEALLOCATE: return ret + "DEALLOCATE";
      default: return ret + "UNKNOWN";
    }
  }

private:
  Type type_;
};

struct BufferPoolLogEntry
{
  int32_t buffer_pool_id;  /// buffer pool id
  int32_t operation_type;  /// operation type
  PageNum page_num;        /// page number

  std::string to_string() const;
};


class BufferPoolLogHandler final
{
public:
  BufferPoolLogHandler(BufferPool &buffer_pool, LogHandler &log_handler);
  ~BufferPoolLogHandler() = default;

  /**
   * @brief 分配一个页面
   * @param page_num 分配的页面号
   * @param[out] lsn 分配页面的日志序列号
   * @note TODO 可以把frame传过来，记录完日志，直接更新页面的lsn
   */
  RC allocate_page(PageNum page_num, LSN &lsn);

  /**
   * @brief 释放一个页面
   * @param page_num 释放的页面编号
   * @param[out] lsn 释放页面的日志序列号
   */
  RC deallocate_page(PageNum page_num, LSN &lsn);

  /**
   * @brief 刷新页面到磁盘之前，需要保证页面对应的日志也已经刷新到磁盘
   * @details 如果页面刷新到磁盘了，但是日志很落后，在重启恢复时，就会出现异常，无法让所有的页面都恢复到一致的状态。
   */
  RC flush_page(Page &page);

private:
  RC append_log(BufferPoolOperation::Type type, PageNum page_num, LSN &lsn);

private:
  BufferPool &buffer_pool_;
  LogHandler     &log_handler_;
};

class BufferPoolLogReplayer final : public LogReplayer
{
public:
  BufferPoolLogReplayer(BufferPoolManager &bp_manager);
  virtual ~BufferPoolLogReplayer() = default;

  ///! @copydoc LogReplayer::replay
  RC replay(const LogEntry &entry) override;

private:
  BufferPoolManager &bp_manager_;
};
