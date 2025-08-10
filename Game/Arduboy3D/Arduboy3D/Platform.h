#pragma once

#include "oled.h"
#include <stdint.h>

class Platform
{
public:
	static uint8_t GetInput(void);
	static void SetLED(uint8_t r, uint8_t g, uint8_t b);

	static void PlaySound(const uint16_t* audioPattern);
	static bool IsAudioEnabled();
	static void SetAudioEnabled(bool isEnabled);

	static void ExpectLoadDelay();
	
	static void FillScreen(uint8_t col);
	static bool GetPixel(const uint8_t *ptr, uint16_t bitIndex);
	static void PutPixel(uint8_t x, uint8_t y, uint8_t colour);
	static void DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap);
	static void DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap);
	static void DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, const uint8_t *mask, uint8_t frame, uint8_t mask_frame);
	static void DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame);	

	static void DrawVLine(uint8_t x, int8_t y1, int8_t y2, uint8_t pattern);
	static void DrawBackground();
};

/* 拿到 U8g2 当前使用的显存首地址 */
static inline uint8_t *GetRawBuffer(void)
{
    return u8g2_GetBufferPtr(&screen);
}

/* 计算第 y 行所在的页首地址（128 字节/页）*/
static inline uint8_t *GetPagePtr(int16_t y)
{
    uint8_t page = y >> 3;           // 0..7
    return GetRawBuffer() + (page << 7);   // page * 128
}

/* 把改好的页刷回 U8g2（可选：立即发屏或等下次 SendBuffer）*/
static inline void FlushPage(int16_t y)
{
    /* 如果 U8g2 使用双缓冲，可省略；SendBuffer 时统一发 */
    (void)y;
}
