// Compact font taken from
// https://hackaday.io/project/6309-vga-graphics-over-spi-and-serial-vgatonic/log/20759-a-tiny-4x6-pixel-font-that-will-fit-on-almost-any-microcontroller-license-mit

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Defines.h"
#include "Font.h"
#include "Platform.h"
#include "Generated/SpriteTypes.h"
#include "oled.h"
#include "u8g2.h"

void Font::PrintString(const char* str, uint8_t line, uint8_t x, uint8_t colour)
{
    static uint8_t backgroundflag = 0;
    
    if (str)
    {
        backgroundflag = 1;
        u8g2_SetDrawColor(&screen, !colour);
        u8g2_DrawBox(&screen, x, line * 12 + 3, 6 * strlen(str), 12);
        
    }
    else
    {
        if (backgroundflag)
        {
            backgroundflag = 0;
            u8g2_SetDrawColor(&screen, colour);
            u8g2_DrawBox(&screen, x, line * 12 + 3, 6 * strlen(str), 12);
        }
    }

    u8g2_SetFont(&screen, u8g2_font_6x10_tf);
    u8g2_SetDrawColor(&screen, colour);

    /* 行高固定 12 像素，x 像素坐标 */
    uint8_t y = line * 12 + 11;          // +11 让基线对齐
    u8g2_DrawStr(&screen, x, y, str);
}

void Font::PrintInt(uint16_t val, uint8_t line, uint8_t x, uint8_t colour)
{
    char buf[7];
    snprintf(buf, sizeof(buf), "%u", val);
    PrintString(buf, line, x, colour);
}

void Font::DrawChar(uint8_t x0, uint8_t y0, char c, uint8_t colour)
{
    char buf[2] = { c, 0 };
    u8g2_SetFont(&screen, u8g2_font_6x10_tf);
    u8g2_SetDrawColor(&screen, colour);
    u8g2_DrawStr(&screen, x0, y0, buf);
}

