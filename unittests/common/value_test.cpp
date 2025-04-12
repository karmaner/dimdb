#include <gtest/gtest.h>
#include "common/value.h"

namespace common {

class ValueTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// 测试构造函数和基本类型获取
TEST_F(ValueTest, BasicConstructionAndGetters) {
  // 测试整数
  Value int_val(42);
  EXPECT_EQ(int_val.get_type(), ArrrType::INTEGER);
  EXPECT_EQ(int_val.get_integer(), 42);
  EXPECT_FALSE(int_val.is_null());

  // 测试浮点数
  Value float_val(3.14f);
  EXPECT_EQ(float_val.get_type(), ArrrType::FLOAT);
  EXPECT_FLOAT_EQ(float_val.get_float(), 3.14f);
  EXPECT_FALSE(float_val.is_null());

  // 测试布尔值
  Value bool_val(true);
  EXPECT_EQ(bool_val.get_type(), ArrrType::BOOLEAN);
  EXPECT_TRUE(bool_val.get_boolean());
  EXPECT_FALSE(bool_val.is_null());

  // 测试字符串
  Value string_val(std::string("hello"));
  EXPECT_EQ(string_val.get_type(), ArrrType::VARCHAR);
  EXPECT_EQ(string_val.get_string(), "hello");
  EXPECT_FALSE(string_val.is_null());

  // 测试时间戳
  Value timestamp_val(int64_t(1234567890));
  EXPECT_EQ(timestamp_val.get_type(), ArrrType::TIMESTAMP);
  EXPECT_EQ(timestamp_val.get_timestamp(), 1234567890);
  EXPECT_FALSE(timestamp_val.is_null());

  // 测试空值
  Value null_val;
  EXPECT_EQ(null_val.get_type(), ArrrType::UNDEFINED);
  EXPECT_TRUE(null_val.is_null());
}

// 测试比较操作符
TEST_F(ValueTest, ComparisonOperators) {
  // 整数比较
  Value int1(10);
  Value int2(20);
  EXPECT_TRUE(int1 < int2);
  EXPECT_TRUE(int1 <= int2);
  EXPECT_FALSE(int1 > int2);
  EXPECT_FALSE(int1 >= int2);
  EXPECT_FALSE(int1 == int2);
  EXPECT_TRUE(int1 != int2);

  // 相同值比较
  Value int3(10);
  EXPECT_TRUE(int1 == int3);
  EXPECT_FALSE(int1 != int3);

  // 不同类型比较
  Value float1(10.0f);
  EXPECT_FALSE(int1 == float1);
  EXPECT_TRUE(int1 != float1);

  // 空值比较
  Value null1;
  Value null2;
  EXPECT_TRUE(null1 == null2);
  EXPECT_FALSE(null1 != null2);
}

// 测试序列化和反序列化
TEST_F(ValueTest, SerializationDeserialization) {
  // 测试整数
  {
    Value original(42);
    char buffer[32];
    original.serialize_to(buffer);
    Value deserialized = Value::deserialize_from(buffer, ArrrType::INTEGER);
    EXPECT_EQ(original, deserialized);
  }

  // 测试浮点数
  {
    Value original(3.14f);
    char buffer[32];
    original.serialize_to(buffer);
    Value deserialized = Value::deserialize_from(buffer, ArrrType::FLOAT);
    EXPECT_EQ(original, deserialized);
  }

  // 测试字符串
  {
    std::string test_str = "hello world";
    Value original(static_cast<std::string>(test_str));
    char buffer[64];
    original.serialize_to(buffer);
    Value deserialized = Value::deserialize_from(buffer, ArrrType::VARCHAR);
    EXPECT_EQ(original, deserialized);
  }

  // 测试空值
  {
    Value original;
    char buffer[32];
    original.serialize_to(buffer);
    Value deserialized = Value::deserialize_from(buffer, ArrrType::UNDEFINED);
    EXPECT_EQ(original.is_null(), deserialized.is_null());
  }
}

// 测试序列化大小计算
TEST_F(ValueTest, SerializedSize) {
  // 测试整数
  Value int_val(42);
  EXPECT_EQ(int_val.get_serialized_size(), 
            sizeof(ArrrType) + sizeof(bool) + sizeof(int32_t));

  // 测试字符串
  std::string test_str = "hello world";
  Value string_val(static_cast<std::string>(test_str));
  EXPECT_EQ(string_val.get_serialized_size(), 
            sizeof(ArrrType) + sizeof(bool) + sizeof(uint32_t) + test_str.length());

  // 测试空值
  Value null_val;
  EXPECT_EQ(null_val.get_serialized_size(), sizeof(ArrrType) + sizeof(bool));
}

// 测试to_string方法
TEST_F(ValueTest, ToString) {
  // 测试整数
  Value int_val(42);
  EXPECT_EQ(int_val.to_string(), "42");

  // 测试浮点数
  Value float_val(3.14f);
  EXPECT_EQ(float_val.to_string(), "3.14");

  // 测试布尔值
  Value bool_val(true);
  EXPECT_EQ(bool_val.to_string(), "true");

  // 测试字符串
  Value string_val(std::string("hello"));
  EXPECT_EQ(string_val.to_string(), "hello");

  // 测试空值
  Value null_val;
  EXPECT_EQ(null_val.to_string(), "NULL");
}

// 测试异常情况
TEST_F(ValueTest, ExceptionCases) {
  // 测试类型不匹配的反序列化
  Value int_val(42);
  char buffer[32];
  int_val.serialize_to(buffer);
  EXPECT_THROW(Value::deserialize_from(buffer, ArrrType::VARCHAR), std::runtime_error);

  // 测试获取错误类型的值
  EXPECT_THROW(int_val.get_string(), std::bad_variant_access);
  EXPECT_THROW(int_val.get_float(), std::bad_variant_access);
}

// 测试定长字符串类型
TEST_F(ValueTest, CharStringType) {
  // 测试定长字符串构造和获取
  CharString char_str("test", 10);
  Value char_val(char_str);
  EXPECT_EQ(char_val.get_type(), ArrrType::CHAR);
  EXPECT_EQ(char_val.get_char().size(), 10);
  EXPECT_EQ(char_val.get_char().to_string(), "test      "); // 应该有6个空格填充

  // 测试定长字符串比较
  Value char1(CharString("abc", 5));
  Value char2(CharString("def", 5));
  EXPECT_TRUE(char1 < char2);
  EXPECT_FALSE(char1 == char2);

  // 测试定长字符串序列化和反序列化
  {
    Value original(CharString("test", 10));
    char buffer[64];
    original.serialize_to(buffer);
    Value deserialized = Value::deserialize_from(buffer, ArrrType::CHAR);
    EXPECT_EQ(original, deserialized);
    EXPECT_EQ(original.get_char().to_string(), deserialized.get_char().to_string());
  }

  // 测试定长字符串序列化大小
  EXPECT_EQ(char_val.get_serialized_size(),
            sizeof(ArrrType) + sizeof(bool) + sizeof(uint32_t) + 10);

  // 测试定长字符串to_string
  EXPECT_EQ(char_val.to_string(), "test      ");
}

// 测试时间戳相关功能
TEST_F(ValueTest, TimestampFunctionality) {
  // 测试从时间戳构造
  int64_t timestamp = 1709467845; // 2024-03-03 14:30:45
  Value timestamp_val(timestamp);
  EXPECT_EQ(timestamp_val.get_timestamp(), timestamp);

  // 测试时间戳序列化和反序列化
  {
    char buffer[32];
    timestamp_val.serialize_to(buffer);
    Value deserialized = Value::deserialize_from(buffer, ArrrType::TIMESTAMP);
    EXPECT_EQ(timestamp_val, deserialized);
  }

  // 测试时间戳比较
  Value timestamp1(1709467845);
  Value timestamp2(1709467846);
  EXPECT_TRUE(timestamp1 < timestamp2);
  EXPECT_FALSE(timestamp1 == timestamp2);
}

} // namespace common

// 主函数
int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
