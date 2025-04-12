#include <cstdint>
#include <sstream>
#include "storage/clog/log_entry.h"
#include "common/log/log.h"


const int32_t LogHeader::HEAD_SIZE = sizeof(LogHeader);

std::string LogHeader::to_string() const {
  std::ostringstream  oss;
  oss << "lsn=" << lsn
      << ",size=" << data_size
      << ",module_id=" << module_id
      << ",module_name=" << LogModule(module_id).name();
  return oss.str();
}


LogEntry::LogEntry() = default;

LogEntry::LogEntry(LogEntry&& other) noexcept 
  : m_header(other.m_header),
    m_data(std::move(other.m_data)) {
  // 源对象的header会自动重置为默认值
  other.m_header = {};
}

LogEntry& LogEntry::operator=(LogEntry&& other) noexcept {
  if (this != &other) {
    m_header = other.m_header;
    m_data = std::move(other.m_data);
    other.m_header = {};
  }
  return *this;
}

RC LogEntry::init(LSN lsn, LogModule::Id module_id, std::vector<char>&& data) {
  return init(lsn, LogModule(module_id), std::move(data));
}

RC LogEntry::init(LSN lsn, LogModule module, std::vector<char>&& data) {
  if (static_cast<int32_t>(data.size()) > max_payload_size()) {
    LOG_DEBUG("log entry data size(%zu) is too large", data.size());
    return RC::MESSAGE_INVAID;
  }

  m_header.lsn = lsn;
  m_header.data_size = static_cast<int32_t>(data.size());
  m_header.module_id = static_cast<int32_t>(module.index());
  m_data = std::move(data);
  return RC::SUCCESS;
}

std::string LogEntry::to_string() const {
  return header().to_string() + ",data=" + std::string(data(), payload_size());
}



