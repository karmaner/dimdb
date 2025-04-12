#include <string.h>
#include <chrono>
#include <thread>
#include <sstream>

#include "storage/clog/log_buffer.h"
#include "common/rc.h"
#include "common/log/log.h"

RC LogBuffer::init(LSN lsn, int32_t max_bytes /* = 0 */) {
	current_lsn_.store(lsn);
	flushed_lsn_.store(lsn);

	if (max_bytes > 0) {
		max_bytes_ = max_bytes;
	}
	return RC::SUCCESS;
}

RC LogBuffer::append(LogEntry&& entry) {
	while (current_bytes_.load() >= max_bytes_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	std::lock_guard<std::mutex> lock(mutex_);
	// 更新LSN和统计信息
	entry.set_lsn(++current_lsn_);

	// 添加日志条目
	entries_.push_back(std::move(entry));
	current_bytes_ += entries_.back().total_size();
	total_appends_++;

	// 检查是否需要触发刷盘
	if (should_flush()) {
		try_notify_flush();
	}

	return RC::SUCCESS;
}

RC LogBuffer::append(LSN& lsn, LogModule::Id module_id, std::vector<char>&& data) {
	return append(lsn, LogModule(module_id), std::move(data));
}

RC LogBuffer::append(LSN& lsn, LogModule module, std::vector<char>&& data) {
	while (current_bytes_.load() >= max_bytes_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	LogEntry entry;
	RC rc = entry.init(lsn, module, std::move(data));
	if (IS_FAIL(rc)) {
		LOG_WARN("failed to init log entry. rc=%s", strrc(rc));
		return rc;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	lsn = ++current_lsn_;
	entry.set_lsn(lsn);

	entries_.push_back(std::move(entry));
	current_bytes_ += entries_.back().total_size();
	total_appends_++;

	return RC::SUCCESS;
}

RC LogBuffer::flush_batch(LogFileWriter& writer, size_t batch_size) {
	std::unique_lock<std::mutex> lock(mutex_);
	
	if (entries_.empty()) {
		return RC::SUCCESS;
	}

	// 确定实际批量大小
	batch_size = std::min(batch_size, entries_.size());
	
	// 记录开始时间
	auto start_time = std::chrono::steady_clock::now();

	// 批量写入日志条目
	size_t written = 0;
	while (written < batch_size && !entries_.empty()) {
		// 先尝试写入
		RC rc = writer.write(entries_.front());
		if (IS_FAIL(rc)) {
			LOG_ERROR("Failed to write log entry in batch, lsn=%ld", entries_.front().lsn());
			return rc;
		}
		
		// 写入成功后才移动和删除条目
		const auto& entry = std::move(entries_.front());
		current_bytes_ -= entry.total_size();
		flushed_lsn_ = entry.lsn();
		entries_.pop_front();
		written++;
	}

	// 更新统计信息
	total_flushes_++;
	auto end_time = std::chrono::steady_clock::now();
	total_wait_time_us_ += std::chrono::duration_cast<std::chrono::microseconds>(
			end_time - start_time).count();

	// 如果还有未刷盘的数据，且达到阈值，继续通知
	if (!entries_.empty() && should_flush()) {
		try_notify_flush();
	} else {
		flush_cv_.notify_all();  // 通知所有等待的线程
	}

	return RC::SUCCESS;
}

RC LogBuffer::flush(LogFileWriter& writer) {
	return flush_batch(writer, entries_.size());  // 刷入所有条目
}

bool LogBuffer::should_flush() const {
	return current_bytes_ >= max_bytes_ * flush_threshold_;
}

void LogBuffer::try_notify_flush() {
	flush_cv_.notify_one();
}

std::string LogBuffer::to_string() const {
	std::stringstream ss;
	ss << "LogBuffer("
		<< "current_bytes=" << current_bytes_ << "/" << max_bytes_ << "(" 
		<< (static_cast<double>(current_bytes_) / max_bytes_ * 100) << "%), "
		<< "entries=" << entries_.size() << ", "
		<< "current_lsn=" << current_lsn_ << ", "
		<< "flushed_lsn=" << flushed_lsn_ << ", "
		<< "total_appends=" << total_appends_ << ", "
		<< "total_flushes=" << total_flushes_ << ", "
		<< "avg_flush_time=" << (total_flushes_ > 0 ? 
			static_cast<double>(total_wait_time_us_) / total_flushes_ / 1000.0 : 0.0) 
		<< "ms)";
	return ss.str();
} 