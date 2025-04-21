#pragma once

#include <string>
#include <list>
#include <mutex>
#include <functional>
#include <unordered_map>

#include "common/rc.h"
#include "common/types.h"

#include "common/bitmap/bitmap.h"
#include "common/mem/mem_pool.h"
#include "storage/buffer/lru_cache.h"
#include "storage/buffer/frame.h"
#include "storage/buffer/page.h"
#include "storage/buffer/double_write_buffer.h"
#include "storage/buffer/buffer_pool_log.h"

namespace storage {
/**
 * @brief 管理页面Frame
 * @ingroup BufferPool
 * @details 管理内存中的页帧。内存是有限的，内存中能够存放的页帧个数也是有限的。
 * 当内存中的页帧不够用时，需要从内存中淘汰一些页帧，以便为新的页帧腾出空间。
 * 这个管理器负责为所有的BufferPool提供页帧管理服务，也就是所有的BufferPool磁盘文件
 * 在访问时都使用这个管理器映射到内存。
 */
class FrameManager {
public:
	FrameManager(const std::string& tag);
	~FrameManager();

	RC init(int pool_num);
	RC cleanup();

	Frame* get(int buffer_pool_id, PageNum page_num);
	std::list<Frame*> find_list(int buffer_pool_id);

	Frame* alloc(int buffer_pool_id, PageNum page_num);
	RC free(int buffer_pool_id, PageNum page_num, Frame* frame);
	int purge_frames(int count, std::function<RC(Frame*)> purger);

	size_t frame_num() const;
	size_t total_frame_num() const;

private:
	Frame* get_internal(const FrameId& frame_id);
	RC free_internal(const FrameId& frame_id, Frame* frame);

	struct FrameIdHash {
		size_t operator()(const FrameId& frame_id) const {
			return frame_id.hash();
		}
	};

	std::mutex mutex_;
	LruCache<FrameId, Frame*, FrameIdHash> frames_; // 采用LRU缓存
	MemPoolSimple<Frame> allocator_; // 采用内存池
};

struct BPFileHeader
{
  int32_t buffer_pool_id;   //! buffer pool id
  int32_t page_count;       //! 当前文件一共有多少个页面
  int32_t allocated_pages;  //! 已经分配了多少个页面
  char    bitmap[0];        //! 页面分配位图, 第0个页面(就是当前页面)，总是1

  static const int MAX_PAGE_NUM = (BP_PAGE_DATA_SIZE - sizeof(page_count) - sizeof(allocated_pages)) * 8;

  std::string to_string() const;
};

class BufferPool final {
public:
	BufferPool(BufferPoolManager& bp_manager, FrameManager& frame_manager,
		DoubleWriteBuffer& dblwr_manager, LogHandler& lgo_handler);
	~BufferPool();

	RC open_file(const std::string& file_name);

	RC close_file();

	RC get_this_page(PageNum page_num, Frame** frame);

	RC allocate_page(Frame** frame);

	RC dispose_page(PageNum page_num);

	RC purge_page(PageNum page_num);
	RC purge_all_page();

	RC unpin_page(Frame* farme);
	RC check_all_pages_unpinned();

	int file_desc() const;

	RC flush_page(Frame& frame);
	RC flush_all_pages();

	RC recover_page(PageNum page_num);

	RC write_page(PageNum page_num, Page &page);

  RC redo_allocate_page(LSN lsn, PageNum page_num);
  RC redo_deallocate_page(LSN lsn, PageNum page_num);

public:
	int32_t id() const { return buffer_pool_id_; }

	std::string filename() const { return filename_; }

protected:
  RC allocate_frame(PageNum page_num, Frame **buf);

  RC purge_frame(PageNum page_num, Frame *used_frame);
  RC check_page_num(PageNum page_num);

  RC load_page(PageNum page_num, Frame *frame);

  RC flush_page_internal(Frame &frame);

private:
  BufferPoolManager   &bp_manager_;     /// BufferPool 管理器
  FrameManager      &frame_manager_;  /// Frame 管理器
  DoubleWriteBuffer   &dblwr_manager_;  /// Double Write Buffer 管理器
  BufferPoolLogHandler log_handler_;    /// BufferPool 日志处理器

  int fd_ = -1;  /// 文件描述符
  /// 由于在最开始打开文件时，没有正确的buffer pool id不能加载header frame，所以单独从文件中读取此标识
  int32_t       buffer_pool_id_ = -1;
  Frame        *hdr_frame_      = nullptr;  /// 文件头页面
  BPFileHeader *file_header_    = nullptr;  /// 文件头
  std::set<PageNum>  disposed_pages_;            /// 已经释放的页面

  std::string filename_;  /// 文件名

private:
  friend class BufferPoolIterator;
};

class BufferPoolManager final {
public:

private:
	FrameManager frame_manager_{"BufferPool"};

	std::unique_ptr<DoubleWriteBuffer> dbwr_buffer_;

	std::unordered_map<std::string, BufferPool*> buffer_pools_;
	std::unordered_map<int32_t, BufferPool*> id_to_buffer_pools_;
	std::atomic<int32_t> next_buffer_pool_id{1};
};

class BufferPoolIterator {
public:
	BufferPoolIterator();
  ~BufferPoolIterator();

  RC      init(BufferPool &bp, PageNum start_page = 0);
  bool    has_next();
  PageNum next();
  RC      reset();
private:
	Bitmap bitmpa_;
	PageNum current_page_num_ = -1;
};

} // namespace storage
