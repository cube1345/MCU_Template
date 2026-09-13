#ifndef MYUSED_MYDEVICES_OLED_DRIVER_H
#define MYUSED_MYDEVICES_OLED_DRIVER_H

#include <stdint.h>
#include "device_base.h"
#include "device_ops.h"



typedef void(*OLED_SCL)(void *context, uint8_t state);
typedef void(*OLED_SDA)(void *context, uint8_t state);


typedef struct oled_s oled_t;
typedef device_display_ops_t oled_base_ops_t;

typedef struct {
    OLED_SCL scl_write;
    OLED_SDA sda_write;
    void *context; /**< Caller-owned bus context, valid for object lifetime. */
    uint8_t addr7; /**< Explicit 7-bit address, normally 0x3C or 0x3D. */
} oled_cfg_t;

struct oled_s {
    device_base_t base;
    const oled_base_ops_t *ops;
    oled_cfg_t cfg;
    uint8_t framebuffer[8][128]; /**< Private 128 x 64 pixel buffer. */
};

/** @brief Bind one display and clear its buffer. Invalid cfg leaves is_init=0. */
void OLED_InitDevice(oled_t *me, const oled_cfg_t *cfg);
void OLED_InitDeviceEx(oled_t *me);
void OLED_UpdateDevice(oled_t *me);
void OLED_ClearDevice(oled_t *me);


/*参数宏定义*********************/

/*FontSize参数取值*/
/*此参数值不仅用于判断，而且用于计算横向字符偏移，默认值为字体像素宽度*/
#define OLED_8X16				8
#define OLED_6X8				6

/*IsFilled参数数值*/
#define OLED_UNFILLED			0
#define OLED_FILLED				1

/*********************参数宏定义*/


/*函数声明*********************/

/*初始化函数*/


/*更新函数*/

void OLED_UpdateAreaDevice(oled_t *me, int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/*显存控制函数*/

void OLED_ClearAreaDevice(oled_t *me, int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
void OLED_ReverseDevice(oled_t *me);
void OLED_ReverseAreaDevice(oled_t *me, int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/** @name Instance drawing API
 * @brief All calls use me; null/uninitialized objects are safe no-ops.
 */
/*显示函数*/
void OLED_ShowCharDevice(oled_t *me, int16_t X, int16_t Y, char Char, uint8_t FontSize);
void OLED_ShowStringDevice(oled_t *me, int16_t X, int16_t Y, char *String, uint8_t FontSize);
void OLED_ShowNumDevice(oled_t *me, int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowSignedNumDevice(oled_t *me, int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowHexNumDevice(oled_t *me, int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowBinNumDevice(oled_t *me, int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowFloatNumDevice(oled_t *me, int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
void OLED_ShowImageDevice(oled_t *me, int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image);
void OLED_PrintfDevice(oled_t *me, int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);

/*绘图函数*/
void OLED_DrawPointDevice(oled_t *me, int16_t X, int16_t Y);
uint8_t OLED_GetPointDevice(oled_t *me, int16_t X, int16_t Y);
void OLED_DrawLineDevice(oled_t *me, int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void OLED_DrawRectangleDevice(oled_t *me, int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled);
void OLED_DrawTriangleDevice(oled_t *me, int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
void OLED_DrawCircleDevice(oled_t *me, int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
void OLED_DrawEllipseDevice(oled_t *me, int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
void OLED_DrawArcDevice(oled_t *me, int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);

/*********************函数声明*/

#endif /* MYUSED_MYDEVICES_OLED_DRIVER_H */


/*****************江协科技|版权所有****************/
/*****************jiangxiekeji.com*****************/
