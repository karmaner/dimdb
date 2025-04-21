#include <unistd.h>
#include <fcntl.h>
#include <mutex>

#include "storage/buffer/double_write_buffer.h"
#include "common/log/log.h"
#include "common/io/io.h"
#include "common/math/crc.h"

struct DoubleWritePage {
public:
  DoubleWritePage() = default;
  DoubleWritePage(int32_t buffer_pool_id, PageNum page_num, int32_t page_index, Page& page);

public:
  DoubleWritePageKey key;
  int32_t            page_index = -1; /// 页面在double write buffer文件中的页索引
  bool               valid = true; /// 表示页面是否有效，在页面被删除时，需要同时标记磁盘上的值。
  Page               page;

  static const int32_t SIZE;
};

DoubleWritePage::DoubleWritePage(int32_t buffer_pool_id, PageNum page_num,int32_t page_index, 
  Page &_page) :key{buffer_pool_id, page_num}, page_index(page_index), page(_page) {}


const int32_t DoubleWritePage::SIZE = sizeof(DoubleWritePage);

const int32_t DoubleWriteBufferHeader::SIZE = sizeof(DoubleWriteBufferHeader);


/************************ DiskDoubleWriteBuffer ****************************/
DiskDoubleWriteBuffer::DiskDoubleWriteBuffer(BufferPoolManager &bp_manager, int max_pages /*=16*/) 
  : max_pages_(max_pages), bp_manager_(bp_manager) {}

DiskDoubleWriteBuffer::~DiskDoubleWriteBuffer() {
  flush_page();
  close(file_desc_);
}

RC DiskDoubleWriteBuffer::open_file(const std::string& filename) {
  if (file_desc_ >= 0) {
    LOG_ERROR("Double write buffer has already opened. file desc=%d", file_desc_);
    return RC::BUFFERPOOL_OPENED;
  }

  int fd = open(filename.c_str(), O_CREAT | O_RDWR, 0644);
  if (fd < 0) {
    LOG_ERROR("Failed to open or creat %s, due to %s.", filename.c_str(), strerror(errno));
    return RC::SCHEMA_DB_EXIST;
  }

  file_desc_ = fd;
  return load_pages();
}

RC DiskDoubleWriteBuffer::load_pages() {
  if (file_desc_ < 0) {
    LOG_ERROR("Failed to load pages, due to file desc is invalid.");
    return RC::BUFFERPOOL_OPENED;
  }

  if (!dblwr_pages_.empty()) {
    LOG_ERROR("Failed to load pages, due to double write buffer is not empty. opened?");
    return RC::BUFFERPOOL_OPENED;
  }

  if (lseek(file_desc_, 0, SEEK_SET) == -1) {
    LOG_ERROR("Failed to load page header, due to failed to lseek:%s.", strerror(errno));
    return RC::IOERR_SEEK;
  }

  int ret = readn(file_desc_, &header_, sizeof(header_));
  if (ret != 0 && ret != -1) {
    LOG_ERROR("Failed to load page header, file_desc:%d, due to failed to read data:%s, ret=%d",
      file_desc_, strerror(errno), ret);
    return RC::IOERR_READ;
  }

  for (int page_num = 0; page_num < header_.page_cnt; page_num++) {
    int64_t offset = ((int64_t)page_num) * DoubleWritePage::SIZE + DoubleWriteBufferHeader::SIZE;

    if (lseek(file_desc_, offset, SEEK_SET) == -1) {
      LOG_ERROR("Failed to load page %d, offset=%ld, due to failed to lseek:%s.",
        page_num, offset, strerror(errno));
      return RC::IOERR_SEEK;
    }

    auto dblwr_page = std::make_unique<DoubleWritePage>();
    Page &page     = dblwr_page->page;
    page.header.check_sum = (CheckSum)-1;

    ret = readn(file_desc_, dblwr_page.get(), DoubleWritePage::SIZE);
    if (ret != 0) {
      LOG_ERROR("Failed to load page, file_desc:%d, page num:%d, due to failed to read data:%s, ret=%d, page count=%d",
        file_desc_, page_num, strerror(errno), ret, page_num);
      return RC::IOERR_READ;
    }

    const CheckSum check_sum = crc32(page.data, BP_PAGE_DATA_SIZE);
    if (check_sum == page.header.check_sum) {
      DoubleWritePageKey key = dblwr_page->key;
      dblwr_pages_.insert(std::pair<DoubleWritePageKey, DoubleWritePage *>(key, dblwr_page.release()));
    } else {
      LOG_TRACE("got a page with an invalid checksum. on disk:%d, in memory:%d", page.header.check_sum, check_sum);
    }
  }

  LOG_INFO("double write buffer load pages done. page num=%ld", dblwr_pages_.size());
  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::flush_page()
{
  sync();

  for (const auto &pair : dblwr_pages_) {
    RC rc = write_page(pair.second);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    pair.second->valid = false;
    write_page_internal(pair.second);
    delete pair.second;
  }

  dblwr_pages_.clear();
  header_.page_cnt = 0;

  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::add_page(BufferPool* bp, PageNum page_num, Page& page)
{
  DoubleWritePageKey key{bp->id(), page_num};
  auto iter = dblwr_pages_.find(key);
  if (iter != dblwr_pages_.end()) {
    iter->second->page = page;
    LOG_TRACE("[cache hit]add page into double write buffer. buffer_pool_id:%d,page_num:%d,lsn=%ld, dwb size=%d",
      bp->id(), page_num, page.lsn, static_cast<int>(dblwr_pages_.size()));
    return write_page_internal(iter->second);
  }

  int64_t          page_cnt   = dblwr_pages_.size();
  DoubleWritePage *dblwr_page = new DoubleWritePage(bp->id(), page_num, page_cnt, page);
  dblwr_pages_.insert(std::pair<DoubleWritePageKey, DoubleWritePage *>(key, dblwr_page));
  LOG_TRACE("insert page into double write buffer. buffer_pool_id:%d,page_num:%d,lsn=%d, dwb size:%d",
    bp->id(), page_num, page.lsn, static_cast<int>(dblwr_pages_.size()));

  RC rc = write_page_internal(dblwr_page);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to write page into double write buffer. rc=%s buffer_pool_id:%d,page_num:%d,lsn=%d.",
        strrc(rc), bp->id(), page_num, page.lsn);
    return rc;
  }

  if (page_cnt + 1 > header_.page_cnt) {
    header_.page_cnt = page_cnt + 1;
    if (lseek(file_desc_, 0, SEEK_SET) == -1) {
      LOG_ERROR("Failed to add page header due to failed to seek %s.", strerror(errno));
      return RC::IOERR_SEEK;
    }

    if (writen(file_desc_, &header_, sizeof(header_)) != 0) {
      LOG_ERROR("Failed to add page header due to %s.", strerror(errno));
      return RC::IOERR_WRITE;
    }
  }

  if (static_cast<int>(dblwr_pages_.size()) >= max_pages_) {
    RC rc = flush_page();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to flush pages in double write buffer");
      return rc;
    }
  }

  return RC::SUCCESS;
}