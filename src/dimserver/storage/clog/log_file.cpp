#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <string_view>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cstring>
#include <algorithm>

#include "storage/clog/log_file.h"
#include "common/log/log.h"
#include "storage/clog/log_entry.h"
#include "common/io/io.h"
#include "common/utils/utils.h"


/******************** LogFileReader ********************/

RC LogFileReader::open(const std::string& filename) {

  m_filename = filename;
  m_fd = ::open(filename.c_str(), O_RDONLY);
  if (m_fd < 0) {
    LOG_ERROR("open clog file failed. filename=%s, errno=%d, errmsg=%s", 
      filename.c_str(), errno, strerror(errno));
    return RC::FILE_NOT_FOUND;
  }

  LOG_INFO("open file success. filename=%s, fd=%d", filename.c_str(), m_fd);
  return RC::SUCCESS;
}

RC LogFileReader::close() {
  if (m_fd < 0) {
    return RC::FILE_NOT_FOUND;
  }

  ::close(m_fd);
  m_fd = -1;
  return RC::SUCCESS;
}

RC LogFileReader::go_to(LSN lsn) {
  if (m_fd < 0) {
    LOG_ERROR("clog file not opened.");
    return RC::FILE_NOT_FOUND;
  }

  // 定位到文件开头
  off_t pos = ::lseek(m_fd, 0, SEEK_SET);
  if (pos == off_t(-1)) {
    LOG_ERROR("seek file failed. seek to the beginning. filename=%s, error=%s", 
      m_filename.c_str(), strerror(errno));
    return RC::IOERR_SEEK;
  }

  // 从文件开头开始查找
  LogHeader header;
  while (true) {
    // 读取日志头
    int rc = readn(m_fd, reinterpret_cast<char *>(&header), LogHeader::HEAD_SIZE);
    if (rc != 0) {
      if (rc == -1) {
        // 文件结束
        break;
      }
      LOG_ERROR("read file failed. filename=%s, ret=%d, error=%s", 
        m_filename.c_str(), rc, strerror(errno));
      return RC::IOERR_READ;
    }

    // 找到目标LSN
    if (header.lsn >= lsn) {
      // 回退到日志头开始位置
      pos = ::lseek(m_fd, -LogHeader::HEAD_SIZE, SEEK_CUR);
      if (pos == off_t(-1)) {
        LOG_ERROR("seek file failed. skip back log header. filename=%s, error=%s", 
          m_filename.c_str(), strerror(errno));
        return RC::IOERR_SEEK;
      }
      break;
    }

    // 检查日志头数据区是否有效
    if (header.data_size < 0 || header.data_size > LogEntry::max_payload_size()) {
      LOG_ERROR("invalid log entry size. filename=%s, size=%d", 
        m_filename.c_str(), header.data_size);
      return RC::IOERR_READ;
    }

    // 跳过日志体
    pos = ::lseek(m_fd, LogHeader::HEAD_SIZE, SEEK_CUR);
    if (pos == off_t(-1)) {
      LOG_ERROR("seek file failed. skip log entry payload. filename=%s, error=%s", 
        m_filename.c_str(), strerror(errno));
      return RC::IOERR_SEEK;
    }
  }

  return RC::SUCCESS;
}

RC LogFileReader::iterate(std::function<RC(LogEntry&)> callback, LSN start_lsn) {
  if (m_fd < 0) {
    LOG_ERROR("log file not opened");
    return RC::FILE_NOT_FOUND;
  }

  RC rc = go_to(start_lsn);
  if (IS_FAIL(rc)) {
    return rc;
  }

  LogHeader header;
  while (true) {
    int ret = readn(m_fd, reinterpret_cast<char*> (&header), LogHeader::HEAD_SIZE);
    if (ret != 0) {
      if (-1 == ret) {
        break;
      }
      LOG_ERROR("read file faild. filename=%s, ret=%d, error=%s",
        m_filename.c_str(), ret, strerror(errno));
      return RC::IOERR_READ;
    }


    // 读取日志体
    std::vector<char> data(header.data_size);
    ret = readn(m_fd, data.data(), header.data_size);
    if (0 != ret) {
      LOG_WARN("read file faild. filename=%s, size=%d, ret=%d, error=%s",
        m_filename.c_str(), header.data_size, ret, strerror(errno));
      return RC::IOERR_READ;
    }

    // 创建日志条目
    LogEntry entry;
    entry.init(header.lsn, LogModule(header.module_id), std::move(data));

    RC rc = callback(entry);
    if (IS_FAIL(rc)) {
      LOG_INFO("iterate log entry failed. entry=%s, rc=%s", entry.to_string().c_str(), strrc(rc));
      return rc;
    }
    LOG_TRACE("redo log iterate entry success. entry=%s", entry.to_string().c_str());
  }

  return RC::SUCCESS;
}


/******************** LogFileWriter ********************/

LogFileWriter::~LogFileWriter() {
  (void)this->close();
}

RC LogFileWriter::open(const std::string& filename, LSN end_lsn) {
  if (m_fd >= 0) {
    LOG_WARN("log file %s already opened", filename.c_str());
    return RC::FILE_OPEND;
  }

  m_fd = ::open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (m_fd < 0) {
    LOG_ERROR("failed to open log file %s, errno=%d", filename.c_str(), errno);
    return RC::FILE_NOT_FOUND;
  }

  m_filename = filename;
  this->m_end_lsn = end_lsn;
  return RC::SUCCESS;
}

