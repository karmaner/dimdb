#include "storage/buffer/buffer_pool.h"


namespace storage {

/********** FrameManager ************/
FrameManager::FrameManager(const std::string& tag) : frames_(0), allocator_(tag) { }

FrameManager::~FrameManager() {
	allocator_.cleanup();
}

RC FrameManager::init(int pool_num) {
	int ret = allocator_.init(false, pool_num, 1);
	if (ret != 0) {
		LOG_ERROR("Failed to initialize frame manager, ret:%d.", ret);
		return RC::NO_MEM_POOL;
	}
	return RC::SUCCESS;
}

RC FrameManager::cleanup() {
  if (frames_.count() > 0) {
    LOG_ERROR("There are still frames in the frame manager, cannot cleanup.");
    return RC::NO_MEM_POOL;
  }
  frames_.destroy();
  return RC::SUCCESS;
}

Frame* FrameManager::get(int buffer_pool_id, PageNum page_num) {
  FrameId frame_id(buffer_pool_id, page_num);
  std::lock_guard lock_guard(mutex_);
  return get_internal(frame_id);
}

Frame* FrameManager::get_internal(const FrameId &frame_id) {
  Frame* frame = nullptr;
  (void)frames_.get(frame_id, frame);
  if (frame != nullptr) {
    frame->pin();
  }
  return frame;
}

std::list<Frame*> FrameManager::find_list(int buffer_pool_id) {
  std::lock_guard lock(mutex_);

  std::list<Frame*> frames;
  auto func = [&frames, buffer_pool_id]([[maybe_unused]] const FrameId& frame_id, Frame* const frame) {
    if (frame->buffer_pool_id() == buffer_pool_id) {
      frame->pin();
      frames.push_back(frame);
    }
    return true;
  };
  frames_.foreach(func);
  return frames;
}

Frame* FrameManager::alloc(int buffer_pool_id, PageNum page_num) {
  FrameId frame_id(buffer_pool_id, page_num);
  Frame* frame = nullptr;
  std::lock_guard lock(mutex_);
  frame = allocator_.alloc();
  if (frame != nullptr) {
    ASSERT(frame->pin_count() == 0, "Frame is already pinned. frame=%s", frame->to_string().c_str());
    frame->set_buffer_pool_id(buffer_pool_id);
    frame->set_page_num(page_num);
    frame->pin();
    frames_.put(frame_id, frame);
  }
  return frame;
}

RC FrameManager::free(int buffer_pool_id, PageNum page_num, Frame* frame) {
  FrameId frame_id(buffer_pool_id, page_num);

  std::lock_guard lock(mutex_);
  return free_internal(frame_id, frame);
}

RC FrameManager::free_internal(const FrameId& frame_id, Frame* frame) {
  Frame* out = nullptr;
  bool ret = frames_.get(frame_id, out);

  ASSERT(ret && frame == out && frame->pin_count() == 1,
    "failed to free frame. found=%d, frameId=%s, frame_source=%p, frame=%p, pinCount=%d, lbt=%s",
    ret, frame_id.to_string().c_str(), out, frame, frame->pin_count(), lbt());

  frame->set_page_num(-1);
  frame->unpin();
  frames_.remove(frame_id);
  allocator_.free(frame);
  return RC::SUCCESS;
}

int FrameManager::purge_frames(int count, std::function<RC(Frame*)> purger) {
  if (count <= 0) count = 1;
  std::lock_guard lock(mutex_);

  std::vector<Frame*> can_purge_frame;
  can_purge_frame.reserve(count); // 预分配

  auto purge_finder = [&can_purge_frame, count]([[maybe_unused]] const FrameId& frame_id, Frame* const frame) {
    if (frame->can_purge()) {
      frame->pin();
      can_purge_frame.push_back(frame);
      if (can_purge_frame.size() >= static_cast<size_t>(count)) {
        return false;
      }
    }
    return true;
  };

  frames_.foreach_reverse(purge_finder);
  LOG_INFO("purge frames find %ld pages total", can_purge_frame.size());

  int freed_count = 0;
  for (Frame* frame : can_purge_frame) {
    RC rc = purger(frame);
    if (rc == RC::SUCCESS) {
      free_internal(frame->frame_id(), frame);
      freed_count++;
    } else {
      frame->unpin();
      LOG_WARN("failed to purge frame. frame_id=%s, rc=%s", 
        frame->frame_id().to_string().c_str(), strrc(rc));
    }
  }
  LOG_INFO("purge frame done. number=%d", freed_count);
  return freed_count;
}

} // namespace storage
