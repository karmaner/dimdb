#include "value.h"
#include <cstring>
#include <sstream>

bool Value::operator==(const Value& other) const {
  if (type_ != other.type_ || is_null_ != other.is_null_) {
    return false;
  }
  if (is_null_) {
    return true;
  }
  return value_ == other.value_;
}

bool Value::operator!=(const Value& other) const {
  return !(*this == other);
}

bool Value::operator<(const Value& other) const {
  if (type_ != other.type_) {
    return type_ < other.type_;
  }
  if (is_null_ || other.is_null_) {
    return !is_null_ && other.is_null_;
  }
  return value_ < other.value_;
}

bool Value::operator<=(const Value& other) const {
  return *this < other || *this == other;
}

bool Value::operator>(const Value& other) const {
  return !(*this <= other);
}

bool Value::operator>=(const Value& other) const {
  return !(*this < other);
}

void Value::serialize_to(char* buf) const {
  size_t offset = 0;

  // 序列化类型
  memcpy(buf + offset, &type_, sizeof(ArrrType));
  offset += sizeof(ArrrType);

  // 序列化空值标记
  memcpy(buf + offset, &is_null_, sizeof(bool));
  offset += sizeof(bool);

  if (!is_null_) {
    // 序列化具体的值
    switch (type_) {
      case ArrrType::INTEGER: {
        auto val = get_integer();
        memcpy(buf + offset, &val, sizeof(int32_t));
        break;
      }
      case ArrrType::FLOAT: {
        auto val = get_float();
        memcpy(buf + offset, &val, sizeof(float));
        break;
      }
      case ArrrType::BOOLEAN: {
        auto val = get_boolean();
        memcpy(buf + offset, &val, sizeof(bool));
        break;
      }
      case ArrrType::VARCHAR: {
        const auto& str = get_string();
        uint32_t len = str.length();
        memcpy(buf + offset, &len, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buf + offset, str.c_str(), len);
        break;
      }
      case ArrrType::TIMESTAMP: {
        auto val = get_timestamp();
        memcpy(buf + offset, &val, sizeof(int64_t));
        break;
      }
      case ArrrType::CHAR: {
        const auto& charStr = get_char();
        uint32_t size = charStr.size();
        memcpy(buf + offset, &size, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buf + offset, charStr.data(), size);
        break;
      }
      default:
        break;
    }
  }
}

Value Value::deserialize_from(const char* buf, ArrrType type) {
  size_t offset = 0;

  // 反序列化类型
  ArrrType stored_type;
  memcpy(&stored_type, buf + offset, sizeof(ArrrType));
  offset += sizeof(ArrrType);

  // 类型检查
  if (stored_type != type) {
    throw std::runtime_error("Type mismatch in Value deserialization");
  }

  // 反序列化空值标记
  bool is_null;
  memcpy(&is_null, buf + offset, sizeof(bool));
  offset += sizeof(bool);

  if (is_null) {
    Value v;
    v.type_ = type;
    v.is_null_ = true;
    return v;
  }

  // 反序列化具体的值
  switch (type) {
    case ArrrType::INTEGER: {
      int32_t val;
      memcpy(&val, buf + offset, sizeof(int32_t));
      return Value(val);
    }
    case ArrrType::FLOAT: {
      float val;
      memcpy(&val, buf + offset, sizeof(float));
      return Value(val);
    }
    case ArrrType::BOOLEAN: {
      bool val;
      memcpy(&val, buf + offset, sizeof(bool));
      return Value(val);
    }
    case ArrrType::VARCHAR: {
      uint32_t len;
      memcpy(&len, buf + offset, sizeof(uint32_t));
      offset += sizeof(uint32_t);
      return Value(std::string(buf + offset, len));
    }
    case ArrrType::TIMESTAMP: {
      int64_t val;
      memcpy(&val, buf + offset, sizeof(int64_t));
      return Value(val);
    }
    case ArrrType::CHAR: {
      uint32_t size;
      memcpy(&size, buf + offset, sizeof(uint32_t));
      offset += sizeof(uint32_t);
      return Value(CharString(buf + offset, size));
    }
    default:
      throw std::runtime_error("Invalid type in Value deserialization");
  }
}

uint32_t Value::get_serialized_size() const {
  uint32_t size = sizeof(ArrrType) + sizeof(bool);  // type + is_null
  if (!is_null_) {
    switch (type_) {
      case ArrrType::INTEGER:
        size += sizeof(int32_t);
        break;
      case ArrrType::FLOAT:
        size += sizeof(float);
        break;
      case ArrrType::BOOLEAN:
        size += sizeof(bool);
        break;
      case ArrrType::VARCHAR:
        size += sizeof(uint32_t) + get_string().length();
        break;
      case ArrrType::TIMESTAMP:
        size += sizeof(int64_t);
        break;
      case ArrrType::CHAR:
        size += sizeof(uint32_t) + get_char().size();
        break;
      default:
        break;
    }
  }
  return size;
}

std::string Value::to_string() const {
  if (is_null_) {
    return "NULL";
  }

  std::stringstream ss;
  switch (type_) {
    case ArrrType::INTEGER:
      ss << get_integer();
      break;
    case ArrrType::FLOAT:
      ss << get_float();
      break;
    case ArrrType::BOOLEAN:
      ss << (get_boolean() ? "true" : "false");
      break;
    case ArrrType::VARCHAR:
      ss << get_string();
      break;
    case ArrrType::TIMESTAMP:
      ss << get_timestamp();
      break;
    case ArrrType::CHAR:
      ss << get_char().to_string();
      break;
    default:
      ss << "INVALID";
      break;
  }
  return ss.str();
}