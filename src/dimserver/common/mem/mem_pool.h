#pragma once

#include <cstddef>
#include <set>
#include <list>
#include <string>
#include <shared_mutex>
#include <memory>
#include <functional>
#include <sstream>

#include "common/log/log.h"

constexpr int DEFAULT_ITEM_NUM_PER_POOL = 128;
constexpr int DEFAULT_POOL_NUM = 1;


template <typename T>
class MemPool {
public:
	explicit MemPool(std::string tag) : size_(0), dynamic_(false), name_(std::move(tag)) {
		this->size_ = 0;
	}
	virtual ~MemPool() = default;

	/**
	 * 初始化内存池
	 * @param dynamic 是否动态扩展
	 * @param pool_num 内存池的数量
	 * @param item_num_per_pool 每个池中的项目数
	 * @return 0表示成功，其他表示失败
	 */
	virtual int init(
		bool dynamic = true, 
		int pool_num = DEFAULT_POOL_NUM, 
		int item_num_per_pool = DEFAULT_ITEM_NUM_PER_POOL) = 0;

	virtual void cleanup() = 0;

	virtual int extend() = 0;
	virtual T* alloc() = 0;
	virtual void free(T* item) = 0;

	virtual std::string to_string() = 0;


	const std::string& name() const { return name_; }
	bool dynamic() const { return dynamic_; }
	int size() const { 
		std::shared_lock<std::shared_mutex> lock(mutex_);
		return size_; 
	}

protected:
	mutable std::shared_mutex mutex_;  // 读写锁，提高并发性能
	int size_;
	bool dynamic_;
	std::string name_;
};


template <typename T>
class MemPoolSimple : public MemPool<T> {
public:
	explicit MemPoolSimple(std::string tag) : MemPool<T>(std::move(tag)), item_num_per_pool_(0) {}
	virtual ~MemPoolSimple() override { cleanup(); }
	
	/**
	 * 初始化内存池
	 * @param dynamic 是否动态扩展
	 * @param pool_num 内存池的数量
	 * @param item_num_per_pool 每个池中的项目数
	 * @return 0表示成功，其他表示失败
	 */
	int init(bool dynamic = true,
		int pool_num = DEFAULT_POOL_NUM,
		int item_num_per_pool = DEFAULT_ITEM_NUM_PER_POOL) override;
	
	void cleanup() override;
	int extend() override;

	T* alloc() override;
	void free(T* item) override;

	std::string to_string() override;

	int get_item_num_per_pool() const { return item_num_per_pool_; }

	int used_num() {
		std::shared_lock<std::shared_mutex> lock(this->mutex_);
		return used_.size();
	}

protected:
	std::list<T*> pools_;
	std::set<T*> used_;
	std::list<T*> frees_;
	int item_num_per_pool_;
};


class MemPoolItem {
public:
	using item_unique_ptr = std::unique_ptr<void, std::function<void(void* const)>>;

public:
	MemPoolItem(const std::string tag)
		: name_(std::move(tag))
		, dynamic_(false)
		, size_(0)
		, item_size_(0)
		, item_num_per_pool_(0) { }
	
	virtual ~MemPoolItem() {
		cleanup();
	}

	int init(int item_size, bool dynamic = true,
		int pool_num = DEFAULT_POOL_NUM,
		int item_num_per_pool = DEFAULT_ITEM_NUM_PER_POOL);

	void cleanup();
	int extend();

	void* alloc();
	item_unique_ptr alloc_unique_ptr();

	void free(void* item);

	bool is_used(void* item) {
		std::shared_lock<std::shared_mutex> lock(mutex_);
		return used_.find(item) != used_.end();
	}

	std::string to_string() {
		std::stringstream ss;

    ss << "name:" << name_ << ","
			<< "dynamic:" << (dynamic_ ? "true" : "false") << ","
			<< "size:" << size_ << ","
			<< "pool_size:" << pools_.size() << ","
			<< "used_size:" << used_.size() << ","
			<< "free_size:" << frees_.size();
    return ss.str();
	}

	const std::string& get_name() const { return name_; }
	bool is_dynamic() const { return dynamic_; }
	int get_size() const { return size_; }
	int get_item_size() const { return item_size_; }
	int get_item_num_per_pool() const { return item_num_per_pool_; }

	int get_used_num() {
		std::shared_lock<std::shared_mutex> lock(mutex_);
		return used_.size();
	}

protected:
  mutable std::shared_mutex mutex_;
  std::string          name_;
  bool            dynamic_;
  int             size_;
  int             item_size_;
  int             item_num_per_pool_;

