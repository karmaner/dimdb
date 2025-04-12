#pragma once

#include <string>
#include <variant>
#include <array>
#include <chrono>
#include <cstring>
#include <ctime>
#include <stdexcept>


/**
 * @brief 扩展后的字段类型枚举
 */
enum class ArrrType {
  UNDEFINED = 0,
  INTEGER,    // 整数类型
  FLOAT,      // 浮点类型
  VARCHAR,    // 变长字符串
  BOOLEAN,    // 布尔类型
  TIMESTAMP,  // 时间戳类型 (统一使用时间戳存储时间)
  CHAR,       // 定长字符串
  MAX_TYPE    // 最大类型
};

// 前向声明
class Value;

/**
 * @brief 定长字符串类，固定大小的字符数组
 */
class CharString {
public:
  explicit CharString(size_t size) : size_(size)
    , data_(new char[size + 1]()) {
    data_[size] = '\0';  // 确保null终止
  }
  
  CharString(const char* str, size_t size) : size_(size)
    , data_(new char[size + 1]()) {
    size_t copyLen = std::min(size, strlen(str));
    std::memcpy(data_, str, copyLen);
    // 将剩余空间填充空格
    if (copyLen < size) {
      std::memset(data_ + copyLen, ' ', size - copyLen);
    }
    data_[size] = '\0';  // 确保null终止
  }
  
  CharString(const CharString& other) : size_(other.size_)
    , data_(new char[size_ + 1]()) {
    std::memcpy(data_, other.data_, size_ + 1);
  }
  
  CharString& operator=(const CharString& other) {
    if (this != &other) {
      delete[] data_;
      size_ = other.size_;
      data_ = new char[size_ + 1];
      std::memcpy(data_, other.data_, size_ + 1);
    }
    return *this;
  }
  
  CharString(CharString&& other) noexcept : size_(other.size_)
    , data_(other.data_) {
    other.data_ = nullptr;
    other.size_ = 0;
  }
  
  CharString& operator=(CharString&& other) noexcept {
    if (this != &other) {
      delete[] data_;
      size_ = other.size_;
      data_ = other.data_;
      other.data_ = nullptr;
      other.size_ = 0;
    }
    return *this;
  }
  
  ~CharString() {
    delete[] data_;
  }
  
  const char* data() const { return data_; }
  char* data() { return data_; }
  size_t size() const { return size_; }

  // 获取实际长度（忽略尾部空格）
  size_t actual_length() const {
    size_t end = size_;
    while (end > 0 && data_[end-1] == ' ') {
      end--;
    }
    return end;
  }
  
  bool operator==(const CharString& other) const {
    // 找到两个字符串的实际结束位置（忽略尾部空格）
    size_t end1 = size_;
    size_t end2 = other.size_;
    while (end1 > 0 && data_[end1-1] == ' ') end1--;
    while (end2 > 0 && other.data_[end2-1] == ' ') end2--;
    
    // 如果实际长度不同，则不相等
    if (end1 != end2) return false;
    
    // 比较实际内容
    return std::memcmp(data_, other.data_, end1) == 0;
  }
  
  bool operator<(const CharString& other) const {
    // 找到两个字符串的实际结束位置（忽略尾部空格）
    size_t end1 = size_;
    size_t end2 = other.size_;
    while (end1 > 0 && data_[end1-1] == ' ') end1--;
    while (end2 > 0 && other.data_[end2-1] == ' ') end2--;
    
    // 比较实际内容
    int cmp = std::memcmp(data_, other.data_, std::min(end1, end2));
    if (cmp != 0) return cmp < 0;
    return end1 < end2;
  }
  
  std::string to_string() const {
    return std::string(data_);
  }

private:
  size_t size_;
  char* data_;
};

/**
 * @brief 字段值类，用于表示数据库中的各种类型的值
 */
class Value {
  using Data = std::variant<
    int32_t, float, bool, std::string, int64_t,
    CharString
  >;

public:
  // 默认构造函数
  Value() : type_(ArrrType::UNDEFINED), is_null_(true) {}

