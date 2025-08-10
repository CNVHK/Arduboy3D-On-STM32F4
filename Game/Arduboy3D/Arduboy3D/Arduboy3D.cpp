#include "Arduboy32.h"
#include "Defines.h"
#include "Game.h"
#include "Draw.h"
#include "FixedMath.h"
#include "Platform.h"

#include "main.h"
#include "matrixkey.h"
#include "oled.h"
#include "u8g2.h"

uint8_t Platform::GetInput()
{
  uint8_t result = 0;
  uint8_t keyNum = MatrixKey_Scan();

  if (keyNum != 0x00)
  {
    if (keyNum & UP_BUTTON)    result |= INPUT_UP;
    if (keyNum & DOWN_BUTTON)  result |= INPUT_DOWN;
    if (keyNum & LEFT_BUTTON)  result |= INPUT_LEFT;
    if (keyNum & RIGHT_BUTTON) result |= INPUT_RIGHT;
    if (keyNum & A_BUTTON)     result |= INPUT_A;
    if (keyNum & B_BUTTON)     result |= INPUT_B;
  }

  return result;
}

void Platform::PlaySound(const uint16_t* audioPattern)
{
}

void Platform::SetLED(uint8_t r, uint8_t g, uint8_t b)
{
}

void Platform::PutPixel(uint8_t x, uint8_t y, uint8_t colour)
{
  if (x >= 128 || y >= 64) return;
  u8g2_SetDrawColor(&screen, colour);
  u8g2_DrawPixel(&screen, x, y);
}

// Adpated from https://github.com/a1k0n/arduboy3d/blob/master/draw.cpp
const uint8_t topmask_[] = {
  0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80 };
const uint8_t bottommask_[] = {
  0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };

void Platform::DrawVLine(uint8_t x, int8_t y0, int8_t y1, uint8_t pattern) 
{
  if (y1 < y0 || y1 < 0 || y0 > 63) return;
  if (y0 < 0) y0 = 0;
  if (y1 > 63) y1 = 63;

  /* 用 U8g2 自带的垂直线即可，pattern 先忽略 */
  u8g2_SetDrawColor(&screen, (pattern) ? COLOUR_WHITE : COLOUR_BLACK);
  u8g2_DrawVLine(&screen, x, y0, y1 - y0 + 1);
}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame)
{
  uint8_t width = bitmap[0];
  uint8_t height = bitmap[1];
  uint16_t frame_size = ((width * height) / 8); // 字节数
  const uint8_t *frame_data = bitmap + 2; // + (frame * frame_size);
  u8g2_SetDrawColor(&screen, COLOUR_WHITE);
  u8g2_DrawXBM(&screen, x, y, width, height, frame_data);
}

bool Platform::GetPixel(const uint8_t *ptr, uint16_t bitIndex)
{
    return (ptr[bitIndex >> 3] >> (7 - (bitIndex & 7))) & 1;
}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap,
  const uint8_t *mask, uint8_t frame, uint8_t mask_frame)
{
  if(!bitmap || !mask) return;

  const uint8_t w = bitmap[0];
  const uint8_t h = bitmap[1];

  /* 单帧字节数 */
  const uint16_t frameBytes = ((w * h) + 7) >> 3;

  /* 指向当前帧的数据（跳过宽高2字节）*/
  const uint8_t *spriteBits = bitmap  + 2 + frame      * frameBytes;
  const uint8_t *maskBits   = mask    + 2 + mask_frame * frameBytes;

  /* 逐像素处理 */
  for(uint8_t dy = 0; dy < h; ++dy)
  {
    for(uint8_t dx = 0; dx < w; ++dx)
    {
      uint16_t bitIdx = dy * w + dx;

      /* mask 为 0 → 完全透明，跳过 */
      if(!GetPixel(maskBits, bitIdx)) continue;

      /* 真正像素 */
      uint8_t colour = GetPixel(spriteBits, bitIdx) ? COLOUR_WHITE : COLOUR_BLACK;

      int16_t px = x + dx;
      int16_t py = y + dy;

      if(px < 0 || px >= 128 || py < 0 || py >= 64) continue; /* 屏幕外直接丢弃 */

      u8g2_SetDrawColor(&screen, colour);
      u8g2_DrawPixel(&screen, px, py);
    }
  }
}

void Platform::DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
  DrawSprite(x, y, bitmap, 0);
}

void Platform::DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
  uint8_t w = bitmap[0];
  uint8_t h = bitmap[1];
  u8g2_SetDrawColor(&screen, COLOUR_BLACK);
  //u8g2_DrawBox(&screen, x, y, w, h);           // 先擦黑
  DrawBitmap(x, y, bitmap);                     // 再画图案
}

void Platform::FillScreen(uint8_t colour)
{
  u8g2_SetDrawColor(&screen, colour);
  u8g2_DrawBox(&screen, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

unsigned long lastTimingSample;

bool Platform::IsAudioEnabled()
{
	return false;
}

void Platform::SetAudioEnabled(bool isEnabled)
{
}

void Platform::ExpectLoadDelay()
{
	// Resets the timer so that we don't tick multiple times after a level load
	lastTimingSample = millis();
}

void setup()
{
  Game::Init();
  lastTimingSample = millis();
}

void loop()
{
  static int16_t tickAccum = 0;
  unsigned long timingSample = millis();
  tickAccum += (timingSample - lastTimingSample);
  lastTimingSample = timingSample;
	
	constexpr int16_t frameDuration = 1000 / TARGET_FRAMERATE;
	while(tickAccum > frameDuration)
	{
		Game::Tick();
		tickAccum -= frameDuration;
	}
	
	Game::Draw();
}
