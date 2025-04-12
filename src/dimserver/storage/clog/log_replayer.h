#pragma once

#include <string>
#include "common/rc.h"

class LogEntry;

class LogReplayer {
public:
  LogReplayer()          = default;
  virtual ~LogReplayer() = default;


  virtual RC replay(const LogEntry& entry) = 0;

  virtual RC on_done() { return RC::SUCCESS; }

};