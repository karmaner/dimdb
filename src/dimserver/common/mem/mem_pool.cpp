#include "common/mem/mem_pool.h"
#include <cstring>

int MemPoolItem::init(int item_size, bool dynamic, int pool_num, int item_num_per_pool) {
  if (!pools_.empty()) {
    LOG_WARN("Memory pool has been initialized, but still begin to be initialized, name:%s.", this->name_.c_str());
    return 0;
  }

  if (item_size <= 0 || pool_num <= 0 || item_num_per_pool <= 0) {
    LOG_ERROR("Invalid arguments, item_size:%d, pool_num:%d, item_num_per_pool:%d, name:%s.",
      item_size, pool_num, item_num_per_pool, this->name_.c_str());
    return -1;
  }

  this->item_size_ = item_size;
  this->item_num_per_pool_ = item_num_per_pool;
  this->dynamic_ = true;
  for (int i = 0; i < pool_num; ++i) {
    if (extend() < 0) {
      cleanup();
      return -1;
    }
  }
  this->dynamic_ = dynamic;

  LOG_INFO("Extend one pool, size:%d, item_size:%d, item_num_per_pool:%d, name:%s.",
    this->size_, this->item_size_, this->item_num_per_pool_, this->name_.c_str());
  return 0;
}

void MemPoolItem::cleanup() {
  if (pools_.empty()) {
    LOG_WARN("Begin to do cleanup, but there is no memory pool, name:%s!", this->name_.c_str());
    return;
  }

  std::unique_lock<std::shared_mutex> lock(this->mutex_);
  used_.clear();
  frees_.clear();
  this->size_ = 0;

  for (auto* pool : pools_) {
    free(pool);
  }
  pools_.clear();
  
  LOG_INFO("Successfully do cleanup, name:%s.", this->name_.c_str());
}

int MemPoolItem::extend() {
  if (!this->dynamic_) {
    LOG_ERROR("Disable dynamic extend memory pool, but begin to extend, name:%s", this->name_.c_str());
    return -1;
  }

  std::unique_lock<std::shared_mutex> lock(this->mutex_);
  void* pool = malloc(static_cast<size_t>(this->item_num_per_pool_) * this->item_size_);
  if (!pool) {
    LOG_ERROR("Failed to extend memory pool, size:%d, item_num_per_pool:%d, name:%s.",
      this->size_, this->item_num_per_pool_, this->name_.c_str());
    return -1;
  }

  pools_.push_back(pool);
  this->size_ += this->item_num_per_pool_;
  for (int i = 0; i < this->item_num_per_pool_; ++i) {
    void* item = static_cast<char*>(pool) + i * this->item_size_;
    frees_.push_back(item);
  }
  lock.unlock();

  LOG_INFO("Extend one pool, size:%d, item_size:%d, item_num_per_pool:%d, name:%s.",
    this->size_, this->item_size_, this->item_num_per_pool_, this->name_.c_str());
  return 0;
}

void* MemPoolItem::alloc() {
  std::unique_lock<std::shared_mutex> lock(this->mutex_);
  
  if (frees_.empty()) {
    if (!this->dynamic_) {
      return nullptr;
    }

    if (extend() != 0) {
      LOG_ERROR("Failed to alloc memory, name:%s", this->name_.c_str());
      return nullptr;
    }
  }
  
  void* item = frees_.front();
  frees_.pop_front();
  used_.insert(item);
  lock.unlock();

  memset(item, 0, this->item_size_);
  return item;
}

MemPoolItem::item_unique_ptr MemPoolItem::alloc_unique_ptr() {
  void* item = alloc();
  if (item == nullptr) {
    return nullptr;
  }

  return item_unique_ptr(item, [this](void* const p) { this->free(p); });
}

void MemPoolItem::free(void* item) {
  if (item == nullptr) {
    LOG_WARN("Invalid item pointer (nullptr), name:%s", this->name_.c_str());
    return;
  }

  std::unique_lock<std::shared_mutex> lock(this->mutex_);
  auto it = used_.find(item);
  if (it == used_.end()) {
    LOG_WARN("Try to free an item not in used list, name:%s", this->name_.c_str());
    return;
  }
  
  used_.erase(it);
  frees_.push_back(item);
}