  std::list<void *> pools_;
  std::set<void *>  used_;
  std::list<void *> frees_;
};


/*------------------------- MemPoolSimple -------------------------------*/

/**
 * 初始化内存池
 * @param dynamic 是否动态扩展
 * @param pool_num 内存池的数量
 * @param item_num_per_pool 每个池中的项目数
 * @return 0表示成功，其他表示失败
 */
template<typename T>
int MemPoolSimple<T>::init(bool dynamic, int pool_num, int item_num_per_pool) {
	if (!pools_.empty()) {
		LOG_WARN("Memory pool has been initialized, but still begin to be initialized, name:%s.", this->name_.c_str());
		return 0;
	}

	if (pool_num <= 0 || item_num_per_pool <= 0) {
		LOG_ERROR("Invalid arguments, pool_num:%d, item_num_per_pool:%d, name:%s.",
			pool_num, item_num_per_pool, this->name_.c_str());
		return -1;
	}

	this->item_num_per_pool_ = item_num_per_pool;
	this->dynamic_ = true;
	for (int i = 0; i < pool_num; ++i) {
		if (extend() < 0) {
			cleanup();
			return -1;
		}
	}
	this->dynamic_ = dynamic;

	LOG_INFO("Extend one pool, size:%d, item_num_per_pool:%d, name:%s.",
		this->size_, this->item_num_per_pool_, this->name_.c_str());
	return 0;
}

template<typename T>
void MemPoolSimple<T>::cleanup() {

	if (pools_.empty()) {
		LOG_WARN("Begin to do cleanup, but there is no memory pool, name:%s!", this->name_.c_str());
		return;
	}
	std::unique_lock<std::shared_mutex> lock(this->mutex_);
	used_.clear();
	frees_.clear();
	this->size_ = 0;

	for (auto* pool : pools_) {
		delete[] pool;
	}
	pools_.clear();
	lock.unlock();

	LOG_INFO("Successfully do cleanup, name:%s.", this->name_.c_str());
}

template<typename T>
int MemPoolSimple<T>::extend() {
	if (!this->dynamic_) {
		LOG_ERROR("Disable dynamic extend memory pool, but begin to extend, name:%s", this->name_.c_str());
		return -1;
	}

	std::unique_lock<std::shared_mutex> lock(this->mutex_);
	T* pool = new T[item_num_per_pool_];
	if (!pool) {
		LOG_ERROR("Failed to extend memory pool, size:%d, item_num_per_pool:%d, name:%s.",
			this->size_, this->item_num_per_pool_, this->name_.c_str());
		return -1;
	}

	pools_.push_back(pool);
	this->size_ += item_num_per_pool_;
	for (int i = 0; i < item_num_per_pool_; ++i) {
		frees_.push_back(pool + i);
	}

	LOG_INFO("Extend one pool, size:%d, item_num_per_pool:%d, name:%s.",
		this->size_, this->item_num_per_pool_, this->name_.c_str());
	return 0;
}

template<typename T>
T* MemPoolSimple<T>::alloc() {
	std::unique_lock<std::shared_mutex> lock(this->mutex_);
	
	if (frees_.empty()) {
		if (!this->dynamic_) {
			lock.unlock();
			return nullptr;
		}

		if (extend() != 0) {
			lock.unlock();
			LOG_ERROR("Failed to alloc memory, name:%s", this->name_.c_str());
			return nullptr;
		}
	}
	
	T* item = frees_.front();
	frees_.pop_front();
	used_.insert(item);

	lock.unlock();
	item->reinit(); // 调用已有对象进行set
	return item;
}

template<typename T>
void MemPoolSimple<T>::free(T* item) {
	if (item == nullptr) {
		LOG_WARN("Invalid item pointer (nullptr), name:%s", this->name_.c_str());
		return;
	}
	
	item->reset();

	std::unique_lock<std::shared_mutex> lock(this->mutex_);
	
	// 检查是否是已使用项
	auto it = used_.find(item);
	if (it == used_.end()) {
		LOG_WARN("Try to free an item not in used list, name:%s", this->name_.c_str());
		return;
	}
	
	used_.erase(it);
	frees_.push_back(item);
	lock.unlock();
	return;
}

template<typename T>
std::string MemPoolSimple<T>::to_string() {
  std::stringstream ss;

  ss << "name:" << this->name_ << ","
		<< "dynamic:" << this->dynamic_ << ","
		<< "size:" << this->size_ << ","
		<< "pool_size:" << this->pools_.size() << ","
		<< "used_size:" << this->used_.size() << ","
		<< "free_size:" << this->frees_.size();
  return ss.str();
}