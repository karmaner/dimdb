#include "value.h"
#include <cstring>
#include <sstream>

namespace dimdb {

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

void Value::SerializeTo(char* buf) const {
  size_t offset = 0;

  // 序列化类型
  memcpy(buf + offset, &type_, sizeof(FieldType));
  offset += sizeof(FieldType);

  // 序列化空值标记
  memcpy(buf + offset, &is_null_, sizeof(bool));
  offset += sizeof(bool);

  if (!is_null_) {
    // 序列化具体的值
    switch (type_) {
      case FieldType::INTEGER: {
        auto val = GetInteger();
        memcpy(buf + offset, &val, sizeof(int32_t));
        break;
      }
      case FieldType::FLOAT: {
        auto val = GetFloat();
        memcpy(buf + offset, &val, sizeof(float));
        break;
      }
      case FieldType::BOOLEAN: {
        auto val = GetBoolean();
        memcpy(buf + offset, &val, sizeof(bool));
        break;
      }
      case FieldType::VARCHAR: {
        const auto& str = GetString();
        uint32_t len = str.length();
        memcpy(buf + offset, &len, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buf + offset, str.c_str(), len);
        break;
      }
      case FieldType::TIMESTAMP: {
        auto val = GetTimestamp();
        memcpy(buf + offset, &val, sizeof(int64_t));
        break;
      }
      case FieldType::CHAR: {
        const auto& charStr = GetChar();
        uint32_t size = charStr.size();
        memcpy(buf + offset, &size, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buf + offset, charStr.data(), size);
        break;
      }
      case FieldType::DATE: {
        const auto& date = GetDate();
        int16_t year = date.GetYear();
        uint8_t month = date.GetMonth();
        uint8_t day = date.GetDay();
        memcpy(buf + offset, &year, sizeof(int16_t));
        offset += sizeof(int16_t);
        memcpy(buf + offset, &month, sizeof(uint8_t));
        offset += sizeof(uint8_t);
        memcpy(buf + offset, &day, sizeof(uint8_t));
        break;
      }
      case FieldType::TIME: {
        const auto& time = GetTime();
        uint8_t hour = time.GetHour();
        uint8_t minute = time.GetMinute();
        uint8_t second = time.GetSecond();
        memcpy(buf + offset, &hour, sizeof(uint8_t));
        offset += sizeof(uint8_t);
        memcpy(buf + offset, &minute, sizeof(uint8_t));
        offset += sizeof(uint8_t);
        memcpy(buf + offset, &second, sizeof(uint8_t));
        break;
      }
      default:
        break;
    }
  }
}

Value Value::DeserializeFrom(const char* buf, FieldType type) {
  size_t offset = 0;

  // 反序列化类型
  FieldType stored_type;
  memcpy(&stored_type, buf + offset, sizeof(FieldType));
  offset += sizeof(FieldType);

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
    case FieldType::INTEGER: {
      int32_t val;
      memcpy(&val, buf + offset, sizeof(int32_t));
      return Value(val);
    }
    case FieldType::FLOAT: {
      float val;
      memcpy(&val, buf + offset, sizeof(float));
      return Value(val);
    }
    case FieldType::BOOLEAN: {
      bool val;
      memcpy(&val, buf + offset, sizeof(bool));
      return Value(val);
    }
    case FieldType::VARCHAR: {
      uint32_t len;
      memcpy(&len, buf + offset, sizeof(uint32_t));
      offset += sizeof(uint32_t);
      return Value(std::string(buf + offset, len));
    }
    case FieldType::TIMESTAMP: {
      int64_t val;
      memcpy(&val, buf + offset, sizeof(int64_t));
      return Value(val);
    }
    case FieldType::CHAR: {
      uint32_t size;
      memcpy(&size, buf + offset, sizeof(uint32_t));
      offset += sizeof(uint32_t);
      return Value(CharString(buf + offset, size));
    }
    case FieldType::DATE: {
      int16_t year;
      uint8_t month, day;
      memcpy(&year, buf + offset, sizeof(int16_t));
      offset += sizeof(int16_t);
      memcpy(&month, buf + offset, sizeof(uint8_t));
      offset += sizeof(uint8_t);
      memcpy(&day, buf + offset, sizeof(uint8_t));
      return Value(Date(year, month, day));
    }
    case FieldType::TIME: {
      uint8_t hour, minute, second;
      memcpy(&hour, buf + offset, sizeof(uint8_t));
      offset += sizeof(uint8_t);
      memcpy(&minute, buf + offset, sizeof(uint8_t));
      offset += sizeof(uint8_t);
      memcpy(&second, buf + offset, sizeof(uint8_t));
      return Value(Time(hour, minute, second));
    }
    default:
      throw std::runtime_error("Invalid type in Value deserialization");
  }
}

uint32_t Value::GetSerializedSize() const {
  uint32_t size = sizeof(FieldType) + sizeof(bool);  // type + is_null
  if (!is_null_) {
    switch (type_) {
      case FieldType::INTEGER:
        size += sizeof(int32_t);
        break;
      case FieldType::FLOAT:
        size += sizeof(float);
        break;
      case FieldType::BOOLEAN:
        size += sizeof(bool);
        break;
      case FieldType::VARCHAR:
        size += sizeof(uint32_t) + GetString().length();
        break;
      case FieldType::TIMESTAMP:
        size += sizeof(int64_t);
        break;
      case FieldType::CHAR:
        size += sizeof(uint32_t) + GetChar().size();
        break;
      case FieldType::DATE:
        size += sizeof(int16_t) + 2 * sizeof(uint8_t);  // year + month + day
        break;
      case FieldType::TIME:
        size += 3 * sizeof(uint8_t);  // hour + minute + second
        break;
      default:
        break;
    }
  }
  return size;
}

std::string Value::ToString() const {
  if (is_null_) {
    return "NULL";
  }

  std::stringstream ss;
  switch (type_) {
    case FieldType::INTEGER:
      ss << GetInteger();
      break;
    case FieldType::FLOAT:
      ss << GetFloat();
      break;
    case FieldType::BOOLEAN:
      ss << (GetBoolean() ? "true" : "false");
      break;
    case FieldType::VARCHAR:
      ss << GetString();
      break;
    case FieldType::TIMESTAMP:
      ss << GetTimestamp();
      break;
    case FieldType::CHAR:
      ss << GetChar().ToString();
      break;
    case FieldType::DATE:
      ss << GetDate().ToString();
      break;
    case FieldType::TIME:
      ss << GetTime().ToString();
      break;
    default:
      ss << "INVALID";
      break;
  }
  return ss.str();
}

} // namespace dimdb
