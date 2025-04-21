#include "storage/buffer/frame.h"

namespace storage {

bool FrameId::operator==(const FrameId& other) const {
  return buffer_pool_id == other.buffer_pool_id && page_num == other.page_num;
}

bool FrameId::operator!=(const FrameId& other) const {
  return !(*this == other);
}

bool FrameId::operator<(const FrameId& other) const {
  if (buffer_pool_id != other.buffer_pool_id) {
    return buffer_pool_id < other.buffer_pool_id;
  }
  return page_num < other.page_num;
}

bool FrameId::is_valid() const {
  return buffer_pool_id >= 0 && page_num != BP_INVALID_PAGE_NUM;
}

size_t FrameId::hash() const {
  return static_cast<size_t>(buffer_pool_id) << 32L | page_num;
}

std::string FrameId::to_string() const {
  std::stringstream ss;
  ss << "buffer_pool_id:" << buffer_pool_id << ",page_num:" << page_num;
  return ss.str();
}

// Frame实现
Frame::Frame() : 
  pin_count_(0), 
  acc_time_(0) {
  page_.init();
}

Frame::~Frame() {
  LOG_DEBUG("deallocate frame, this=%p, lbt=%s", this, common::stacktrace().c_str());
}

const FrameId& Frame::frame_id() const { 
  return frame_id_; 
}

void Frame::set_frame_id(const FrameId& frame_id) { 
  frame_id_ = frame_id; 
}

Page& Frame::page() { 
  return page_; 
}

const Page& Frame::page() const { 
  return page_; 
}

PageNum Frame::page_num() const { 
  return page_.header.page_num; 
}

void Frame::pin() { 
  ++pin_count_; 
}

void Frame::unpin() {
  if (pin_count_ > 0) {
    --pin_count_;
  }
}

int Frame::pin_count() const { 
  return pin_count_; 
}

void Frame::access() {
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  acc_time_ = tp.tv_sec * 1000 * 1000 * 1000UL + tp.tv_nsec;
}

bool Frame::is_dirty() const { 
  return page_.header.flags & PAGE_DIRTY_FLAG;
}

void Frame::mark_dirty() { 
  page_.header.flags |= PAGE_DIRTY_FLAG;
}

void Frame::clear_dirty() {
  page_.header.flags &= ~PAGE_DIRTY_FLAG;
}

int Frame::buffer_pool_id() const {
  return frame_id_.buffer_pool_id;
}

void Frame::set_buffer_pool_id(int id) {
  frame_id_.buffer_pool_id = id;
}

LSN Frame::lsn() const {
  return page_.header.lsn;
}

void Frame::set_lsn(LSN lsn) {
  page_.header.lsn = lsn;
}

void Frame::set_page_num(PageNum page_num) {
  page_.header.page_num = page_num;
}

PageType Frame::page_type() const {
  return static_cast<PageType>(page_.header.page_type);
}

void Frame::set_page_type(PageType type) {
  page_.header.page_type = static_cast<uint8_t>(type);
}

void Frame::calc_checksum() {
  page_.calc_checksum();
}

bool Frame::verify_checksum() const {
  return page_.verify_checksum();
}

std::string Frame::to_string() const {
  std::stringstream ss;
  ss << "frame id: " << frame_id_.to_string() 
      << ", pin: " << pin_count_
      << ", dirty: " << (is_dirty() ? "yes" : "no")
      << ", page type: " << page_type()
      << ", lsn: " << lsn();
  return ss.str();
}

} // namespace storage