  // 基本类型的构造函数
  explicit Value(int32_t val) : type_(ArrrType::INTEGER), value_(val) {}
  explicit Value(float val) : type_(ArrrType::FLOAT), value_(val) {}
  explicit Value(bool val) : type_(ArrrType::BOOLEAN), value_(val) {}
  explicit Value(const std::string& val) : type_(ArrrType::VARCHAR), value_(val) {}
  explicit Value(std::string&& val) : type_(ArrrType::VARCHAR), value_(std::move(val)) {}
  explicit Value(int64_t timestamp) : type_(ArrrType::TIMESTAMP), value_(timestamp) {}
  explicit Value(const CharString& val) : type_(ArrrType::CHAR), value_(val) {}
  explicit Value(CharString&& val) : type_(ArrrType::CHAR), value_(std::move(val)) {}

  // 时间相关的便捷构造函数
  explicit Value(const std::string& dateStr, bool isDate = true) {
    type_ = ArrrType::TIMESTAMP;
    struct tm tm = {};
    const char* result = nullptr;
    if (isDate) {
      // 尝试不同的日期格式
      if ((result = strptime(dateStr.c_str(), "%Y-%m-%d", &tm)) != nullptr ||
          (result = strptime(dateStr.c_str(), "%Y-%m", &tm)) != nullptr ||
          (result = strptime(dateStr.c_str(), "%Y", &tm)) != nullptr) {
        value_ = mktime(&tm);
      } else {
        throw std::invalid_argument("Invalid date format. Expected: YYYY[-MM[-DD]]");
      }
    } else {
      // 尝试不同的时间格式
      if ((result = strptime(dateStr.c_str(), "%H:%M:%S", &tm)) != nullptr ||
          (result = strptime(dateStr.c_str(), "%H:%M", &tm)) != nullptr ||
          (result = strptime(dateStr.c_str(), "%H", &tm)) != nullptr) {
        value_ = mktime(&tm);
      } else {
        throw std::invalid_argument("Invalid time format. Expected: HH[:MM[:SS]]");
      }
    }
  }

  // 拷贝构造和赋值
  Value(const Value& other) = default;
  Value& operator=(const Value& other) = default;

  // 移动构造和赋值
  Value(Value&& other) noexcept = default;
  Value& operator=(Value&& other) noexcept = default;

  // 析构函数
  ~Value() = default;

  // 类型相关方法
  inline ArrrType get_type() const { return type_; }
  inline bool is_null() const { return is_null_; }
  inline void set_null(bool null) { is_null_ = null; }

  // 获取基本类型的值
  inline int32_t get_integer() const {
    return std::get<int32_t>(value_);
  }

  inline float get_float() const {
    return std::get<float>(value_);
  }

  inline bool get_boolean() const {
    return std::get<bool>(value_);
  }

  inline const std::string& get_string() const {
    return std::get<std::string>(value_);
  }

  // 获取时间戳
  inline int64_t get_timestamp() const {
    return std::get<int64_t>(value_);
  }
  
  // 获取新增类型的值
  inline const CharString& get_char() const {
    return std::get<CharString>(value_);
  }
  
  // 获取日期字符串 (YYYY-MM-DD)
  std::string get_date_string() const {
    if (type_ != ArrrType::TIMESTAMP) {
      throw std::runtime_error("Value is not a timestamp");
    }
    time_t timestamp = std::get<int64_t>(value_);
    struct tm* timeinfo = std::localtime(&timestamp);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
             timeinfo->tm_year + 1900,
             timeinfo->tm_mon + 1,
             timeinfo->tm_mday);
    return std::string(buf);
  }

  // 获取时间字符串 (HH:MM:SS)
  std::string get_time_string() const {
    if (type_ != ArrrType::TIMESTAMP) {
      throw std::runtime_error("Value is not a timestamp");
    }
    time_t timestamp = std::get<int64_t>(value_);
    struct tm* timeinfo = std::localtime(&timestamp);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             timeinfo->tm_hour,
             timeinfo->tm_min,
             timeinfo->tm_sec);
    return std::string(buf);
  }

  // 比较操作符
  bool operator==(const Value& other) const;
  bool operator!=(const Value& other) const;
  bool operator<(const Value& other) const;
  bool operator<=(const Value& other) const;
  bool operator>(const Value& other) const;
  bool operator>=(const Value& other) const;

  // 序列化和反序列化
  void serialize_to(char* buf) const;
  static Value deserialize_from(const char* buf, ArrrType type);

  // 获取序列化后的大小
  uint32_t get_serialized_size() const;

  // 转换为字符串
  std::string to_string() const;

private:
  ArrrType type_;
  bool is_null_{false};
  Data value_;
};
