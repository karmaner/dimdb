#pragma once

#include "storage/clog/log_handler.h"

/**
 * @brief 空日志处理器类
 * 实现了LogHandler接口的空操作版本，所有操作都直接返回成功
 * 主要用于测试或不需要实际日志功能的场景
 * 
 * @ingroup CLog
 */
class VacuousLogHandler : public LogHandler
{
public:
  VacuousLogHandler()          = default;
  virtual ~VacuousLogHandler() = default;

  RC init(const std::string& dir) override { return RC::SUCCESS; }
  RC start() override { return RC::SUCCESS; }
  RC stop() override { return RC::SUCCESS; }
  RC await_termination() override { return RC::SUCCESS; }
  RC replay(LogReplayer &replayer, LSN start_lsn) override { return RC::SUCCESS; }
  RC iterate(std::function<RC(LogEntry&)> consumer, LSN start_lsn) override { return RC::SUCCESS; }

  RC wait_lsn(LSN lsn) override { return RC::SUCCESS; }

  LSN current_lsn() const override { return 0; }

private:
  RC _append(LSN &lsn, LogModule module, std::vector<char>&& data) override
  {
    lsn = 0;
    return RC::SUCCESS;
  }
};