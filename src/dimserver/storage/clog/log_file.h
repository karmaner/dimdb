#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <filesystem>
#include <map>
#include "common/types.h"
#include "common/rc.h"


class LogEntry;

/**
 * @brief 日志文件读取器类
 * 用于读取日志文件中的日志条目
 */
class LogFileReader {
public:
  LogFileReader() = default;
  ~LogFileReader() = default;

  /**
   * @brief 打开日志文件
   * @param filename 日志文件名
   * @return 返回操作结果
   */
  RC open(const std::string& filename);
  RC close();
  
  /**
   * @brief 遍历日志文件中的日志条目
   * @param callback 回调函数，处理每个日志条目
   * @param start_lsn 起始日志序列号
   * @return 返回操作结果
   */
  RC iterate(std::function<RC(LogEntry&)> callback, LSN start_lsn = 0);

private:
  /**
   * @brief 跳转到指定LSN的日志条目
   * @param lsn 期望开始的第一条日志的LSN
   * @return RC::SUCCESS: 成功, 其他: 失败
   */
  RC go_to(LSN lsn); 
  
private:
  std::string  m_filename; // 文件名
  int          m_fd = -1;  // 文件描述符
};


/**
 * @brief 日志文件写入器类
 * 用于向日志文件写入日志条目
 */
class LogFileWriter {
public:
  LogFileWriter() = default;
  ~LogFileWriter();

  /**
   * @brief 打开日志文件
   * @param filename 日志文件名
   * @param end_lsn 文件允许的最大LSN
   * @return 返回操作结果
   */
  RC open(const std::string& filename, LSN end_lsn);
  RC close();

  /**
   * @brief 写入日志条目
   * @param entry 要写入的日志条目
   * @return 返回操作结果
   */
  RC write(const LogEntry& entry);

  /**
   * @brief 检查文件是否已打开
   * @return true: 已打开, false: 未打开
   */
  bool is_open() const;
  bool is_full() const;
  std::string to_string() const;

  /**
   * @brief 获取文件名
   * @return 返回文件名
   */
  const char* filename() const { return m_filename.c_str(); }

private:
  std::string  m_filename;       // 文件名
  int          m_fd = -1;        // 文件描述符
  LSN          m_last_lsn = 0;   // 写入的最后一个LSN
  LSN          m_end_lsn = 0;    // 文件允许的最大LSN
};


/**
 * @brief 日志文件管理器类
 * 用于管理日志文件的创建、删除和查询
 */
class LogFileManager {
public:
  LogFileManager() = default;
  ~LogFileManager() = default;

  /**
   * @brief 初始化日志文件管理器
   * @param dir 日志文件目录
   * @param max_file_count 最大文件数量
   * @return 返回操作结果
   */
  RC init(const std::string& dir, int max_file_count);

  /**
   * @brief 列出日志文件
   * @param files 输出参数，存储文件列表
   * @param start_lsn 起始日志序列号
   * @return 返回操作结果
   */
  RC list_files(std::vector<std::string>& files, LSN start_lsn = 0);

  /**
   * @brief 获取最后一个日志文件
   * @param writer 输出参数，存储日志文件写入器
   * @return 返回操作结果
   */
  RC last_file(LogFileWriter& writer);
  RC next_file(LogFileWriter& writer);
private:
  /**
   * @brief 从文件名中提取LSN
   * @param filename 日志文件名
   * @param lsn 输出参数，存储提取的LSN
   * @return 返回操作结果
   */
  static RC get_lsn_from_filename(const std::string& filename, LSN& lsn);
private:
  static constexpr std::string_view CLOG_FILE_PREFIX = "clog_";
  static constexpr std::string_view CLOG_FILE_SUFFIX = ".log";

  std::filesystem::path        m_dir;
  int32_t                      max_entry_number_per_file_;

  std::map<LSN, std::filesystem::path> m_log_files;
};
