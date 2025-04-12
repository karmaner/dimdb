#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common/types.h"
#include "common/rc.h"
#include "storage/clog/log_module.h"

/**
 * @brief 日志头结构体
 * 包含日志的基本元数据信息
 */
struct LogHeader final {
  LSN         lsn{0};        // 日志序列号
  int32_t     data_size{0};  // 日志数据大小（不包含header）
  int32_t     module_id{0};  // 日志模块ID

  static const int32_t HEAD_SIZE; // header的大小

  /**
   * @brief 转换为字符串表示
   * @return 返回日志头的字符串表示
   */
  std::string to_string() const;
};

/**
 * @brief 日志条目类
 * 表示一条完整的日志记录，包含日志头和日志数据
 */
class LogEntry {
public:
  // 构造和析构函数
  LogEntry();
  ~LogEntry() = default;

  // 移动语义支持
  LogEntry(LogEntry&& other) noexcept;
  LogEntry& operator=(LogEntry&& other) noexcept;

  // 禁用拷贝
  LogEntry(LogEntry& other) = delete;
  LogEntry& operator=(LogEntry& other) = delete;

  // 静态配置
  /**
   * @brief 获取最大日志大小
   * @return 返回最大日志大小（4MB）
   */
  static int32_t max_size() { return 4 * 1024 * 1024; }
  
  /**
   * @brief 获取最大数据大小
   * @return 返回最大数据大小（不包含header）
   */
  static int32_t max_payload_size() { return max_size() - LogHeader::HEAD_SIZE; }

public:
  /**
   * @brief 初始化日志条目
   * @param lsn 日志序列号
   * @param module_id 日志模块ID
   * @param data 日志数据
   * @return 返回操作结果
   */
  RC init(LSN lsn, LogModule::Id module_id, std::vector<char>&& data);
  
  /**
   * @brief 初始化日志条目
   * @param lsn 日志序列号
   * @param module 日志模块
   * @param data 日志数据
   * @return 返回操作结果
   */
  RC init(LSN lsn, LogModule module, std::vector<char>&& data);

  // 数据访问接口
  /**
   * @brief 获取日志头
   * @return 返回日志头引用
   */
  const LogHeader& header() const { return m_header; }
  
  /**
   * @brief 获取日志数据
   * @return 返回日志数据指针
   */
  const char* data() const { return m_data.data(); }
  
  /**
   * @brief 获取数据大小
   * @return 返回数据大小
   */
  int32_t payload_size() const { return m_header.data_size; }
  
  /**
   * @brief 获取总大小
   * @return 返回总大小（包含header）
   */
  int32_t total_size() const { return m_header.data_size + LogHeader::HEAD_SIZE; }

  // LSN相关操作
  /**
   * @brief 设置日志序列号
   * @param lsn 新的日志序列号
   */
  void set_lsn(LSN lsn) { m_header.lsn = lsn; }
  
  /**
   * @brief 获取日志序列号
   * @return 返回日志序列号
   */
  LSN lsn() const { return m_header.lsn; }
  
  /**
   * @brief 获取日志模块
   * @return 返回日志模块
   */
  LogModule module() const { return LogModule(header().module_id); }

  /**
   * @brief 转换为字符串表示
   * @return 返回日志条目的字符串表示
   */
  std::string to_string() const;

private:
  LogHeader m_header;        // 日志头
  std::vector<char> m_data;  // 日志数据
};