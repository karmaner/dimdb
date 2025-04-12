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

std::string FrameId::to_string() const {
  std::stringstream ss;
  ss << "buffer_pool_id:" << buffer_pool_id << ",page_num:" << page_num;
  return ss.str();
}

// Frame实现
Frame::Frame() : 
  m_pin_count(0), 
  m_acc_time(0) {
  m_page.init();
}

Frame::~Frame() {
  LOG_DEBUG("deallocate frame, this=%p, lbt=%s", this, common::stacktrace().c_str());
}

const FrameId& Frame::get_frame_id() const { 
  return m_frame_id; 
}

void Frame::set_frame_id(const FrameId& frame_id) { 
  m_frame_id = frame_id; 
}

Page& Frame::get_page() { 
  return m_page; 
}

const Page& Frame::get_page() const { 
  return m_page; 
}

PageNum Frame::get_page_num() const { 
  return m_page.header.page_num; 
}

void Frame::pin() { 
  ++m_pin_count; 
}

void Frame::unpin() {
  if (m_pin_count > 0) {
    --m_pin_count;
  }
}

int Frame::get_pin_count() const { 
  return m_pin_count; 
}

void Frame::access() {
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  m_acc_time = tp.tv_sec * 1000 * 1000 * 1000UL + tp.tv_nsec;
}

bool Frame::is_dirty() const { 
  return m_page.header.flags & PAGE_DIRTY_FLAG;
}

void Frame::mark_dirty() { 
  m_page.header.flags |= PAGE_DIRTY_FLAG;
}

void Frame::clear_dirty() {
  m_page.header.flags &= ~PAGE_DIRTY_FLAG;
}

int Frame::buffer_pool_id() const {
  return m_frame_id.buffer_pool_id;
}

void Frame::set_buffer_pool_id(int id) {
  m_frame_id.buffer_pool_id = id;
}

LSN Frame::get_lsn() const {
  return m_page.header.lsn;
}

void Frame::set_lsn(LSN lsn) {
  m_page.header.lsn = lsn;
}

PageType Frame::get_page_type() const {
  return static_cast<PageType>(m_page.header.page_type);
}

void Frame::set_page_type(PageType type) {
  m_page.header.page_type = static_cast<uint8_t>(type);
}

void Frame::calc_checksum() {
  m_page.calc_checksum();
}

bool Frame::verify_checksum() const {
  return m_page.verify_checksum();
}

std::string Frame::to_string() const {
  std::stringstream ss;
  ss << "frame id: " << m_frame_id.to_string() 
      << ", pin: " << m_pin_count
      << ", dirty: " << (is_dirty() ? "yes" : "no")
      << ", page type: " << get_page_type()
      << ", lsn: " << get_lsn();
  return ss.str();
}

} // namespace storag
