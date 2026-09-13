#ifndef DEVICE_OPS_H
#define DEVICE_OPS_H

#include "stdint.h"

// 开关类设备的通用 ops 接口，他们的操作只有开关反转
typedef struct {
    void (*on)(void *me);
    void (*off)(void *me);
    void (*toggle)(void *me);
} device_switch_ops_t;

// 输入类设备的通用 ops 接口，例如按键、循迹、限位开关等
typedef struct {
    uint8_t (*scan)(void *me);
    uint8_t (*read_level)(void *me);
} device_input_ops_t;

// 计数类设备的通用 ops 接口，例如编码器等
typedef struct {
    int32_t (*get_count)(void *me);
    void (*reset_count)(void *me);
    void (*update_from_ab)(void *me,
                           uint8_t changed_a,
                           uint8_t changed_b,
                           uint8_t level_a,
                           uint8_t level_b);
} device_counter_ops_t;

/**
 * @brief 显示类设备通用 ops 接口，例如 OLED。
 */
typedef struct {
    /* 基础操作 */
    void (*init)(void *me);
    void (*init_ex)(void *me);
    void (*update)(void *me);
    void (*update_area)(void *me, int16_t X, int16_t Y, uint8_t W, uint8_t H);
    void (*clear)(void *me);
    void (*clear_area)(void *me, int16_t X, int16_t Y, uint8_t W, uint8_t H);
    void (*reverse)(void *me);
    void (*reverse_area)(void *me, int16_t X, int16_t Y, uint8_t W, uint8_t H);
    /* 像素操作 */
    void (*draw_point)(void *me, int16_t X, int16_t Y);
    uint8_t (*get_point)(void *me, int16_t X, int16_t Y);
    /* 图形绘制 */
    void (*draw_line)(void *me, int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
    void (*draw_rectangle)(void *me, int16_t X, int16_t Y, uint8_t W, uint8_t H, uint8_t IsFilled);
    void (*draw_triangle)(void *me, int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
    void (*draw_circle)(void *me, int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
    void (*draw_ellipse)(void *me, int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
    void (*draw_arc)(void *me, int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);
    /* 文字渲染 */
    void (*show_char)(void *me, int16_t X, int16_t Y, char Char, uint8_t FontSize);
    void (*show_string)(void *me, int16_t X, int16_t Y, char *String, uint8_t FontSize);
    void (*show_num)(void *me, int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
    void (*show_signed_num)(void *me, int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
    void (*show_hex_num)(void *me, int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
    void (*show_bin_num)(void *me, int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
    void (*show_float_num)(void *me, int16_t X, int16_t Y, double Number, uint8_t IntLen, uint8_t FraLen, uint8_t FontSize);
    /* 位图与格式化输出 */
    void (*show_image)(void *me, int16_t X, int16_t Y, uint8_t W, uint8_t H, const uint8_t *Image);
    void (*printf)(void *me, int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);
} device_display_ops_t;

/**
 * @brief 传感器类设备通用 ops 接口，例如 IMU。
 */
typedef struct {
    int32_t (*init)(void *me);
    int32_t (*read_raw)(void *me, void *raw_data);
    int32_t (*read_scaled)(void *me, void *scaled_data);
} device_sensor_ops_t;

/**
 * @brief 多路输入阵列通用 ops 接口，例如五路循迹模块。
 */
typedef struct {
    int32_t (*read_all)(void *me, uint8_t *data, uint8_t count);
} device_array_input_ops_t;

/**
 * @brief 直流电机类设备通用 ops 接口。
 */
typedef struct {
    void (*start)(void *me);
    void (*stop)(void *me);
    void (*set_pwm)(void *me, int32_t pwm_a, int32_t pwm_b);
    int32_t (*velocity_calc)(void *me, uint8_t channel, int32_t target, int32_t current);
} device_motor_ops_t;

/**
 * @brief 步进电机类设备通用 ops 接口。
 */
typedef struct {
    /* 核心操作 */
    void (*enable)(void *me, uint8_t enable);
    void (*set_direction)(void *me, uint8_t direction);
    uint8_t (*set_frequency)(void *me, uint32_t frequency_hz);
    void (*start)(void *me);
    void (*stop)(void *me);
    void (*service)(void *me);
    /* 配置操作 */
    void (*set_enable_polarity)(void *me, uint8_t polarity);
    void (*set_forward_level)(void *me, uint8_t forward_level_high);
    uint8_t (*set_pulse_width_us)(void *me, uint32_t pulse_width_us);
    /* 便捷操作 */
    void (*toggle_direction)(void *me);
    /* 状态查询 */
    void *(*get_state)(void *me);
} device_stepper_ops_t;

/**
 * @brief 磁编码器类设备通用 ops 接口，例如 MT6701。
 */
typedef struct {
    int32_t (*read_angle_raw)(void *me, uint8_t axis);
    float   (*read_angle_rad)(void *me, uint8_t axis);
    float   (*read_angle_deg)(void *me, uint8_t axis);
    uint8_t (*get_field_status)(void *me, uint8_t axis);
    int32_t (*get_last_angle_raw)(void *me, uint8_t axis);
    uint8_t (*is_crc_valid)(void *me, uint8_t axis);
    float   (*get_speed)(void *me, uint8_t axis);
    void    (*service)(void *me);
} device_magnetic_encoder_ops_t;

#endif
