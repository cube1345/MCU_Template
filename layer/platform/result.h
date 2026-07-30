/**
 * @file    result.h
 * @brief   工程通用 int 返回码定义。
 * @author  weixin
 */
#ifndef RESULT_H
#define RESULT_H

#include <stdint.h>

#define PLATFORM_OK (0)
#define PLATFORM_ERR_INVALID_ARGUMENT (-1)
#define PLATFORM_ERR_INVALID_STATE (-2)
#define PLATFORM_ERR_INVALID_CONFIGURATION (-3)
#define PLATFORM_ERR_TIMEOUT (-4)
#define PLATFORM_ERR_BUSY (-5)
#define PLATFORM_ERR_IO (-6)
#define PLATFORM_ERR_ISR_CONTEXT (-7)
#define PLATFORM_ERR_UNSUPPORTED (-8)
#define PLATFORM_WAIT_FOREVER UINT32_MAX

#endif /* RESULT_H */
