/* SPDX-License-Identifier: MIT */
/**
 * @file    weixin_protocol.c
 * @brief   WEIXIN Serial Monitor 下位机文本协议实现。
 * @author  weixin
 */
#include "weixin_protocol/weixin_protocol.h"

#include "math_utils.h"
#include "result.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 协议最多需要 param、set、name、value 四个参数，额外槽用于检测多参。 */
#define WEIXIN_PROTOCOL_ARGUMENT_CAPACITY UINT8_C(5)
/* 防止异常参数表导致命令处理时间不可控。 */
#define WEIXIN_PROTOCOL_PARAM_COUNT_MAX UINT16_C(64)
/* 协议参数名最大长度，不包含字符串结尾的 NUL。 */
#define WEIXIN_PROTOCOL_PARAM_NAME_MAX UINT16_C(48)
/* 默认协议只接收可打印 ASCII 字符。 */
#define WEIXIN_PROTOCOL_ASCII_MIN UINT8_C(0x20)
/* 默认协议可打印 ASCII 的最大值。 */
#define WEIXIN_PROTOCOL_ASCII_MAX UINT8_C(0x7E)

/**
 * @brief  判断 double 是否为有限值。
 * @param  value：待检查数值。
 * @retval true：value 不是 NaN 或正负无穷大。
 * @retval false：value 是 NaN 或正负无穷大。
 */
static bool weixin_double_is_finite(double value)
{
  return isfinite(value) != 0;
}

/**
 * @brief  判断参数类型是否受当前协议组件支持。
 * @param  type：WEIXIN_PARAM_TYPE_* 类型编号。
 * @retval true：类型受支持。
 * @retval false：类型无效。
 */
static bool weixin_param_type_is_valid(uint8_t type)
{
  return type <= WEIXIN_PARAM_TYPE_FLOAT;
}

/**
 * @brief  检查协议参数名是否符合无空白稳定标识符要求。
 * @param  name：待检查的参数名。
 * @retval true：参数名合法。
 * @retval false：参数名为空、过长或包含空白字符。
 */
static bool weixin_param_name_is_valid(const char *name)
{
  uint16_t index;

  if ((name == NULL) || (name[0] == '\0')) {
    return false;
  }

  for (index = 0U; index <= WEIXIN_PROTOCOL_PARAM_NAME_MAX; index++) {
    if (name[index] == '\0') {
      return true;
    }
    if ((name[index] == ' ') || (name[index] == '\t') ||
        (name[index] == '\r') || (name[index] == '\n')) {
      return false;
    }
  }

  return false;
}

/**
 * @brief  检查单个协议参数描述是否合法。
 * @param  param：待检查的参数描述。
 * @retval true：参数描述可以安全使用。
 * @retval false：类型、地址、名称或范围无效。
 */
static bool weixin_param_is_valid(const struct weixin_param *param)
{
  if ((param == NULL) || (param->value == NULL) ||
      (!weixin_param_name_is_valid(param->name)) ||
      (!weixin_param_type_is_valid(param->type)) ||
      (!weixin_double_is_finite(param->minimum)) ||
      (!weixin_double_is_finite(param->maximum))) {
    return false;
  }

  return param->minimum <= param->maximum;
}

/**
 * @brief  检查参数表中是否存在重名或非法描述。
 * @param  params：待检查的参数表。
 * @param  count：参数数量。
 * @retval PLATFORM_OK：参数表合法。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：参数表无效。
 */