RC LogFileWriter::close() {
  if (m_fd < 0) {
    return RC::SUCCESS;
  }

  ::close(m_fd);

  m_fd = -1;
  m_filename.clear();
  return RC::SUCCESS;
}

RC LogFileWriter::write(const LogEntry& entry) {
  if (m_fd < 0) {
    LOG_ERROR("log file not open.");
    return RC::FILE_NOT_OPEN;
  }

  if (entry.lsn() >= m_end_lsn) {
    LOG_ERROR("log file is full, lsn=%ld, end_lsn=%ld", entry.lsn(), m_end_lsn);
    return RC::FILE_FULL;
  }

  // 写入日志头
  int ret = writen(m_fd, reinterpret_cast<const char *>(&entry.header()), LogHeader::HEAD_SIZE);
  if (0 != ret) {
    LOG_WARN("write log entry header faidl. filename=%s, ret=%d, error=%s, entry=%s",
      m_filename.c_str(), ret, strerror(errno), entry.to_string().c_str());
    return RC::IOERR_WRITE;
  }

  ret = writen(m_fd, entry.data(), entry.payload_size());
  if (0 != ret) {
    LOG_WARN("write log entry data faild. filename=%s, ret=%d, error=%s, entry=%s",
      m_filename.c_str(), ret, strerror(errno), entry.to_string().c_str());
    return RC::IOERR_WRITE;
  }

  m_last_lsn = entry.lsn();
  return RC::SUCCESS;
}

bool LogFileWriter::is_open() const {
  return m_fd >= 0;
}

bool LogFileWriter::is_full() const {
  return m_last_lsn >= m_end_lsn;
}

std::string LogFileWriter::to_string() const {
  return std::string("LogFileWriter(filename=") + m_filename + 
    ", last_lsn=" + std::to_string(m_last_lsn) + 
    ", end_lsn=" + std::to_string(m_end_lsn) + ")";
}


/******************** LogFileManager ********************/

RC LogFileManager::get_lsn_from_filename(const std::string& filename, LSN& lsn) {

  if (!starts_with(filename, LogFileManager::CLOG_FILE_PREFIX) ||
    !ends_with(filename, LogFileManager::CLOG_FILE_SUFFIX)) {
    return RC::FILE_NAME_INVALID;
  }

  // 提取LSN部分：去掉前缀和后缀
  std::string_view lsn_str = filename;
  lsn_str.remove_prefix(LogFileManager::CLOG_FILE_PREFIX.size());
  lsn_str.remove_suffix(LogFileManager::CLOG_FILE_SUFFIX.size());

  try {
    lsn = std::stol(std::string(lsn_str));
    return RC::SUCCESS;
  } catch (const std::exception&) {
    return RC::FILE_NAME_INVALID;
  }
}

RC LogFileManager::init(const std::string& dir, int max_file_count) {
  m_dir = dir;
  max_entry_number_per_file_ = max_file_count;

  // 创建目录
  if (!std::filesystem::exists(m_dir)) {
    if (!std::filesystem::create_directories(m_dir)) {
      LOG_ERROR("create log directory %s failed.", m_dir.c_str());
      return RC::FILE_CREATE_ERR;
    }
  }

  // 扫描目录中的日志文件
  for (const auto& file : std::filesystem::directory_iterator(m_dir)) {
    if (!file.is_regular_file()) {
      continue;
    }

    std::string filename = file.path().filename().string();
    if (!starts_with(filename, CLOG_FILE_PREFIX) || 
      !ends_with(filename, CLOG_FILE_SUFFIX)) {
      continue;
    }

    LSN lsn;
    RC rc = get_lsn_from_filename(filename, lsn);
    if (IS_FAIL(rc)) {
      LOG_WARN("invalid log file name %s", filename.c_str());
      continue;
    }

    m_log_files[lsn] = file.path();
  }

  return RC::SUCCESS;
}

RC LogFileManager::list_files(std::vector<std::string>& files, LSN start_lsn) {
  files.clear();
  for (const auto& [lsn, path] : m_log_files) {
    if (lsn >= start_lsn) {
      files.push_back(path.string());
    }
  }
  return RC::SUCCESS;
}

RC LogFileManager::last_file(LogFileWriter& writer) {
  if (m_log_files.empty()) {
    return RC::FILE_NOT_FOUND;
  }

  auto it = m_log_files.rbegin();
  std::string filename = it->second.string();
  return writer.open(filename, it->first + max_entry_number_per_file_);
}

RC LogFileManager::next_file(LogFileWriter& writer) {
  writer.close();

  LSN next_lsn = 0;
  if (!m_log_files.empty()) {
    next_lsn = m_log_files.rbegin()->first + max_entry_number_per_file_;
  }

  std::string filename = m_dir.string()
    + "/"
    + std::string(LogFileManager::CLOG_FILE_PREFIX)
    + std::to_string(next_lsn)
    + std::string(LogFileManager::CLOG_FILE_SUFFIX);
  return writer.open(filename, next_lsn + max_entry_number_per_file_);
}