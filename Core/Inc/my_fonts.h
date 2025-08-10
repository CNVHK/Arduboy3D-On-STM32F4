#ifndef _MY_FONTS_H
#define _MY_FONTS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef U8G2_USE_LARGE_FONTS
#define U8G2_USE_LARGE_FONTS
#endif

#ifndef U8X8_FONT_SECTION

#ifdef __GNUC__
#define U8X8_SECTION(name) __attribute__((section(name)))
#else
#define U8X8_SECTION(name)
#endif

#if defined(__GNUC__) && defined(__AVR__)
#define U8X8_FONT_SECTION(name) U8X8_SECTION(".progmem." name)
#endif

#ifndef U8X8_FONT_SECTION
#define U8X8_FONT_SECTION(name)
#endif

#endif

#ifndef U8G2_FONT_SECTION
#define U8G2_FONT_SECTION(name) U8X8_FONT_SECTION(name)
#endif

    extern const uint8_t u8g2_font_wqy12_t_gb2312_lite[] U8G2_FONT_SECTION("u8g2_font_wqy12_t_gb2312_lite");
    extern const uint8_t u8g2_font_wqy14_t_gb2312_lite[] U8G2_FONT_SECTION("u8g2_font_wqy14_t_gb2312_lite");

#ifdef __cplusplus
}
#endif

#endif