static int weixin_params_validate(const struct weixin_param *params,
                                  uint16_t count)
{
  uint16_t current;
  uint16_t previous;

  if ((count == 0U) || (count > WEIXIN_PROTOCOL_PARAM_COUNT_MAX)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  for (current = 0U; current < count; current++) {
    if (!weixin_param_is_valid(&params[current])) {
      return PLATFORM_ERR_INVALID_CONFIGURATION;
    }
    for (previous = 0U; previous < current; previous++) {
      if (strcmp(params[current].name, params[previous].name) == 0) {
        return PLATFORM_ERR_INVALID_CONFIGURATION;
      }
    }
  }

  return PLATFORM_OK;
}

/**
 * @brief  直接发送一条已经格式化完成的协议响应。
 * @param  self：协议对象。
 * @param  text：以 NUL 结尾的响应文本。
 * @return 写回函数返回的 PLATFORM_* 状态。
 */
static int weixin_send_literal(struct weixin_protocol *self,
                               const char *text)
{
  size_t length;

  length = strlen(text);
  if (length > UINT16_MAX) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  return self->write(self->write_context,
                     (const uint8_t *)text,
                     (uint16_t)length);
}

/**
 * @brief  格式化并发送一条协议响应。
 * @param  self：协议对象。
 * @param  format：printf 风格的响应格式。
 * @return PLATFORM_OK 表示发送成功；缓冲区不足时返回错误响应结果。
 */
static int weixin_send_format(struct weixin_protocol *self,
                              const char *format,
                              ...)
{
  va_list arguments;
  int written;

  va_start(arguments, format);
  written = vsnprintf(self->response,
                      sizeof(self->response),
                      format,
                      arguments);
  va_end(arguments);

  if ((written < 0) ||
      ((uint32_t)written >= sizeof(self->response))) {
    return weixin_send_literal(self, "[param] err buf\r\n");
  }

  return weixin_send_literal(self, self->response);
}

/**
 * @brief  把命令行原地切分为空格或 Tab 分隔的参数。
 * @param  line：可修改的 NUL 结尾命令行。
 * @param  arguments：用于接收参数地址的数组。
 * @return 参数数量；达到容量时返回容量值以便检测多余参数。
 */
static uint8_t weixin_split_arguments(char *line, char **arguments)
{
  char *cursor = line;
  uint8_t count = 0U;

  while (*cursor != '\0') {
    while ((*cursor == ' ') || (*cursor == '\t')) {
      cursor++;
    }
    if (*cursor == '\0') {
      break;
    }
    if (count >= WEIXIN_PROTOCOL_ARGUMENT_CAPACITY) {
      return WEIXIN_PROTOCOL_ARGUMENT_CAPACITY;
    }

    arguments[count] = cursor;
    count++;
    while ((*cursor != '\0') &&
           (*cursor != ' ') && (*cursor != '\t')) {
      cursor++;
    }
    if (*cursor != '\0') {
      *cursor = '\0';
      cursor++;
    }
  }

  return count;
}

/**
 * @brief  查找指定协议名对应的参数描述。
 * @param  self：协议对象。
 * @param  name：上位机传入的参数名。
 * @return 找到时返回参数描述地址，否则返回 NULL。
 */
static const struct weixin_param *weixin_find_param(
    const struct weixin_protocol *self,
    const char *name)
{
  uint16_t index;

  for (index = 0U; index < self->param_count; index++) {
    if (strcmp(self->params[index].name, name) == 0) {
      return &self->params[index];
    }
  }

  return NULL;
}

/**
 * @brief  获取参数类型对应的默认协议类型名。
 * @param  type：WEIXIN_PARAM_TYPE_* 类型编号。
 * @return 类型名；类型无效时返回 NULL。
 */
static const char *weixin_param_type_name(uint8_t type)
{
  static const char *const type_names[] = {
    "u8", "i8", "u16", "i16", "u32", "i32", "float"
  };

  if (!weixin_param_type_is_valid(type)) {
    return NULL;
  }

  return type_names[type];
}

/**
 * @brief  输出参数表和结束计数。
 * @param  self：协议对象。
 * @return PLATFORM_OK 表示全部发送成功；其他值表示写回失败。
 */
static int weixin_command_list(struct weixin_protocol *self)
{
  const struct weixin_param *param;
  const char *type_name;
  uint16_t index;
  int result;

  for (index = 0U; index < self->param_count; index++) {
    param = &self->params[index];
    type_name = weixin_param_type_name(param->type);
    result = weixin_send_format(
        self, "[param] %s %s %.6f %.6f %s\r\n",
        param->name, type_name, param->minimum, param->maximum,
        param->writable ? "rw" : "ro");
    if (result != PLATFORM_OK) {
      return result;
    }
  }

  return weixin_send_format(self, "[param] ok list %u\r\n",
                            (unsigned int)self->param_count);
}

/**
 * @brief  返回指定参数的当前值。
 * @param  self：协议对象。
 * @param  param：待读取的参数描述。
 * @return PLATFORM_OK 表示发送成功；其他值表示类型或写回失败。
 */
static int weixin_report_param_value(
    struct weixin_protocol *self,
    const struct weixin_param *param)
{
  switch (param->type) {
    case WEIXIN_PARAM_TYPE_U8:
      return weixin_send_format(self, "[param] ok %s=%u\r\n",
                                param->name,
                                (unsigned int)*(uint8_t *)param->value);
    case WEIXIN_PARAM_TYPE_I8:
      return weixin_send_format(self, "[param] ok %s=%d\r\n",
                                param->name,
                                (int)*(int8_t *)param->value);
    case WEIXIN_PARAM_TYPE_U16:
      return weixin_send_format(self, "[param] ok %s=%u\r\n",
                                param->name,
                                (unsigned int)*(uint16_t *)param->value);
    case WEIXIN_PARAM_TYPE_I16:
      return weixin_send_format(self, "[param] ok %s=%d\r\n",
                                param->name,
                                (int)*(int16_t *)param->value);
    case WEIXIN_PARAM_TYPE_U32:
      return weixin_send_format(
          self, "[param] ok %s=%lu\r\n", param->name,
          (unsigned long)*(uint32_t *)param->value);
    case WEIXIN_PARAM_TYPE_I32:
      return weixin_send_format(self, "[param] ok %s=%ld\r\n",
                                param->name,
                                (long)*(int32_t *)param->value);
    case WEIXIN_PARAM_TYPE_FLOAT:
      return weixin_send_format(self, "[param] ok %s=%.6f\r\n",
                                param->name,
                                (double)*(float *)param->value);
    default:
      return weixin_send_literal(self, "[param] err type\r\n");
  }
}

/**
 * @brief  查找并返回指定参数值。
 * @param  self：协议对象。
 * @param  name：上位机参数名。
 * @return PLATFORM_OK 表示命令响应已发送；其他值表示写回失败。
 */
static int weixin_command_get(struct weixin_protocol *self,
                              const char *name)
{
  const struct weixin_param *param;

  param = weixin_find_param(self, name);
  if (param == NULL) {
    return weixin_send_literal(self, "[param] err not_found\r\n");
  }

  return weixin_report_param_value(self, param);
}

/**
 * @brief  严格解析无符号十进制整数。
 * @param  text：待解析的 NUL 结尾字符串。
 * @param  value：用于接收解析结果的地址。
 * @retval true：完整字符串被成功解析且未溢出。
 * @retval false：字符串包含符号、尾随字符或发生溢出。
 */
static bool weixin_parse_unsigned(const char *text,
                                  unsigned long *value)
{
  char *end;
  unsigned long parsed;

  if ((text == NULL) || (text[0] == '\0') || (text[0] == '-')) {
    return false;
  }

  errno = 0;
  parsed = strtoul(text, &end, 10);
  if ((end == text) || (*end != '\0') || (errno == ERANGE)) {
    return false;
  }

  *value = parsed;
  return true;
}

/**
 * @brief  严格解析有符号十进制整数。
 * @param  text：待解析的 NUL 结尾字符串。
 * @param  value：用于接收解析结果的地址。
 * @retval true：完整字符串被成功解析且未溢出。
 * @retval false：字符串包含尾随字符或发生溢出。
 */
static bool weixin_parse_signed(const char *text, long *value)
{
  char *end;
  long parsed;

  if ((text == NULL) || (text[0] == '\0')) {
    return false;
  }

  errno = 0;
  parsed = strtol(text, &end, 10);
  if ((end == text) || (*end != '\0') || (errno == ERANGE)) {
    return false;
  }

  *value = parsed;
  return true;
}

/**
 * @brief  严格解析单精度浮点参数。
 * @param  text：待解析的 NUL 结尾字符串。
 * @param  value：用于接收解析结果的地址。
 * @retval true：完整字符串解析为有限 float。
 * @retval false：字符串非法、溢出或结果不是有限值。
 */
static bool weixin_parse_float(const char *text, float *value)
{
  char *end;
  double parsed;
  float converted;

  if ((text == NULL) || (text[0] == '\0')) {
    return false;
  }

  errno = 0;
  parsed = strtod(text, &end);
  converted = (float)parsed;
  if ((end == text) || (*end != '\0') || (errno == ERANGE) ||
      (!weixin_double_is_finite(parsed)) ||
      (!math_is_finite(converted))) {
    return false;
  }

  *value = converted;
  return true;
}

/**
 * @brief  判断解析值是否位于参数声明范围内。
 * @param  param：参数描述。
 * @param  value：已经完成类型解析的数值。
 * @retval true：数值位于闭区间内。
 * @retval false：数值超出参数范围。
 */
static bool weixin_value_is_in_range(const struct weixin_param *param,
                                     double value)
{
  return (value >= param->minimum) && (value <= param->maximum);
}

/**
 * @brief  解析并写入无符号整数参数。
 * @param  param：目标参数。
 * @param  text：上位机发送的十进制文本。
 * @retval PLATFORM_OK：写入成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：类型解析失败。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：数值越界。
 */
static int weixin_write_unsigned(const struct weixin_param *param,
                                 const char *text)
{
  unsigned long value;
  unsigned long type_maximum;

  if (!weixin_parse_unsigned(text, &value)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  switch (param->type) {
    case WEIXIN_PARAM_TYPE_U8:
      type_maximum = UINT8_MAX;
      break;
    case WEIXIN_PARAM_TYPE_U16:
      type_maximum = UINT16_MAX;
      break;
    case WEIXIN_PARAM_TYPE_U32:
      type_maximum = UINT32_MAX;
      break;
    default:
      return PLATFORM_ERR_UNSUPPORTED;
  }

  if ((value > type_maximum) ||
      (!weixin_value_is_in_range(param, (double)value))) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  if (param->type == WEIXIN_PARAM_TYPE_U8) {
    *(uint8_t *)param->value = (uint8_t)value;
  } else if (param->type == WEIXIN_PARAM_TYPE_U16) {
    *(uint16_t *)param->value = (uint16_t)value;
  } else {
    *(uint32_t *)param->value = (uint32_t)value;
  }
  return PLATFORM_OK;
}

/**
 * @brief  解析并写入有符号整数参数。
 * @param  param：目标参数。
 * @param  text：上位机发送的十进制文本。
 * @retval PLATFORM_OK：写入成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：类型解析失败。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：数值越界。
 */
static int weixin_write_signed(const struct weixin_param *param,
                               const char *text)
{
  long value;
  long type_minimum;
  long type_maximum;

  if (!weixin_parse_signed(text, &value)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  switch (param->type) {
    case WEIXIN_PARAM_TYPE_I8:
      type_minimum = INT8_MIN;
      type_maximum = INT8_MAX;
      break;
    case WEIXIN_PARAM_TYPE_I16:
      type_minimum = INT16_MIN;
      type_maximum = INT16_MAX;
      break;
    case WEIXIN_PARAM_TYPE_I32:
      type_minimum = INT32_MIN;
      type_maximum = INT32_MAX;
      break;
    default:
      return PLATFORM_ERR_UNSUPPORTED;
  }

  if ((value < type_minimum) || (value > type_maximum) ||
      (!weixin_value_is_in_range(param, (double)value))) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  if (param->type == WEIXIN_PARAM_TYPE_I8) {
    *(int8_t *)param->value = (int8_t)value;
  } else if (param->type == WEIXIN_PARAM_TYPE_I16) {
    *(int16_t *)param->value = (int16_t)value;
  } else {
    *(int32_t *)param->value = (int32_t)value;
  }
  return PLATFORM_OK;
}

/**
 * @brief  解析并写入单精度浮点参数。
 * @param  param：目标参数。
 * @param  text：上位机发送的浮点文本。
 * @retval PLATFORM_OK：写入成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：类型解析失败。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：数值越界。
 */
static int weixin_write_float(const struct weixin_param *param,
                              const char *text)
{
  float value;

  if (!weixin_parse_float(text, &value)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (!weixin_value_is_in_range(param, (double)value)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  *(float *)param->value = value;
  return PLATFORM_OK;
}

/**
 * @brief  按参数类型解析并写入文本值。
 * @param  param：目标参数。
 * @param  text：上位机发送的值文本。
 * @return PLATFORM_OK 表示写入成功；其他值表示类型或范围错误。
 */
static int weixin_write_param(const struct weixin_param *param,
                              const char *text)
{
  if ((param->type == WEIXIN_PARAM_TYPE_U8) ||
      (param->type == WEIXIN_PARAM_TYPE_U16) ||
      (param->type == WEIXIN_PARAM_TYPE_U32)) {
    return weixin_write_unsigned(param, text);
  }
  if ((param->type == WEIXIN_PARAM_TYPE_I8) ||
      (param->type == WEIXIN_PARAM_TYPE_I16) ||
      (param->type == WEIXIN_PARAM_TYPE_I32)) {
    return weixin_write_signed(param, text);
  }
  if (param->type == WEIXIN_PARAM_TYPE_FLOAT) {
    return weixin_write_float(param, text);
  }

  return PLATFORM_ERR_UNSUPPORTED;
}

/**
 * @brief  校验并设置指定参数，然后回读实际值。
 * @param  self：协议对象。
 * @param  name：上位机参数名。
 * @param  text：待写入的值文本。
 * @return PLATFORM_OK 表示命令响应已发送；其他值表示写回失败。
 */
static int weixin_command_set(struct weixin_protocol *self,
                              const char *name,
                              const char *text)
{
  const struct weixin_param *param;
  int result;

  param = weixin_find_param(self, name);
  if (param == NULL) {
    return weixin_send_literal(self, "[param] err not_found\r\n");
  }
  if (!param->writable) {
    return weixin_send_literal(self, "[param] err readonly\r\n");
  }

  result = weixin_write_param(param, text);
  if (result == PLATFORM_ERR_INVALID_ARGUMENT) {
    return weixin_send_literal(self, "[param] err type\r\n");
  }
  if (result == PLATFORM_ERR_INVALID_CONFIGURATION) {
    return weixin_send_literal(self,
                               "[param] err out_of_range\r\n");
  }
  if (result != PLATFORM_OK) {
    return weixin_send_literal(self, "[param] err type\r\n");
  }

  return weixin_report_param_value(self, param);
}

/**
 * @brief  分发一条完整命令行。
 * @param  self：协议对象。
 * @param  line：可修改的 NUL 结尾命令行。
 * @return PLATFORM_OK 表示处理成功；其他值表示写回失败。
 */
static int weixin_handle_line(struct weixin_protocol *self, char *line)
{
  char *arguments[WEIXIN_PROTOCOL_ARGUMENT_CAPACITY];
  const char *readonly_arguments[WEIXIN_PROTOCOL_ARGUMENT_CAPACITY];
  uint8_t count;
  uint8_t index;

  count = weixin_split_arguments(line, arguments);
  if (count == 0U) {
    return PLATFORM_OK;
  }

  /*
   * 应用命令优先于内置 param 命令分发。回调只获得只读参数视图，
   * 避免应用层意外修改协议组件正在处理的命令缓冲区。
   */
  if (self->command_handler != NULL) {
    for (index = 0U; index < count; index++) {
      readonly_arguments[index] = arguments[index];
    }
    if (self->command_handler(self->command_context, count,
                              readonly_arguments)) {
      return PLATFORM_OK;
    }
  }

  if (strcmp(arguments[0], "param") != 0) {
    /* 同一 UART 上的普通日志或其他协议必须静默忽略。 */
    return PLATFORM_OK;
  }

  if ((count == 2U) && (strcmp(arguments[1], "list") == 0)) {
    return weixin_command_list(self);
  }
  if ((count == 3U) && (strcmp(arguments[1], "get") == 0)) {
    return weixin_command_get(self, arguments[2]);
  }
  if ((count == 4U) && (strcmp(arguments[1], "set") == 0)) {
    return weixin_command_set(self, arguments[2], arguments[3]);
  }
  if ((count == 2U) && (strcmp(arguments[1], "help") == 0)) {
    return weixin_send_literal(
        self, "[param] help: list|get|set|help\r\n");
  }
  if ((count >= 2U) &&
      ((strcmp(arguments[1], "list") == 0) ||
       (strcmp(arguments[1], "get") == 0) ||
       (strcmp(arguments[1], "set") == 0) ||
       (strcmp(arguments[1], "help") == 0))) {
    return weixin_send_literal(self, "[param] err param\r\n");
  }

  return weixin_send_literal(self, "[param] err unknown_cmd\r\n");
}

/**
 * @brief  初始化 WEIXIN Serial Monitor 协议组件。
 * @param  self：调用方提供的协议对象。
 * @param  params：静态参数表。
 * @param  param_count：参数表元素数量，必须大于零。
 * @param  write：完整协议响应写回函数。
 * @param  write_context：传递给 write 的上下文，允许为 NULL。
 * @param  command_handler：应用自定义命令回调，不需要扩展命令时允许为 NULL。
 * @param  command_context：传递给 command_handler 的上下文，允许为 NULL。
 * @retval PLATFORM_OK：初始化成功。
 * @retval PLATFORM_ERR_INVALID_ARGUMENT：对象、参数表或回调无效。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：参数描述内容无效。
 */
int weixin_protocol_init(struct weixin_protocol *self,
                         const struct weixin_param *params,
                         uint16_t param_count,
                         weixin_protocol_write_t write,
                         void *write_context,
                         weixin_protocol_command_handler_t command_handler,
                         void *command_context)
{
  int result;

  if ((self == NULL) || (params == NULL) || (write == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }

  result = weixin_params_validate(params, param_count);
  if (result != PLATFORM_OK) {
    return result;
  }

  self->params = params;
  self->param_count = param_count;
  self->write = write;
  self->write_context = write_context;
  self->command_handler = command_handler;
  self->command_context = command_context;
  self->line[0] = '\0';
  self->response[0] = '\0';
  self->line_length = 0U;
  self->dropping_line = false;
  self->initialized = true;
  return PLATFORM_OK;
}

/**
 * @brief  向协议组件输入一个 UART 接收字节。
 * @param  self：已经初始化的协议对象。
 * @param  byte：UART 收到的一个字节。
 * @retval PLATFORM_OK：字节已接收、被忽略或命令处理成功。
 * @retval PLATFORM_ERR_INVALID_STATE：协议对象尚未初始化。
 * @return 其他 PLATFORM_* 表示协议响应写回失败。
 */
int weixin_protocol_feed_byte(struct weixin_protocol *self, uint8_t byte)
{
  int result;

  if ((self == NULL) || (!self->initialized)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  if ((byte == '\r') || (byte == '\n')) {
    if (self->dropping_line) {
      self->dropping_line = false;
      self->line_length = 0U;
      return PLATFORM_OK;
    }
    if (self->line_length == 0U) {
      return PLATFORM_OK;
    }

    self->line[self->line_length] = '\0';
    result = weixin_handle_line(self, self->line);
    self->line_length = 0U;
    return result;
  }

  if (self->dropping_line ||
      (byte < WEIXIN_PROTOCOL_ASCII_MIN) ||
      (byte > WEIXIN_PROTOCOL_ASCII_MAX)) {
    return PLATFORM_OK;
  }

  if (self->line_length >= (WEIXIN_PROTOCOL_LINE_CAPACITY - 1U)) {
    self->line_length = 0U;
    self->dropping_line = true;
    return weixin_send_literal(self,
                               "[param] err line_too_long\r\n");
  }

  self->line[self->line_length] = (char)byte;
  self->line_length++;
  return PLATFORM_OK;
}

/**
 * @brief  丢弃当前尚未完成的命令行并恢复初始接收状态。
 * @param  self 已经初始化的协议对象。
 * @return PLATFORM_OK 表示状态已重置，对象无效时返回状态错误。
 * @note   本函数不发送协议响应，只清理接收状态。
 */
int weixin_protocol_reset_line(struct weixin_protocol *self)
{
  if ((self == NULL) || (!self->initialized)) {
    return PLATFORM_ERR_INVALID_STATE;
  }

  self->line_length = 0U;
  self->dropping_line = false;
  return PLATFORM_OK;
}
