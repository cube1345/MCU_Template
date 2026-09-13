/**
 * @file icm42688_board.h
 * @brief ICM42688 板级对象绑定接口。
 * @details 管理默认设备的生命周期与资源，不执行采集算法。
 * @author cube
 * @date 2026-09-13
 * @note Updated: 2026-09-13
 */
#ifndef ICM42688_BOARD_H
#define ICM42688_BOARD_H

#include "icm42688_driver.h"

/**
 * @brief 绑定默认设备配置；不启动传感器，base.is_init 保持 0。
 * @note 重复调用会清除设备运行状态，正常运行期间不要重新绑定。
 */
void ICM42688_Board_Init(void);
/** @brief 返回静态默认对象；绑定与硬件启动必须另行完成。 */
icm42688_t *ICM42688_Board_GetDevice(void);
/** @brief 返回对象绑定的 7 位地址；绑定前为 0。 */
uint8_t ICM42688_Board_GetDeviceAddr(void);

#endif /* ICM42688_BOARD_H */
