#include <gtest/gtest.h>
#include "common/value.h"

namespace dimdb {

class ValueTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

// 测试构造函数和基本类型获取
TEST_F(ValueTest, BasicConstructionAndGetters) {
  // 测试整数
  Value int_val(42);
  EXPECT_EQ(int_val.GetType(), FieldType::INTEGER);
  EXPECT_EQ(int_val.GetInteger(), 42);
  EXPECT_FALSE(int_val.IsNull());

  // 测试浮点数
  Value float_val(3.14f);
  EXPECT_EQ(float_val.GetType(), FieldType::FLOAT);
  EXPECT_FLOAT_EQ(float_val.GetFloat(), 3.14f);
  EXPECT_FALSE(float_val.IsNull());

  // 测试布尔值
  Value bool_val(true);
  EXPECT_EQ(bool_val.GetType(), FieldType::BOOLEAN);
  EXPECT_TRUE(bool_val.GetBoolean());
  EXPECT_FALSE(bool_val.IsNull());

  // 测试字符串
  Value string_val(std::string("hello"));
  EXPECT_EQ(string_val.GetType(), FieldType::VARCHAR);
  EXPECT_EQ(string_val.GetString(), "hello");
  EXPECT_FALSE(string_val.IsNull());

  // 测试时间戳
  Value timestamp_val(int64_t(1234567890));
  EXPECT_EQ(timestamp_val.GetType(), FieldType::TIMESTAMP);
  EXPECT_EQ(timestamp_val.GetTimestamp(), 1234567890);
  EXPECT_FALSE(timestamp_val.IsNull());

  // 测试空值
  Value null_val;
  EXPECT_EQ(null_val.GetType(), FieldType::INVALID);
  EXPECT_TRUE(null_val.IsNull());
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
    original.SerializeTo(buffer);
    Value deserialized = Value::DeserializeFrom(buffer, FieldType::INTEGER);
    EXPECT_EQ(original, deserialized);
  }

  // 测试浮点数
  {
    Value original(3.14f);
    char buffer[32];
    original.SerializeTo(buffer);
    Value deserialized = Value::DeserializeFrom(buffer, FieldType::FLOAT);
    EXPECT_EQ(original, deserialized);
  }

  // 测试字符串
  {
    Value original(std::string("hello world"));
    char buffer[64];
    original.SerializeTo(buffer);
    Value deserialized = Value::DeserializeFrom(buffer, FieldType::VARCHAR);
    EXPECT_EQ(original, deserialized);
  }

  // 测试空值
  {
    Value original;
    char buffer[32];
    original.SerializeTo(buffer);
    Value deserialized = Value::DeserializeFrom(buffer, FieldType::INVALID);
    EXPECT_EQ(original.IsNull(), deserialized.IsNull());
  }
}

// 测试序列化大小计算
TEST_F(ValueTest, SerializedSize) {
  // 测试整数
  Value int_val(42);
  EXPECT_EQ(int_val.GetSerializedSize(), 
            sizeof(FieldType) + sizeof(bool) + sizeof(int32_t));

  // 测试字符串
  std::string test_str = "hello world";
  Value string_val(test_str);
  EXPECT_EQ(string_val.GetSerializedSize(), 
            sizeof(FieldType) + sizeof(bool) + sizeof(uint32_t) + test_str.length());

  // 测试空值
  Value null_val;
  EXPECT_EQ(null_val.GetSerializedSize(), sizeof(FieldType) + sizeof(bool));
}

// 测试ToString方法
TEST_F(ValueTest, ToString) {
  // 测试整数
  Value int_val(42);
  EXPECT_EQ(int_val.ToString(), "42");

  // 测试浮点数
  Value float_val(3.14f);
  EXPECT_EQ(float_val.ToString(), "3.14");

  // 测试布尔值
  Value bool_val(true);
  EXPECT_EQ(bool_val.ToString(), "true");

  // 测试字符串
  Value string_val(std::string("hello"));
  EXPECT_EQ(string_val.ToString(), "hello");

  // 测试空值
  Value null_val;
  EXPECT_EQ(null_val.ToString(), "NULL");
}

// 测试异常情况
TEST_F(ValueTest, ExceptionCases) {
  // 测试类型不匹配的反序列化
  Value int_val(42);
  char buffer[32];
  int_val.SerializeTo(buffer);
  EXPECT_THROW(Value::DeserializeFrom(buffer, FieldType::VARCHAR), std::runtime_error);

  // 测试获取错误类型的值
  EXPECT_THROW(int_val.GetString(), std::bad_variant_access);
  EXPECT_THROW(int_val.GetFloat(), std::bad_variant_access);
}

// 测试新增类型的构造和获取
TEST_F(ValueTest, ExtendedTypesConstructionAndGetters) {
  // 测试定长字符串
  CharString char_str("test", 10);
  Value char_val(char_str);
  EXPECT_EQ(char_val.GetType(), FieldType::CHAR);
  EXPECT_EQ(char_val.GetChar().size(), 10);
  EXPECT_EQ(char_val.GetChar().ToString(), "test      "); // 应该有6个空格填充

  // 测试日期
  Date date(2024, 3, 3);
  Value date_val(date);
  EXPECT_EQ(date_val.GetType(), FieldType::DATE);
  EXPECT_EQ(date_val.GetDate().GetYear(), 2024);
  EXPECT_EQ(date_val.GetDate().GetMonth(), 3);
  EXPECT_EQ(date_val.GetDate().GetDay(), 3);

  // 测试时间
  Time time(14, 30, 45);
  Value time_val(time);
  EXPECT_EQ(time_val.GetType(), FieldType::TIME);
  EXPECT_EQ(time_val.GetTime().GetHour(), 14);
  EXPECT_EQ(time_val.GetTime().GetMinute(), 30);
  EXPECT_EQ(time_val.GetTime().GetSecond(), 45);
}

