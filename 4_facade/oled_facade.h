#ifndef MYUSED_MYFACADES_OLED_FACADE_H
#define MYUSED_MYFACADES_OLED_FACADE_H

/** @file oled_facade.h
 * @brief Compatibility calls target the Board-selected OLED. Use Device APIs for multiple displays.
 */
#include "../3_board/oled_board.h"
#include <stdarg.h>
#include <stdio.h>

static inline void OLED_Init(void)
{
    OLED_InitDeviceEx(OLED_Board_GetDevice());
}

static inline void OLED_Update(void)
{
    OLED_UpdateDevice(OLED_Board_GetDevice());
}

static inline void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    OLED_UpdateAreaDevice(OLED_Board_GetDevice(), X, Y, Width, Height);
}

static inline void OLED_Clear(void)
{
    OLED_ClearDevice(OLED_Board_GetDevice());
}

static inline void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    OLED_ClearAreaDevice(OLED_Board_GetDevice(), X, Y, Width, Height);
}

static inline void OLED_Reverse(void)
{
    OLED_ReverseDevice(OLED_Board_GetDevice());
}

static inline void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    OLED_ReverseAreaDevice(OLED_Board_GetDevice(), X, Y, Width, Height);
}

static inline void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize)
{
    OLED_ShowCharDevice(OLED_Board_GetDevice(), X, Y, Char, FontSize);
}

static inline void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize)
{
    OLED_ShowStringDevice(OLED_Board_GetDevice(), X, Y, String, FontSize);
}

static inline void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    OLED_ShowNumDevice(OLED_Board_GetDevice(), X, Y, Number, Length, FontSize);
}

static inline void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize)
{
    OLED_ShowSignedNumDevice(OLED_Board_GetDevice(), X, Y, Number, Length, FontSize);
}

static inline void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    OLED_ShowHexNumDevice(OLED_Board_GetDevice(), X, Y, Number, Length, FontSize);
}

static inline void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    OLED_ShowBinNumDevice(OLED_Board_GetDevice(), X, Y, Number, Length, FontSize);
}

static inline void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
    OLED_ShowFloatNumDevice(OLED_Board_GetDevice(), X, Y, Number, IntLength, FraLength, FontSize);
}

static inline void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
    OLED_ShowImageDevice(OLED_Board_GetDevice(), X, Y, Width, Height, Image);
}

static inline void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...)
{
    char text[256];
    va_list args;
    if (format == NULL) { return; }
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    OLED_ShowStringDevice(OLED_Board_GetDevice(), X, Y, text, FontSize);
}

static inline void OLED_DrawPoint(int16_t X, int16_t Y)
{
    OLED_DrawPointDevice(OLED_Board_GetDevice(), X, Y);
}

static inline uint8_t OLED_GetPoint(int16_t X, int16_t Y)
{
    return OLED_GetPointDevice(OLED_Board_GetDevice(), X, Y);
}

static inline void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1)
{
    OLED_DrawLineDevice(OLED_Board_GetDevice(), X0, Y0, X1, Y1);
}

static inline void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
    OLED_DrawRectangleDevice(OLED_Board_GetDevice(), X, Y, Width, Height, IsFilled);
}

static inline void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled)
{
    OLED_DrawTriangleDevice(OLED_Board_GetDevice(), X0, Y0, X1, Y1, X2, Y2, IsFilled);
}

static inline void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled)
{
    OLED_DrawCircleDevice(OLED_Board_GetDevice(), X, Y, Radius, IsFilled);
}

static inline void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
    OLED_DrawEllipseDevice(OLED_Board_GetDevice(), X, Y, A, B, IsFilled);
}

static inline void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
    OLED_DrawArcDevice(OLED_Board_GetDevice(), X, Y, Radius, StartAngle, EndAngle, IsFilled);
}

#endif
