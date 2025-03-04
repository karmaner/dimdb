#pragma once

#include <string>
#include <variant>
#include <array>
#include <chrono>
#include <cstring>
#include <ctime>
#include <stdexcept>

namespace dimdb {

/**
 * @brief 扩展后的字段类型枚举
 */
enum class FieldType : uint8_t {
  INVALID = 0,
  INTEGER = 1,    // 整数类型
  FLOAT = 2,      // 浮点类型
  VARCHAR = 3,    // 变长字符串
  BOOLEAN = 4,    // 布尔类型
  TIMESTAMP = 5,  // 时间戳类型
  CHAR = 6,       // 定长字符串
  DATE = 7,       // 日期类型 (YYYY-MM-DD)
  TIME = 8        // 时间类型 (HH:MM:SS)
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
  
  bool operator==(const CharString& other) const {
    return size_ == other.size_ 
      && std::memcmp(data_, other.data_, size_) == 0;
  }
  
  bool operator<(const CharString& other) const {
    int cmp = std::memcmp(data_, other.data_
      , std::min(size_, other.size_));
    return cmp < 0 || (cmp == 0 && size_ < other.size_);
  }
  
  std::string ToString() const {
    return std::string(data_, size_);
  }

private:
  size_t size_;
  char* data_;
};

/**
 * @brief 日期类，表示YYYY-MM-DD格式
 */
class Date {
public:
  Date() : year_(0), month_(0), day_(0) {}
  
  Date(int16_t year, uint8_t month, uint8_t day) 
    : year_(year), month_(month), day_(day) {}
  
  // 从字符串构造 "YYYY-MM-DD"
  explicit Date(const std::string& dateStr) {
    if (dateStr.size() != 10 || dateStr[4] != '-' 
      || dateStr[7] != '-') {
      throw std::invalid_argument("Invalid date format. Expected: YYYY-MM-DD");
    }
    try {
      year_ = std::stoi(dateStr.substr(0, 4));
      month_ = std::stoi(dateStr.substr(5, 2));
      day_ = std::stoi(dateStr.substr(8, 2));
      
      // 验证日期有效性
      if (month_ < 1 || month_ > 12 || day_ < 1 || day_ > 31) {
        throw std::invalid_argument("Invalid date values");
      }
    } catch (const std::exception&) {
      throw std::invalid_argument("Invalid date format");
    }
  }
  
  // 从时间戳构造
  explicit Date(time_t timestamp) {
    struct tm* timeinfo = std::localtime(&timestamp);
    year_ = timeinfo->tm_year + 1900;
    month_ = timeinfo->tm_mon + 1;
    day_ = timeinfo->tm_mday;
  }
  
  int16_t GetYear() const { return year_; }
  uint8_t GetMonth() const { return month_; }
  uint8_t GetDay() const { return day_; }
  
  std::string ToString() const {
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d"
      , year_, month_, day_);
    return std::string(buf);
  }
  
  bool operator==(const Date& other) const {
    return year_ == other.year_ && month_ == other.month_ && day_ == other.day_;
  }
  
  bool operator<(const Date& other) const {
    if (year_ != other.year_) return year_ < other.year_;
    if (month_ != other.month_) return month_ < other.month_;
    return day_ < other.day_;
  }
  
private:
  int16_t year_;
  uint8_t month_;
  uint8_t day_;
};

/**
 * @brief 时间类，表示HH:MM:SS格式
 */
class Time {
public:
  Time() : hour_(0), minute_(0), second_(0) {}
  
  Time(uint8_t hour, uint8_t minute, uint8_t second) 
    : hour_(hour), minute_(minute), second_(second) {}
  
  // 从字符串构造 "HH:MM:SS"
  explicit Time(const std::string& timeStr) {
    if (timeStr.size() != 8 || timeStr[2] != ':' 
      || timeStr[5] != ':') {
      throw std::invalid_argument("Invalid time format. Expected: HH:MM:SS");
    }
    try {
      hour_ = std::stoi(timeStr.substr(0, 2));
      minute_ = std::stoi(timeStr.substr(3, 2));
      second_ = std::stoi(timeStr.substr(6, 2));
      
      // 验证时间有效性
      if (hour_ > 23 || minute_ > 59 || second_ > 59) {
        throw std::invalid_argument("Invalid time values");
    }
    } catch (const std::exception&) {
      throw std::invalid_argument("Invalid time format");
    }
  }
  