// 测试新增类型的比较操作
TEST_F(ValueTest, ExtendedTypesComparison) {
  // 测试定长字符串比较
  Value char1(CharString("abc", 5));
  Value char2(CharString("def", 5));
  EXPECT_TRUE(char1 < char2);
  EXPECT_FALSE(char1 == char2);

  // 测试日期比较
  Value date1(Date(2024, 3, 3));
  Value date2(Date(2024, 3, 4));
  EXPECT_TRUE(date1 < date2);
  EXPECT_FALSE(date1 >= date2);

  // 测试时间比较
  Value time1(Time(14, 30, 0));
  Value time2(Time(14, 30, 1));
  EXPECT_TRUE(time1 < time2);
  EXPECT_FALSE(time1 == time2);
}

// 测试新增类型的序列化和反序列化
TEST_F(ValueTest, ExtendedTypesSerializationDeserialization) {
  // 测试定长字符串
  {
    Value original(CharString("test", 10));
    char buffer[64];
    original.SerializeTo(buffer);
    Value deserialized = Value::DeserializeFrom(buffer, FieldType::CHAR);
    EXPECT_EQ(original, deserialized);
    EXPECT_EQ(original.GetChar().ToString(), deserialized.GetChar().ToString());
  }

  // 测试日期
  {
    Value original(Date(2024, 3, 3));
    char buffer[32];
    original.SerializeTo(buffer);
    Value deserialized = Value::DeserializeFrom(buffer, FieldType::DATE);
    EXPECT_EQ(original, deserialized);
    EXPECT_EQ(original.GetDate().ToString(), deserialized.GetDate().ToString());
  }

  // 测试时间
  {
    Value original(Time(14, 30, 45));
    char buffer[32];
    original.SerializeTo(buffer);
    Value deserialized = Value::DeserializeFrom(buffer, FieldType::TIME);
    EXPECT_EQ(original, deserialized);
    EXPECT_EQ(original.GetTime().ToString(), deserialized.GetTime().ToString());
  }
}

// 测试新增类型的序列化大小计算
TEST_F(ValueTest, ExtendedTypesSerializedSize) {
  // 测试定长字符串
  CharString char_str("test", 10);
  Value char_val(char_str);
  EXPECT_EQ(char_val.GetSerializedSize(),
            sizeof(FieldType) + sizeof(bool) + sizeof(uint32_t) + 10);

  // 测试日期
  Value date_val(Date(2024, 3, 3));
  EXPECT_EQ(date_val.GetSerializedSize(),
            sizeof(FieldType) + sizeof(bool) + sizeof(int16_t) + 2 * sizeof(uint8_t));

  // 测试时间
  Value time_val(Time(14, 30, 45));
  EXPECT_EQ(time_val.GetSerializedSize(),
            sizeof(FieldType) + sizeof(bool) + 3 * sizeof(uint8_t));
}

// 测试新增类型的ToString方法
TEST_F(ValueTest, ExtendedTypesToString) {
  // 测试定长字符串
  Value char_val(CharString("test", 10));
  EXPECT_EQ(char_val.ToString(), "test      ");

  // 测试日期
  Value date_val(Date(2024, 3, 3));
  EXPECT_EQ(date_val.ToString(), "2024-03-03");

  // 测试时间
  Value time_val(Time(14, 30, 45));
  EXPECT_EQ(time_val.ToString(), "14:30:45");
}

// 测试新增类型的异常情况
TEST_F(ValueTest, ExtendedTypesExceptionCases) {
  // 测试无效的日期
  EXPECT_THROW(Date("invalid-date"), std::invalid_argument);
  
  // 测试无效的时间
  EXPECT_THROW(Time("invalid-time"), std::invalid_argument);

  // 测试类型不匹配
  Value char_val(CharString("test", 10));
  EXPECT_THROW(char_val.GetDate(), std::bad_variant_access);
  EXPECT_THROW(char_val.GetTime(), std::bad_variant_access);
}

// 测试字符串构造的日期和时间
TEST_F(ValueTest, StringConstructedDateTime) {
  // 测试从字符串构造日期
  Date date("2024-03-03");
  Value date_val(date);
  EXPECT_EQ(date_val.GetDate().ToString(), "2024-03-03");

  // 测试从字符串构造时间
  Time time("14:30:45");
  Value time_val(time);
  EXPECT_EQ(time_val.GetTime().ToString(), "14:30:45");
}

// 测试时间戳构造的日期和时间
TEST_F(ValueTest, TimestampConstructedDateTime) {
  // 使用固定的时间戳进行测试
  time_t timestamp = 1709467845; // 2024-03-03 14:30:45

  // 测试从时间戳构造日期
  Date date(timestamp);
  Value date_val(date);
  EXPECT_EQ(date_val.GetDate().GetYear(), 2024);
  EXPECT_EQ(date_val.GetDate().GetMonth(), 3);
  EXPECT_EQ(date_val.GetDate().GetDay(), 3);

  // 测试从时间戳构造时间
  Time time(timestamp);
  Value time_val(time);
  EXPECT_EQ(time_val.GetTime().GetHour(), 20);
  EXPECT_EQ(time_val.GetTime().GetMinute(), 10);
  EXPECT_EQ(time_val.GetTime().GetSecond(), 45);
}

} // namespace dimdb

// 主函数
int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
