#pragma once

#include <string>
#include <memory>
#include <functional>
#include <string_view>

#include "storage/clog/log_buffer.h"
#include "storage/clog/log_file.h"
#include "common/types.h"
#include "common/rc.h"

class LogEntry;
class LogReplayer;

/**
 * @brief 日志处理器抽象类
 * 定义了日志系统的核心接口，包括日志的写入、读取和回放等功能
 */
class LogHandler {
public:
  LogHandler()          = default;
  virtual ~LogHandler() = default;

  /**
   * @brief 初始化日志处理器
   * @param dir 日志文件目录
   * @return 返回操作结果
   */
  virtual RC init(const std::string& dir) = 0;

  /**
   * @brief 启动日志处理器
   * 开始接收和处理日志
   * @return 返回操作结果
   */
  virtual RC start() = 0;

  /**
   * @brief 停止日志处理器
   * 停止接收和处理日志
   * @return 返回操作结果
   */
  virtual RC stop() = 0;

  /**
   * @brief 等待日志处理器终止
   * 等待所有日志处理完成
   * @return 返回操作结果
   */
  virtual RC await_termination() = 0;

  /**
   * @brief 回放日志
   * @param replayer 日志回放器
   * @param start_lsn 起始日志序列号
   * @return 返回操作结果
   */
  virtual RC replay(LogReplayer& replayer, LSN start_lsn) = 0;

  /**
   * @brief 遍历日志
   * @param consumer 日志消费者回调函数
   * @param start_lsn 起始日志序列号
   * @return 返回操作结果
   */
  virtual RC iterate(std::function<RC(LogEntry&)> consumer, LSN start_lsn) = 0;

  /**
   * @brief 追加日志（使用string_view）
   * @param lsn 输出参数，返回分配的日志序列号
   * @param module_id 日志模块ID
   * @param data 日志数据
   * @return 返回操作结果
   */
  virtual RC append(LSN& lsn, LogModule::Id module_id, std::string_view data);

  /**
   * @brief 追加日志（使用vector）
   * @param lsn 输出参数，返回分配的日志序列号
   * @param module_id 日志模块ID
   * @param data 日志数据
   * @return 返回操作结果
   */
  virtual RC append(LSN& lsn, LogModule::Id module_id, std::vector<char>&& data);

  /**
   * @brief 追加日志（使用LogModule）
   * @param lsn 输出参数，返回分配的日志序列号
   * @param module 日志模块
   * @param data 日志数据
   * @return 返回操作结果
   */
  virtual RC append(LSN& lsn, LogModule module, std::vector<char>&& data);

  /**
   * @brief 等待指定LSN的日志被处理
   * @param lsn 要等待的日志序列号
   * @return 返回操作结果
   */
  virtual RC wait_lsn(LSN lsn) = 0;

  /**
   * @brief 获取当前LSN
   * @return 返回当前日志序列号
   */
  virtual LSN current_lsn() const = 0;

  /**
   * @brief 创建日志处理器实例
   * @param name 日志处理器名称
   * @param handler 输出参数，返回创建的日志处理器
   * @return 返回操作结果
   */
  static RC create(const std::string& name, LogHandler*& handler);

private:
  /**
   * @brief 内部追加日志实现
   * @param lsn 输出参数，返回分配的日志序列号
   * @param module 日志模块
   * @param data 日志数据
   * @return 返回操作结果
   */
  virtual RC _append(LSN& lsn, LogModule module, std::vector<char>&& data) = 0;
}; 