/* SPDX-License-Identifier: MIT */
/**
 * @file    weixin_protocol.h
 * @brief   WEIXIN Serial Monitor 下位机文本协议组件。
 * @author  weixin
 *
 * 本组件负责命令行接收、参数表查询和参数读写，不依赖 UART、RTOS 或
 * STM32 HAL。调用方通过写回调注入发送能力，并在任务上下文逐字节输入。
 */
#ifndef WEIXIN_PROTOCOL_H
#define WEIXIN_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

/* 协议支持的无符号 8 位参数类型。 */
#define WEIXIN_PARAM_TYPE_U8 UINT8_C(0)
/* 协议支持的有符号 8 位参数类型。 */
#define WEIXIN_PARAM_TYPE_I8 UINT8_C(1)
/* 协议支持的无符号 16 位参数类型。 */
#define WEIXIN_PARAM_TYPE_U16 UINT8_C(2)
/* 协议支持的有符号 16 位参数类型。 */
#define WEIXIN_PARAM_TYPE_I16 UINT8_C(3)
/* 协议支持的无符号 32 位参数类型。 */
#define WEIXIN_PARAM_TYPE_U32 UINT8_C(4)
/* 协议支持的有符号 32 位参数类型。 */
#define WEIXIN_PARAM_TYPE_I32 UINT8_C(5)
/* 协议支持的单精度浮点参数类型。 */
#define WEIXIN_PARAM_TYPE_FLOAT UINT8_C(6)

/* 命令行容量包含结尾的 NUL，满足默认协议建议的至少 96 字节。 */
#define WEIXIN_PROTOCOL_LINE_CAPACITY UINT16_C(128)
/* 单条协议响应容量包含结尾的 NUL。 */
#define WEIXIN_PROTOCOL_RESPONSE_CAPACITY UINT16_C(192)

/**
 * @brief WEIXIN 协议参数描述。
 */
struct weixin_param {
  /* 上位机可见的稳定参数名，不能包含空白字符。 */
  const char *name;
  /* WEIXIN_PARAM_TYPE_* 参数类型。 */
  uint8_t type;
  /* 参数存储地址，读写期间必须保持有效。 */
  void *value;
  /* 上位机元数据和写入检查使用的最小值。 */
  double minimum;
  /* 上位机元数据和写入检查使用的最大值。 */
  double maximum;
  /* true 表示允许 set，false 表示只读。 */
  bool writable;
};

/**
 * @brief 协议响应写回函数。
 * @param context：调用方在初始化时传入的写上下文。
 * @param data：待发送的完整协议响应。
 * @param size：响应长度，不包含字符串结尾的 NUL。
 * @return PLATFORM_OK 表示发送成功；其他 PLATFORM_* 表示失败。
 * @note 回调在 weixin_protocol_feed_byte() 的调用上下文执行。
 */
typedef int (*weixin_protocol_write_t)(void *context,
                                      const uint8_t *data,
                                      uint16_t size);

/**
 * @brief 应用自定义文本命令处理回调。
 * @param context：初始化协议对象时传入的应用命令上下文。
 * @param argument_count：命令行按空格或 Tab 切分后的参数数量。
 * @param arguments：指向各个 NUL 结尾参数字符串的只读指针数组。
 * @retval true：命令已被应用层识别并处理，协议组件不再继续分发。
 * @retval false：命令不属于应用层，由协议组件继续按内置规则处理。
 * @note 回调在 weixin_protocol_feed_byte() 的任务上下文同步执行。
 */
typedef bool (*weixin_protocol_command_handler_t)(
    void *context,
    uint8_t argument_count,
    const char *const *arguments);

/**
 * @brief WEIXIN 协议组件运行状态。
 */
struct weixin_protocol {
  /* 参数表只保存引用，生命周期必须长于本对象。 */
  const struct weixin_param *params;
  /* 参数表元素数量。 */
  uint16_t param_count;
  /* 协议响应写回函数。 */
  weixin_protocol_write_t write;
  /* 原样传递给写回函数的上下文。 */
  void *write_context;
  /* 可选的应用自定义命令处理回调；未配置时为 NULL。 */
  weixin_protocol_command_handler_t command_handler;
  /* 原样传递给应用自定义命令处理回调的上下文。 */
  void *command_context;
  /* 当前正在接收的 ASCII 命令行。 */
  char line[WEIXIN_PROTOCOL_LINE_CAPACITY];
  /* 复用的协议响应格式化缓冲区。 */
  char response[WEIXIN_PROTOCOL_RESPONSE_CAPACITY];
  /* 当前命令行中已经保存的字符数。 */
  uint16_t line_length;
  /* 命令过长后置位，直到收到行结束符才恢复接收。 */
  bool dropping_line;
  /* 初始化成功后置位。 */
  bool initialized;
};

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
                         void *command_context);

/**
 * @brief  向协议组件输入一个 UART 接收字节。
 * @param  self：已经初始化的协议对象。
 * @param  byte：UART 收到的一个字节。
 * @retval PLATFORM_OK：字节已接收、被忽略或命令处理成功。
 * @retval PLATFORM_ERR_INVALID_STATE：协议对象尚未初始化。
 * @return 其他 PLATFORM_* 表示协议响应写回失败。
 * @note   只能在任务上下文调用；本函数可能同步发送协议响应。
 */
int weixin_protocol_feed_byte(struct weixin_protocol *self, uint8_t byte);

/**
 * @brief  丢弃当前尚未完成的命令行并恢复初始接收状态。
 * @param  self 已经初始化的协议对象。
 * @return PLATFORM_OK 表示状态已重置，对象无效时返回状态错误。
 * @note   UART 发生丢字节、溢出或帧错误后应调用本函数，避免残缺行污染下一命令。
 */
int weixin_protocol_reset_line(struct weixin_protocol *self);

#endif /* WEIXIN_PROTOCOL_H */