  // 从时间戳构造
  explicit Time(time_t timestamp) {
    struct tm* timeinfo = std::localtime(&timestamp);
    hour_ = timeinfo->tm_hour;
    minute_ = timeinfo->tm_min;
    second_ = timeinfo->tm_sec;
  }
  
  uint8_t GetHour() const { return hour_; }
  uint8_t GetMinute() const { return minute_; }
  uint8_t GetSecond() const { return second_; }
  
  std::string ToString() const {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d"
      , hour_, minute_, second_);
    return std::string(buf);
  }
  
  bool operator==(const Time& other) const {
    return hour_ == other.hour_ && minute_ == other.minute_ 
      && second_ == other.second_;
  }
  
  bool operator<(const Time& other) const {
    if (hour_ != other.hour_) return hour_ < other.hour_;
    if (minute_ != other.minute_) return minute_ < other.minute_;
    return second_ < other.second_;
  }
  
private:
  uint8_t hour_;
  uint8_t minute_;
  uint8_t second_;
};

/**
 * @brief 字段值类，用于表示数据库中的各种类型的值
 */
class Value {
 public:
  // 默认构造函数
  Value() : type_(FieldType::INVALID), is_null_(true) {}

  // 基本类型的构造函数
  explicit Value(int32_t val) : type_(FieldType::INTEGER), value_(val) {}
  explicit Value(float val) : type_(FieldType::FLOAT), value_(val) {}
  explicit Value(bool val) : type_(FieldType::BOOLEAN), value_(val) {}
  explicit Value(const std::string& val) : type_(FieldType::VARCHAR), value_(val) {}
  explicit Value(std::string&& val) : type_(FieldType::VARCHAR), value_(std::move(val)) {}
  explicit Value(int64_t timestamp) : type_(FieldType::TIMESTAMP), value_(timestamp) {}
  
  // 新增类型的构造函数
  explicit Value(const CharString& val) : type_(FieldType::CHAR), value_(val) {}
  explicit Value(CharString&& val) : type_(FieldType::CHAR), value_(std::move(val)) {}
  explicit Value(const Date& val) : type_(FieldType::DATE), value_(val) {}
  explicit Value(const Time& val) : type_(FieldType::TIME), value_(val) {}

  // 拷贝构造和赋值
  Value(const Value& other) = default;
  Value& operator=(const Value& other) = default;

  // 移动构造和赋值
  Value(Value&& other) noexcept = default;
  Value& operator=(Value&& other) noexcept = default;

  // 析构函数
  ~Value() = default;

  // 类型相关方法
  inline FieldType GetType() const { return type_; }
  inline bool IsNull() const { return is_null_; }
  inline void SetNull(bool null) { is_null_ = null; }

  // 获取基本类型的值
  inline int32_t GetInteger() const {
    return std::get<int32_t>(value_);
  }

  inline float GetFloat() const {
    return std::get<float>(value_);
  }

  inline bool GetBoolean() const {
    return std::get<bool>(value_);
  }

  inline const std::string& GetString() const {
    return std::get<std::string>(value_);
  }

  inline int64_t GetTimestamp() const {
    return std::get<int64_t>(value_);
  }
  
  // 获取新增类型的值
  inline const CharString& GetChar() const {
    return std::get<CharString>(value_);
  }
  
  inline const Date& GetDate() const {
    return std::get<Date>(value_);
  }
  
  inline const Time& GetTime() const {
    return std::get<Time>(value_);
  }

  // 比较操作符
  bool operator==(const Value& other) const;
  bool operator!=(const Value& other) const;
  bool operator<(const Value& other) const;
  bool operator<=(const Value& other) const;
  bool operator>(const Value& other) const;
  bool operator>=(const Value& other) const;

  // 序列化和反序列化
  void SerializeTo(char* buf) const;
  static Value DeserializeFrom(const char* buf, FieldType type);

  // 获取序列化后的大小
  uint32_t GetSerializedSize() const;

  // 转换为字符串
  std::string ToString() const;

 private:
  FieldType type_;
  bool is_null_{false};
  std::variant<
    int32_t, float, bool, std::string, int64_t,
    CharString, Date, Time
  > value_;
};

} // namespace dimdb